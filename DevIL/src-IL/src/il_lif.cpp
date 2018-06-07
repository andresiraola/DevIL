//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_lif.cpp
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_LIF

#include "il_lif.h"

typedef struct LIF_HEAD
{
    char	Id[8];			//"Willy 7"
    ILuint	Version;		// Version Number (260)
    ILuint	Flags;			// Usually 50
    ILuint	Width;
	ILuint	Height;
    ILuint	PaletteCRC;		// CRC of palettes for fast comparison.
    ILuint	ImageCRC;		// CRC of the image.
	ILuint	PalOffset;		// Offset to the palette (not used).
	ILuint	TeamEffect0;	// Team effect offset 0
	ILuint	TeamEffect1;	// Team effect offset 1
} LIF_HEAD;

ILboolean iCheckLif(LIF_HEAD *Header);

LifHandler::LifHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid Lif file.
ILboolean LifHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	LifFile;
	ILboolean	bLif = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("lif"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bLif;
	}

	LifFile = context->impl->iopenr(FileName);
	if (LifFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bLif;
	}

	bLif = isValidF(LifFile);
	context->impl->icloser(LifFile);

	return bLif;
}

//! Checks if the ILHANDLE contains a valid Lif file at the current position.
ILboolean LifHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid Lif lump.
ILboolean LifHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the Lif header from the current file.
ILboolean iGetLifHead(ILcontext* context, LIF_HEAD *Header)
{
	context->impl->iread(context, Header->Id, 1, 8);

	Header->Version = GetLittleUInt(context);

	Header->Flags = GetLittleUInt(context);

	Header->Width = GetLittleUInt(context);

	Header->Height = GetLittleUInt(context);

	Header->PaletteCRC = GetLittleUInt(context);

	Header->ImageCRC = GetLittleUInt(context);

	Header->PalOffset = GetLittleUInt(context);

	Header->TeamEffect0 = GetLittleUInt(context);

	Header->TeamEffect1 = GetLittleUInt(context);


	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean LifHandler::isValidInternal()
{
	LIF_HEAD	Head;

	if (!iGetLifHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(LIF_HEAD), IL_SEEK_CUR);

	return iCheckLif(&Head);
}

// Internal function used to check if the HEADER is a valid Lif header.
ILboolean iCheckLif(LIF_HEAD *Header)
{
	if (Header->Version != 260 || Header->Flags != 50)
		return IL_FALSE;
	if (stricmp(Header->Id, "Willy 7"))
		return IL_FALSE;
	return IL_TRUE;
}

//! Reads a .Lif file
ILboolean LifHandler::load(ILconst_string FileName)
{
	ILHANDLE	LifFile;
	ILboolean	bLif = IL_FALSE;

	LifFile = context->impl->iopenr(FileName);
	if (LifFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bLif;
	}

	bLif = loadF(LifFile);
	context->impl->icloser(LifFile);

	return bLif;
}

//! Reads an already-opened .Lif file
ILboolean LifHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a .Lif
ILboolean LifHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

ILboolean LifHandler::loadInternal()
{
	LIF_HEAD	LifHead;
	ILuint		i;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetLifHead(context, &LifHead))
		return IL_FALSE;

	if (!ilTexImage(context, LifHead.Width, LifHead.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 1024);
	if (context->impl->iCurImage->Pal.Palette == NULL)
		return IL_FALSE;
	context->impl->iCurImage->Pal.PalSize = 1024;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGBA32;

	if (context->impl->iread(context, context->impl->iCurImage->Data, LifHead.Width * LifHead.Height, 1) != 1)
		return IL_FALSE;
	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 1024) != 1024)
		return IL_FALSE;

	// Each data offset is offset by -1, so we add one.
	for (i = 0; i < context->impl->iCurImage->SizeOfData; i++) {
		context->impl->iCurImage->Data[i]++;
	}

	return ilFixImage(context);
}

#endif//IL_NO_LIF