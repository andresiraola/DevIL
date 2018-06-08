//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pix.cpp
//
// Description: Reads from an Alias | Wavefront .pix file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_PIX

#include "il_endian.h"
#include "il_pix.h"

#ifdef _MSC_VER
#pragma pack(push, pix_struct, 1)
#endif

typedef struct PIXHEAD
{
	ILushort	Width;
	ILushort	Height;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Bpp;
} IL_PACKSTRUCT PIXHEAD;

#ifdef _MSC_VER
#pragma pack(pop, pix_struct)
#endif

ILboolean iCheckPix(PIXHEAD *Header);

PixHandler::PixHandler(ILcontext* context) :
	context(context)
{

}

// Internal function used to get the Pix header from the current file.
ILboolean iGetPixHead(ILcontext* context, PIXHEAD *Header)
{
	Header->Width = GetBigUShort(context);
	Header->Height = GetBigUShort(context);
	Header->OffX = GetBigUShort(context);
	Header->OffY = GetBigUShort(context);
	Header->Bpp = GetBigUShort(context);

	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean PixHandler::isValidInternal()
{
	PIXHEAD	Head;

	if (!iGetPixHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(PIXHEAD), IL_SEEK_CUR);

	return iCheckPix(&Head);
}

// Internal function used to check if the HEADER is a valid Pix header.
ILboolean iCheckPix(PIXHEAD *Header)
{
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 24)
		return IL_FALSE;
	//if (Header->OffY != Header->Height)
	//	return IL_FALSE;

	return IL_TRUE;
}

//! Reads a Pix file
ILboolean PixHandler::load(ILconst_string FileName)
{
	ILHANDLE	PixFile;
	ILboolean	bPix = IL_FALSE;

	PixFile = context->impl->iopenr(FileName);
	if (PixFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPix;
	}

	bPix = loadF(PixFile);
	context->impl->icloser(PixFile);

	return bPix;
}

//! Reads an already-opened Pix file
ILboolean PixHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a Pix
ILboolean PixHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the Pix.
ILboolean PixHandler::loadInternal()
{
	PIXHEAD	Header;
	ILuint	i, j;
	ILubyte	ByteHead, Colour[3];

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPixHead(context, &Header))
		return IL_FALSE;
	if (!iCheckPix(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < context->impl->iCurImage->SizeOfData; ) {
		ByteHead = context->impl->igetc(context);
		if (context->impl->iread(context, Colour, 1, 3) != 3)
			return IL_FALSE;
		for (j = 0; j < ByteHead; j++) {
			context->impl->iCurImage->Data[i++] = Colour[0];
			context->impl->iCurImage->Data[i++] = Colour[1];
			context->impl->iCurImage->Data[i++] = Colour[2];
		}
	}

	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return ilFixImage(context);
}

#endif//IL_NO_PIX