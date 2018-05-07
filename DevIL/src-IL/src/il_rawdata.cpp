//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/rawdata.cpp
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
//#ifndef IL_NO_DATA


ILboolean iLoadDataInternal(ILcontext* context, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp);


//! Reads a raw data file
ILboolean ILAPIENTRY ilLoadData(ILcontext* context, ILconst_string FileName, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	ILHANDLE	RawFile;
	ILboolean	bRaw = IL_FALSE;

	// No need to check for raw data
	/*if (!iCheckExtension(FileName, "raw")) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bRaw;
	}*/

	RawFile = context->impl->iopenr(FileName);
	if (RawFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bRaw;
	}

	bRaw = ilLoadDataF(context, RawFile, Width, Height, Depth, Bpp);
	context->impl->icloser(RawFile);

	return bRaw;
}


//! Reads an already-opened raw data file
ILboolean ILAPIENTRY ilLoadDataF(ILcontext* context, ILHANDLE File, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadDataInternal(context, Width, Height, Depth, Bpp);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a raw data memory "lump"
ILboolean ILAPIENTRY ilLoadDataL(ILcontext* context, void *Lump, ILuint Size, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	iSetInputLump(context, Lump, Size);
	return iLoadDataInternal(context, Width, Height, Depth, Bpp);
}


// Internal function to load a raw data image
ILboolean iLoadDataInternal(ILcontext* context, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp)
{
	if (context->impl->iCurImage == NULL || ((Bpp != 1) && (Bpp != 3) && (Bpp != 4))) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!ilTexImage(context, Width, Height, Depth, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	// Tries to read the correct amount of data
	if (context->impl->iread(context, context->impl->iCurImage->Data, Width * Height * Depth * Bpp, 1) != 1)
		return IL_FALSE;

	if (context->impl->iCurImage->Bpp == 1)
		context->impl->iCurImage->Format = IL_LUMINANCE;
	else if (context->impl->iCurImage->Bpp == 3)
		context->impl->iCurImage->Format = IL_RGB;
	else  // 4
		context->impl->iCurImage->Format = IL_RGBA;

	return ilFixImage(context);
}


//! Save the current image to FileName as raw data
ILboolean ILAPIENTRY ilSaveData(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE DataFile;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	DataFile = context->impl->iopenr(FileName);
	if (DataFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	context->impl->iwrite(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData);
	context->impl->icloser(DataFile);

	return IL_TRUE;
}


//#endif//IL_NO_DATA
