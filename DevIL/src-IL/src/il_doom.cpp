//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_doom.c
//
// Description: Reads Doom textures and flats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_DOOM
#include "il_pal.h"
#include "il_doompal.h"


ILboolean iLoadDoomInternal(ILcontext* context);
ILboolean iLoadDoomFlatInternal(ILcontext* context);


//
// READ A DOOM IMAGE
//

//! Reads a Doom file
ILboolean ilLoadDoom(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	DoomFile;
	ILboolean	bDoom = IL_FALSE;

	// Not sure of any kind of specified extension...maybe .lmp?
	/*if (!iCheckExtension(FileName, "")) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return NULL;
	}*/

	DoomFile = context->impl->iopenr(FileName);
	if (DoomFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bDoom;
	}

	bDoom = ilLoadDoomF(context, DoomFile);
	context->impl->icloser(DoomFile);

	return bDoom;
}


//! Reads an already-opened Doom file
ILboolean ilLoadDoomF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadDoomInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Doom texture
ILboolean ilLoadDoomL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadDoomInternal(context);
}


// From the DTE sources (mostly by Denton Woods with corrections by Randy Heit)
ILboolean iLoadDoomInternal(ILcontext* context)
{
	ILshort	width, height, graphic_header[2], column_loop, row_loop;
	ILint	column_offset, pointer_position, first_pos;
	ILubyte	post, topdelta, length;
	ILubyte	*NewData;
	ILuint	i;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	first_pos = context->impl->itell(context);  // Needed to go back to the offset table
	width = GetLittleShort(context);
	height = GetLittleShort(context);
	graphic_header[0] = GetLittleShort(context);  // Not even used
	graphic_header[1] = GetLittleShort(context);  // Not even used

	if (!ilTexImage(context, width, height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, IL_DOOMPAL_SIZE);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Pal.PalSize = IL_DOOMPAL_SIZE;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(context->impl->iCurImage->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	// 247 is always the transparent colour (usually cyan)
	memset(context->impl->iCurImage->Data, 247, context->impl->iCurImage->SizeOfData);

	for (column_loop = 0; column_loop < width; column_loop++) {
		column_offset = GetLittleInt(context);
		pointer_position = context->impl->itell(context);
		context->impl->iseek(context, first_pos + column_offset, IL_SEEK_SET);

		while (1) {
			if (context->impl->iread(context, &topdelta, 1, 1) != 1)
				return IL_FALSE;
			if (topdelta == 255)
				break;
			if (context->impl->iread(context, &length, 1, 1) != 1)
				return IL_FALSE;
			if (context->impl->iread(context, &post, 1, 1) != 1)
				return IL_FALSE; // Skip extra byte for scaling

			for (row_loop = 0; row_loop < length; row_loop++) {
				if (context->impl->iread(context, &post, 1, 1) != 1)
					return IL_FALSE;
				if (row_loop + topdelta < height)
					context->impl->iCurImage->Data[(row_loop+topdelta) * width + column_loop] = post;
			}
			context->impl->iread(context, &post, 1, 1); // Skip extra scaling byte
		}

		context->impl->iseek(context, pointer_position, IL_SEEK_SET);
	}

	// Converts palette entry 247 (cyan) to transparent.
	if (ilGetBoolean(context, IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(context, context->impl->iCurImage->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < context->impl->iCurImage->SizeOfData; i++) {
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4 + 3] = context->impl->iCurImage->Data[i] != 247 ? 255 : 0;
		}

		if (!ilTexImage(context, context->impl->iCurImage->Width, context->impl->iCurImage->Height, context->impl->iCurImage->Depth,
			4, IL_RGBA, context->impl->iCurImage->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return ilFixImage(context);
}


//
// READ A DOOM FLAT
//

//! Reads a Doom flat file
ILboolean ilLoadDoomFlat(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	FlatFile;
	ILboolean	bFlat = IL_FALSE;

	// Not sure of any kind of specified extension...maybe .lmp?
	/*if (!iCheckExtension(FileName, "")) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return NULL;
	}*/

	FlatFile = context->impl->iopenr(FileName);
	if (FlatFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bFlat;
	}

	bFlat = ilLoadDoomF(context, FlatFile);
	context->impl->icloser(FlatFile);

	return bFlat;
}


//! Reads an already-opened Doom flat file
ILboolean ilLoadDoomFlatF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadDoomFlatInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Doom flat
ILboolean ilLoadDoomFlatL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadDoomFlatInternal(context);
}


// Basically just ireads 4096 bytes and copies the palette
ILboolean iLoadDoomFlatInternal(ILcontext* context)
{
	ILubyte	*NewData;
	ILuint	i;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!ilTexImage(context, 64, 64, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, IL_DOOMPAL_SIZE);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Pal.PalSize = IL_DOOMPAL_SIZE;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	memcpy(context->impl->iCurImage->Pal.Palette, ilDefaultDoomPal, IL_DOOMPAL_SIZE);

	if (context->impl->iread(context, context->impl->iCurImage->Data, 1, 4096) != 4096)
		return IL_FALSE;

	if (ilGetBoolean(context, IL_CONV_PAL) == IL_TRUE) {
		NewData = (ILubyte*)ialloc(context, context->impl->iCurImage->SizeOfData * 4);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		for (i = 0; i < context->impl->iCurImage->SizeOfData; i++) {
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			NewData[i * 4 + 3] = context->impl->iCurImage->Data[i] != 247 ? 255 : 0;
		}

		if (!ilTexImage(context, context->impl->iCurImage->Width, context->impl->iCurImage->Height, context->impl->iCurImage->Depth,
			4, IL_RGBA, context->impl->iCurImage->Type, NewData)) {
			ifree(NewData);
			return IL_FALSE;
		}
		context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
		ifree(NewData);
	}

	return ilFixImage(context);
}


#endif
