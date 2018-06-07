//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_mdl.cpp
//
// Description: Reads a Half-Life model file (.mdl).
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_MDL

#include "il_mdl.h"

typedef struct TEX_HEAD
{
	char	Name[64];
	ILuint	Flags;
	ILuint	Width;
	ILuint	Height;
	ILuint	Offset;
} TEX_HEAD;

MdlHandler::MdlHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid MDL file.
ILboolean MdlHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	MdlFile;
	ILboolean	bMdl = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("mdl"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bMdl;
	}
	
	MdlFile = context->impl->iopenr(FileName);
	if (MdlFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bMdl;
	}
	
	bMdl = isValidF(MdlFile);
	context->impl->icloser(MdlFile);
	
	return bMdl;
}

//! Checks if the ILHANDLE contains a valid MDL file at the current position.
ILboolean MdlHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Checks if Lump is a valid MDL lump.
ILboolean MdlHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function to get the header and check it.
ILboolean MdlHandler::isValidInternal()
{
	ILuint Id, Version;

	Id = GetLittleUInt(context);
	Version = GetLittleUInt(context);
	context->impl->iseek(context, -8, IL_SEEK_CUR);  // Restore to previous position.

	// 0x54534449 == "IDST"
	if (Id != 0x54534449 || Version != 10)
		return IL_FALSE;
	return IL_TRUE;
}

//! Reads a .mdl file
ILboolean MdlHandler::load(ILconst_string FileName)
{
	ILHANDLE	MdlFile;
	ILboolean	bMdl = IL_FALSE;

	MdlFile = context->impl->iopenr(FileName);
	if (MdlFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bMdl;
	}

	bMdl = loadF(MdlFile);
	context->impl->icloser(MdlFile);

	return bMdl;
}

//! Reads an already-opened .mdl file
ILboolean MdlHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a .mdl
ILboolean MdlHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

ILboolean MdlHandler::loadInternal()
{
	ILuint		Id, Version, NumTex, TexOff, TexDataOff, Position, ImageNum;
	ILubyte		*TempPal;
	TEX_HEAD	TexHead;
	ILimage		*BaseImage=NULL;
	ILboolean	BaseCreated = IL_FALSE;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Id = GetLittleUInt(context);
	Version = GetLittleUInt(context);

	// 0x54534449 == "IDST"
	if (Id != 0x54534449 || Version != 10) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Skips the actual model header.
	context->impl->iseek(context, 172, IL_SEEK_CUR);

	NumTex = GetLittleUInt(context);
	TexOff = GetLittleUInt(context);
	TexDataOff = GetLittleUInt(context);

	if (NumTex == 0 || TexOff == 0 || TexDataOff == 0) {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	context->impl->iseek(context, TexOff, IL_SEEK_SET);

	for (ImageNum = 0; ImageNum < NumTex; ImageNum++) {
		if (context->impl->iread(context, TexHead.Name, 1, 64) != 64)
			return IL_FALSE;
		TexHead.Flags = GetLittleUInt(context);
		TexHead.Width = GetLittleUInt(context);
		TexHead.Height = GetLittleUInt(context);
		TexHead.Offset = GetLittleUInt(context);
		Position = context->impl->itell(context);

		if (TexHead.Offset == 0) {
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		if (!BaseCreated) {
			ilTexImage(context, TexHead.Width, TexHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL);
			context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			BaseCreated = IL_TRUE;
			BaseImage = context->impl->iCurImage;
			//context->impl->iCurImage->NumNext = NumTex - 1;  // Don't count the first image.
		}
		else {
			//context->impl->iCurImage->Next = ilNewImage(TexHead.Width, TexHead.Height, 1, 1, 1);
			context->impl->iCurImage = context->impl->iCurImage->Next;
			context->impl->iCurImage->Format = IL_COLOUR_INDEX;
			context->impl->iCurImage->Type = IL_UNSIGNED_BYTE;
		}

		TempPal	= (ILubyte*)ialloc(context, 768);
		if (TempPal == NULL) {
			context->impl->iCurImage = BaseImage;
			return IL_FALSE;
		}
		context->impl->iCurImage->Pal.Palette = TempPal;
		context->impl->iCurImage->Pal.PalSize = 768;
		context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;

		context->impl->iseek(context, TexHead.Offset, IL_SEEK_SET);
		if (context->impl->iread(context, context->impl->iCurImage->Data, TexHead.Width * TexHead.Height, 1) != 1)
			return IL_FALSE;
		if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 768) != 768)
			return IL_FALSE;

		if (ilGetBoolean(context, IL_CONV_PAL) == IL_TRUE) {
			ilConvertImage(context, IL_RGB, IL_UNSIGNED_BYTE);
		}

		context->impl->iseek(context, Position, IL_SEEK_SET);
	}

	context->impl->iCurImage = BaseImage;

	return ilFixImage(context);
}

#endif//IL_NO_MDL