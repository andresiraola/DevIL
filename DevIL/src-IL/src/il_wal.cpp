//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_wal.cpp
//
// Description: Loads a Quake .wal texture.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WAL
#include "il_q2pal.h"


typedef struct WALHEAD
{
	ILbyte	FileName[32];	// Image name
	ILuint	Width;			// Width of first image
	ILuint	Height;			// Height of first image
	ILuint	Offsets[4];		// Offsets to image data
	ILbyte	AnimName[32];	// Name of next frame
	ILuint	Flags;			// ??
	ILuint	Contents;		// ??
	ILuint	Value;			// ??
} WALHEAD;

ILboolean iLoadWalInternal(ILcontext* context);


//! Reads a .wal file
ILboolean ilLoadWal(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	WalFile;
	ILboolean	bWal = IL_FALSE;

	WalFile = context->impl->iopenr(FileName);
	if (WalFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bWal;
	}

	bWal = ilLoadWalF(context, WalFile);
	context->impl->icloser(WalFile);

	return bWal;
}


//! Reads an already-opened .wal file
ILboolean ilLoadWalF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadWalInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .wal file
ILboolean ilLoadWalL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadWalInternal(context);
}


ILboolean iLoadWalInternal(ILcontext* context)
{
	WALHEAD	Header;
	ILimage	*Mipmaps[3], *CurImage;
	ILuint	i, NewW, NewH;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	CurImage = context->impl->iCurImage;


	// Read header
	context->impl->iread(context, &Header.FileName, 1, 32);
	Header.Width = GetLittleUInt(context);
	Header.Height = GetLittleUInt(context);

	for (i = 0; i < 4; i++)
		Header.Offsets[i] = GetLittleUInt(context);

	context->impl->iread(context, Header.AnimName, 1, 32);
	Header.Flags = GetLittleUInt(context);
	Header.Contents = GetLittleUInt(context);
	Header.Value = GetLittleUInt(context);

	if (!ilTexImage(context, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	for (i = 0; i < 3; i++) {
		Mipmaps[i] = (ILimage*)icalloc(context, sizeof(ILimage), 1);
		if (Mipmaps[i] == NULL)
			goto cleanup_error;
		Mipmaps[i]->Pal.Palette = (ILubyte*)ialloc(context, 768);
		if (Mipmaps[i]->Pal.Palette == NULL)
			goto cleanup_error;
		memcpy(Mipmaps[i]->Pal.Palette, ilDefaultQ2Pal, 768);
		Mipmaps[i]->Pal.PalType = IL_PAL_RGB24;
	}

	NewW = Header.Width;
	NewH = Header.Height;
	for (i = 0; i < 3; i++) {
		NewW /= 2;
		NewH /= 2;
		context->impl->iCurImage = Mipmaps[i];
		if (!ilTexImage(context, NewW, NewH, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			goto cleanup_error;
		// Don't set until now so ilTexImage won't get rid of the palette.
		Mipmaps[i]->Pal.PalSize = 768;
		Mipmaps[i]->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	context->impl->iCurImage = CurImage;
	ilCloseImage(context->impl->iCurImage->Mipmaps);
	context->impl->iCurImage->Mipmaps = Mipmaps[0];
	Mipmaps[0]->Mipmaps = Mipmaps[1];
	Mipmaps[1]->Mipmaps = Mipmaps[2];

	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE)
		ifree(context->impl->iCurImage->Pal.Palette);
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 768);
	if (context->impl->iCurImage->Pal.Palette == NULL)
		goto cleanup_error;

	context->impl->iCurImage->Pal.PalSize = 768;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(context->impl->iCurImage->Pal.Palette, ilDefaultQ2Pal, 768);

	context->impl->iseek(context, Header.Offsets[0], IL_SEEK_SET);
	if (context->impl->iread(context, context->impl->iCurImage->Data, Header.Width * Header.Height, 1) != 1)
		goto cleanup_error;

	for (i = 0; i < 3; i++) {
		context->impl->iseek(context, Header.Offsets[i+1], IL_SEEK_SET);
		if (context->impl->iread(context, Mipmaps[i]->Data, Mipmaps[i]->Width * Mipmaps[i]->Height, 1) != 1)
			goto cleanup_error;
	}

	// Fixes all images, even mipmaps.
	return ilFixImage(context);

cleanup_error:
	for (i = 0; i < 3; i++) {
		ilCloseImage(Mipmaps[i]);
	}
	return IL_FALSE;
}


#endif//IL_NO_WAL
