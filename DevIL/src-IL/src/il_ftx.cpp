//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/12/2009
//
// Filename: src-IL/src/il_ftx.cpp
//
// Description: Reads from a Heavy Metal: FAKK2 (.ftx) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_FTX

ILboolean iLoadFtxInternal(ILcontext* context);


//! Reads a FTX file
ILboolean ilLoadFtx(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	FtxFile;
	ILboolean	bFtx = IL_FALSE;

	FtxFile = context->impl->iopenr(FileName);
	if (FtxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bFtx;
	}

	bFtx = ilLoadFtxF(context, FtxFile);
	context->impl->icloser(FtxFile);

	return bFtx;
}


//! Reads an already-opened FTX file
ILboolean ilLoadFtxF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadFtxInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}


//! Reads from a memory "lump" that contains a FTX
ILboolean ilLoadFtxL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadFtxInternal(context);
}


// Internal function used to load the FTX.
ILboolean iLoadFtxInternal(ILcontext* context)
{
	ILuint Width, Height, HasAlpha;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Width = GetLittleUInt(context);
	Height = GetLittleUInt(context);
	HasAlpha = GetLittleUInt(context);  // Kind of backwards from what I would think...

	//@TODO: Right now, it appears that all images are in RGBA format.  See if I can find specs otherwise
	//  or images that load incorrectly like this.
	//if (HasAlpha == 0) {  // RGBA format
		if (!ilTexImage(context, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	//}
	//else if (HasAlpha == 1) {  // RGB format
	//	if (!ilTexImage(context, Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
	//		return IL_FALSE;
	//}
	//else {  // Unknown format
	//	ilSetError(context, IL_INVALID_FILE_HEADER);
	//	return IL_FALSE;
	//}

	// The origin will always be in the upper left.
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	// All we have to do for this format is read the raw, uncompressed data.
	if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData)
		return IL_FALSE;

	return ilFixImage(context);
}

#endif//IL_NO_FTX

