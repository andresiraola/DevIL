//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_pal.cpp
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_pal.h"
#include <string.h>
#include <ctype.h>
#include <limits.h>


//! Loads a palette from FileName into the current image's palette.
ILboolean ILAPIENTRY ilLoadPal(ILcontext* context, ILconst_string FileName)
{
	FILE		*f;
	ILboolean	IsPsp;
	char		Head[8];

	if (FileName == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iCheckExtension(FileName, IL_TEXT("col"))) {
		return ilLoadColPal(context, FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("act"))) {
		return ilLoadActPal(context, FileName);
	}
	if (iCheckExtension(FileName, IL_TEXT("plt"))) {
		return ilLoadPltPal(context, FileName);
	}

#ifndef _UNICODE
	f = fopen(FileName, "rt");
#else
	f = _wfopen(FileName, L"rt");
#endif//_UNICODE
	if (f == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	fread(Head, 1, 8, f);
	if (!strncmp(Head, "JASC-PAL", 8))
		IsPsp = IL_TRUE;
	else
		IsPsp = IL_FALSE;

	fclose(f);

	if (IsPsp)
		return ilLoadJascPal(context, FileName);
	return ilLoadHaloPal(context, FileName);
}


//! Loads a Paint Shop Pro formatted palette (.pal) file.
ILboolean ilLoadJascPal(ILcontext* context, ILconst_string FileName)
{
	FILE *PalFile;
	ILuint NumColours, i, c;
	ILubyte Buff[BUFFLEN];
	ILboolean Error = IL_FALSE;
	ILpal *Pal = &context->impl->iCurImage->Pal;

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#ifndef _UNICODE
	PalFile = fopen(FileName, "rt");
#else
	PalFile = _wfopen(FileName, L"rt");
#endif//_UNICODE
	if (PalFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize > 0 && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
	}

	iFgetw(context, Buff, BUFFLEN, PalFile);
	if (stricmp((const char*)Buff, "JASC-PAL")) {
		Error = IL_TRUE;
	}
	iFgetw(context, Buff, BUFFLEN, PalFile);
	if (stricmp((const char*)Buff, "0100")) {
		Error = IL_TRUE;
	}

	iFgetw(context, Buff, BUFFLEN, PalFile);
	NumColours = atoi((const char*)Buff);
	if (NumColours == 0 || Error) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		fclose(PalFile);
		return IL_FALSE;
	}
	
	Pal->PalSize = NumColours * PALBPP;
	Pal->PalType = IL_PAL_RGB24;
	Pal->Palette = (ILubyte*)ialloc(context, NumColours * PALBPP);
	if (Pal->Palette == NULL) {
		fclose(PalFile);
		return IL_FALSE;
	}

	for (i = 0; i < NumColours; i++) {
		for (c = 0; c < PALBPP; c++) {
			iFgetw(context, Buff, BUFFLEN, PalFile);
			Pal->Palette[i * PALBPP + c] = atoi((const char*)Buff);
		}
	}

	fclose(PalFile);

	return IL_TRUE;
}


// File Get Word
//	MaxLen must be greater than 1, because the trailing NULL is always stored.
char *iFgetw(ILcontext* context, ILubyte *Buff, ILint MaxLen, FILE *File)
{
	ILint Temp;
	ILint i;

	if (Buff == NULL || File == NULL || MaxLen < 2) {
		ilSetError(context, IL_INVALID_PARAM);
		return NULL;
	}

	for (i = 0; i < MaxLen - 1; i++) {
		Temp = fgetc(File);
		if (Temp == '\n' || Temp == '\0' || Temp == IL_EOF || feof(File)) {
			break;			
		}

		if (Temp == ' ') {
			while (Temp == ' ') {  // Just to get rid of any extra spaces
				Temp = fgetc(File);
			}
			fseek(File, -1, IL_SEEK_CUR);  // Go back one
			break;
		}

		if (!isprint(Temp)) {  // Skips any non-printing characters
			while (!isprint(Temp)) {
				Temp = fgetc(File);
			}
			fseek(File, -1, IL_SEEK_CUR);
			break;
		}

		Buff[i] = Temp;
	}

	Buff[i] = '\0';
	return (char *)Buff;
}


ILboolean ILAPIENTRY ilSavePal(ILcontext* context, ILconst_string FileName)
{
	ILstring Ext = iGetExtension(FileName);

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#ifndef _UNICODE
	if (FileName == NULL || strlen(FileName) < 1 || Ext == NULL) {
#else
	if (FileName == NULL || wcslen(FileName) < 1 || Ext == NULL) {
#endif//_UNICODE
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (!context->impl->iCurImage->Pal.Palette || !context->impl->iCurImage->Pal.PalSize || context->impl->iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iStrCmp(Ext, IL_TEXT("pal"))) {
		return ilSaveJascPal(context, FileName);
	}

	ilSetError(context, IL_INVALID_EXTENSION);
	return IL_FALSE;
}


//! Saves a Paint Shop Pro formatted palette (.pal) file.
ILboolean ilSaveJascPal(ILcontext* context, ILconst_string FileName)
{
	FILE	*PalFile;
	ILuint	i, PalBpp, NumCols = ilGetInteger(context, IL_PALETTE_NUM_COLS);
	ILubyte	*CurPal;

	if (context->impl->iCurImage == NULL || NumCols == 0 || NumCols > 256) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
#ifndef _UNICODE
	if (FileName == NULL || strlen(FileName) < 5) {
#else
	if (FileName == NULL || wcslen(FileName) < 5) {
#endif//_UNICODE
		ilSetError(context, IL_INVALID_VALUE);
		return IL_FALSE;
	}

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	// Create a copy of the current palette and convert it to RGB24 format.
	CurPal = context->impl->iCurImage->Pal.Palette;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (!context->impl->iCurImage->Pal.Palette) {
		context->impl->iCurImage->Pal.Palette = CurPal;
		return IL_FALSE;
	}

	memcpy(context->impl->iCurImage->Pal.Palette, CurPal, context->impl->iCurImage->Pal.PalSize);
	if (!ilConvertPal(context, IL_PAL_RGB24)) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = CurPal;
		return IL_FALSE;
	}

#ifndef _UNICODE
	PalFile = fopen(FileName, "wt");
#else
	PalFile = _wfopen(FileName, L"wt");
#endif//_UNICODE
	if (!PalFile) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	// Header needed on all .pal files
	fputs("JASC-PAL\n0100\n256\n", PalFile);

	PalBpp = ilGetBppPal(context->impl->iCurImage->Pal.PalType);
	for (i = 0; i < context->impl->iCurImage->Pal.PalSize; i += PalBpp) {
		fprintf(PalFile, "%d %d %d\n",
			context->impl->iCurImage->Pal.Palette[i], context->impl->iCurImage->Pal.Palette[i+1], context->impl->iCurImage->Pal.Palette[i+2]);
	}

	NumCols = 256 - NumCols;
	for (i = 0; i < NumCols; i++) {
		fprintf(PalFile, "0 0 0\n");
	}

	ifree(context->impl->iCurImage->Pal.Palette);
	context->impl->iCurImage->Pal.Palette = CurPal;

	fclose(PalFile);

	return IL_TRUE;
}


//! Loads a Halo formatted palette (.pal) file.
ILboolean ilLoadHaloPal(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	HaloFile;
	HALOHEAD	HaloHead;
	ILushort	*TempPal;
	ILuint		i, Size;

	if (!iCheckExtension(FileName, IL_TEXT("pal"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	HaloFile = context->impl->iopenr(FileName);
	if (HaloFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (context->impl->iread(context, &HaloHead, sizeof(HALOHEAD), 1) != 1)
		return IL_FALSE;

	if (HaloHead.Id != 'A' + ('H' << 8) || HaloHead.Version != 0xe3) {
		context->impl->icloser(HaloFile);
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	Size = (HaloHead.MaxIndex + 1) * 3;
	TempPal = (ILushort*)ialloc(context, Size * sizeof(ILushort));
	if (TempPal == NULL) {
		context->impl->icloser(HaloFile);
		return IL_FALSE;
	}

	if (context->impl->iread(context, TempPal, sizeof(ILushort), Size) != Size) {
		context->impl->icloser(HaloFile);
		ifree(TempPal);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize > 0 && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
	}
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	context->impl->iCurImage->Pal.PalSize = Size;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		context->impl->icloser(HaloFile);
		return IL_FALSE;
	}

	for (i = 0; i < context->impl->iCurImage->Pal.PalSize; i++, TempPal++) {
		context->impl->iCurImage->Pal.Palette[i] = (ILubyte)*TempPal;
	}
	TempPal -= context->impl->iCurImage->Pal.PalSize;
	ifree(TempPal);

	context->impl->icloser(HaloFile);

	return IL_TRUE;
}


// Hasn't been tested
//	@TODO: Test the thing!

//! Loads a .col palette file
ILboolean ilLoadColPal(ILcontext* context, ILconst_string FileName)
{
	ILuint		RealFileSize, FileSize;
	ILushort	Version;
	ILHANDLE	ColFile;

	if (!iCheckExtension(FileName, IL_TEXT("col"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ColFile = context->impl->iopenr(FileName);
	if (ColFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize > 0 && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
	}

	context->impl->iseek(context, 0, IL_SEEK_END);
	RealFileSize = ftell((FILE*)ColFile);
	context->impl->iseek(context, 0, IL_SEEK_SET);

	if (RealFileSize > 768) {  // has a header
		fread(&FileSize, 4, 1, (FILE*)ColFile);
		if ((FileSize - 8) % 3 != 0) {  // check to make sure an even multiple of 3!
			context->impl->icloser(ColFile);
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
		if (context->impl->iread(context, &Version, 2, 1) != 1) {
			context->impl->icloser(ColFile);
			return IL_FALSE;
		}
		if (Version != 0xB123) {
			context->impl->icloser(ColFile);
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
		if (context->impl->iread(context, &Version, 2, 1) != 1) {
			context->impl->icloser(ColFile);
			return IL_FALSE;
		}
		if (Version != 0) {
			context->impl->icloser(ColFile);
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 768);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		context->impl->icloser(ColFile);
		return IL_FALSE;
	}

	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 768) != 768) {
		context->impl->icloser(ColFile);
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
		return IL_FALSE;
	}

	context->impl->iCurImage->Pal.PalSize = 768;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;

	context->impl->icloser(ColFile);

	return IL_TRUE;
}


//! Loads an .act palette file.
ILboolean ilLoadActPal(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	ActFile;

	if (!iCheckExtension(FileName, IL_TEXT("act"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	ActFile = context->impl->iopenr(FileName);
	if (ActFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize > 0 && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
	}

	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	context->impl->iCurImage->Pal.PalSize = 768;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 768);
	if (!context->impl->iCurImage->Pal.Palette) {
		context->impl->icloser(ActFile);
		return IL_FALSE;
	}

	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 768) != 768) {
		context->impl->icloser(ActFile);
		return IL_FALSE;
	}

	context->impl->icloser(ActFile);

	return IL_TRUE;
}


//! Loads an .plt palette file.
ILboolean ilLoadPltPal(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PltFile;

	if (!iCheckExtension(FileName, IL_TEXT("plt"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	PltFile = context->impl->iopenr(FileName);
	if (PltFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize > 0 && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
	}

	context->impl->iCurImage->Pal.PalSize = GetLittleUInt(context);
	if (context->impl->iCurImage->Pal.PalSize == 0) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (!context->impl->iCurImage->Pal.Palette) {
		context->impl->icloser(PltFile);
		return IL_FALSE;
	}

	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, context->impl->iCurImage->Pal.PalSize, 1) != 1) {
		ifree(context->impl->iCurImage->Pal.Palette);
		context->impl->iCurImage->Pal.Palette = NULL;
		context->impl->icloser(PltFile);
		return IL_FALSE;
	}

	context->impl->icloser(PltFile);

	return IL_TRUE;
}


// Assumes that Dest has nothing in it.
ILboolean iCopyPalette(ILcontext* context, ILpal *Dest, ILpal *Src)
{
	if (Src->Palette == NULL || Src->PalSize == 0)
		return IL_FALSE;

	Dest->Palette = (ILubyte*)ialloc(context, Src->PalSize);
	if (Dest->Palette == NULL)
		return IL_FALSE;

	memcpy(Dest->Palette, Src->Palette, Src->PalSize);

	Dest->PalSize = Src->PalSize;
	Dest->PalType = Src->PalType;

	return IL_TRUE;
}


ILAPI ILpal* ILAPIENTRY iCopyPal(ILcontext* context)
{
	ILpal *Pal;

	if (context->impl->iCurImage == NULL || context->impl->iCurImage->Pal.Palette == NULL ||
		context->impl->iCurImage->Pal.PalSize == 0 || context->impl->iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return NULL;
	}

	Pal = (ILpal*)ialloc(context, sizeof(ILpal));
	if (Pal == NULL) {
		return NULL;
	}
	if (!iCopyPalette(context, Pal, &context->impl->iCurImage->Pal)) {
		ifree(Pal);
		return NULL;
	}

	return Pal;
}


// Converts the palette to the DestFormat format.
ILAPI ILpal* ILAPIENTRY iConvertPal(ILcontext* context, ILpal *Pal, ILenum DestFormat)
{
	ILpal	*NewPal = NULL;
	ILuint	i, j, NewPalSize;

	// Checks to see if the current image is valid and has a palette
	if (Pal == NULL || Pal->PalSize == 0 || Pal->Palette == NULL || Pal->PalType == IL_PAL_NONE) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return NULL;
	}

	/*if (Pal->PalType == DestFormat) {
		return NULL;
	}*/

	NewPal = (ILpal*)ialloc(context, sizeof(ILpal));
	if (NewPal == NULL) {
		return NULL;
	}
	NewPal->PalSize = Pal->PalSize;
	NewPal->PalType = Pal->PalType;

	switch (DestFormat)
	{
		case IL_PAL_RGB24:
		case IL_PAL_BGR24:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR24:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						j = ilGetBppPal(Pal->PalType);
						for (i = 0; i < Pal->PalSize; i += j) {
							NewPal->Palette[i] = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGR32:
				case IL_PAL_BGRA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(context, NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
				case IL_PAL_RGBA32:
					NewPalSize = (ILuint)((ILfloat)Pal->PalSize * 3.0f / 4.0f);
					NewPal->Palette = (ILubyte*)ialloc(context, NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB24) {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
						}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 4, j += 3) {
							NewPal->Palette[j]   = Pal->Palette[i+2];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i];
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				default:
					ilSetError(context, IL_INVALID_PARAM);
					return NULL;
			}
			break;

		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			switch (Pal->PalType)
			{
				case IL_PAL_RGB24:
				case IL_PAL_BGR24:
					NewPalSize = Pal->PalSize * 4 / 3;
					NewPal->Palette = (ILubyte*)ialloc(context, NewPalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if ((Pal->PalType == IL_PAL_BGR24 && (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32)) ||
						(Pal->PalType == IL_PAL_RGB24 && (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32))) {
							for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
								NewPal->Palette[j]   = Pal->Palette[i+2];
								NewPal->Palette[j+1] = Pal->Palette[i+1];
								NewPal->Palette[j+2] = Pal->Palette[i];
								NewPal->Palette[j+3] = 255;
							}
					}
					else {
						for (i = 0, j = 0; i < Pal->PalSize; i += 3, j += 4) {
							NewPal->Palette[j]   = Pal->Palette[i];
							NewPal->Palette[j+1] = Pal->Palette[i+1];
							NewPal->Palette[j+2] = Pal->Palette[i+2];
							NewPal->Palette[j+3] = 255;
						}
					}
					NewPal->PalSize = NewPalSize;
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGB32:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;

					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_RGBA32:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_BGR32 || DestFormat == IL_PAL_BGRA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;
				
				case IL_PAL_BGR32:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = 255;
						}
					}
					else {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i+2];
							NewPal->Palette[i+3] = 255;
						}
					}
					NewPal->PalType = DestFormat;
					break;

				case IL_PAL_BGRA32:
					NewPal->Palette = (ILubyte*)ialloc(context, Pal->PalSize);
					if (NewPal->Palette == NULL)
						goto alloc_error;
					if (DestFormat == IL_PAL_RGB32 || DestFormat == IL_PAL_RGBA32) {
						for (i = 0; i < Pal->PalSize; i += 4) {
							NewPal->Palette[i]   = Pal->Palette[i+2];
							NewPal->Palette[i+1] = Pal->Palette[i+1];
							NewPal->Palette[i+2] = Pal->Palette[i];
							NewPal->Palette[i+3] = Pal->Palette[i+3];
						}
					}
					else {
						memcpy(NewPal->Palette, Pal->Palette, Pal->PalSize);
					}
					NewPal->PalType = DestFormat;
					break;

				default:
					ilSetError(context, IL_INVALID_PARAM);
					return NULL;
			}
			break;


		default:
			ilSetError(context, IL_INVALID_PARAM);
			return NULL;
	}

	NewPal->PalType = DestFormat;

	return NewPal;

alloc_error:
	ifree(NewPal);
	return NULL;
}


//! Converts the current image to the DestFormat format.
ILboolean ILAPIENTRY ilConvertPal(ILcontext* context, ILenum DestFormat)
{
	ILpal *Pal;

	if (context->impl->iCurImage == NULL || context->impl->iCurImage->Pal.Palette == NULL ||
		context->impl->iCurImage->Pal.PalSize == 0 || context->impl->iCurImage->Pal.PalType == IL_PAL_NONE) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Pal = iConvertPal(context, &context->impl->iCurImage->Pal, DestFormat);
	if (Pal == NULL)
		return IL_FALSE;

	ifree(context->impl->iCurImage->Pal.Palette);
	context->impl->iCurImage->Pal.PalSize = Pal->PalSize;
	context->impl->iCurImage->Pal.PalType = Pal->PalType;

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, Pal->PalSize);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	memcpy(context->impl->iCurImage->Pal.Palette, Pal->Palette, Pal->PalSize);

	ifree(Pal->Palette);
	ifree(Pal);
	
	return IL_TRUE;
}


// Sets the current palette.
ILAPI void ILAPIENTRY ilSetPal(ILcontext* context, ILpal *Pal)
{
	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
	}

	if (Pal->Palette && Pal->PalSize && Pal->PalType != IL_PAL_NONE) {
		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, Pal->PalSize);
		if (context->impl->iCurImage->Pal.Palette == NULL)
			return;
		memcpy(context->impl->iCurImage->Pal.Palette, Pal->Palette, Pal->PalSize);
		context->impl->iCurImage->Pal.PalSize = Pal->PalSize;
		context->impl->iCurImage->Pal.PalType = Pal->PalType;
	}
	else {
		context->impl->iCurImage->Pal.Palette = NULL;
		context->impl->iCurImage->Pal.PalSize = 0;
		context->impl->iCurImage->Pal.PalType = IL_PAL_NONE;
	}

	return;
}


ILuint CurSort = 0;
typedef struct COL_CUBE
{
	ILubyte	Min[3];
	ILubyte	Val[3];
	ILubyte	Max[3];
} COL_CUBE;

int sort_func(void *e1, void *e2)
{
	return ((COL_CUBE*)e1)->Val[CurSort] - ((COL_CUBE*)e2)->Val[CurSort];
}


ILboolean ILAPIENTRY ilApplyPal(ILcontext* context, ILconst_string FileName)
{
	ILimage		Image, *CurImage = context->impl->iCurImage;
	ILubyte		*NewData;
	ILuint		*PalInfo, NumColours, NumPix, MaxDist, DistEntry=0, i, j;
	ILint		Dist1, Dist2, Dist3;
	ILboolean	Same;
	ILenum		Origin;
//	COL_CUBE	*Cubes;

    if( context->impl->iCurImage == NULL || (context->impl->iCurImage->Format != IL_BYTE || context->impl->iCurImage->Format != IL_UNSIGNED_BYTE) ) {
    	ilSetError(context, IL_ILLEGAL_OPERATION);
        return IL_FALSE;
    }

	NewData = (ILubyte*)ialloc(context, context->impl->iCurImage->Width * context->impl->iCurImage->Height * context->impl->iCurImage->Depth);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	context->impl->iCurImage = &Image;
	imemclear(&Image, sizeof(ILimage));
	// IL_PAL_RGB24, because we don't want to make parts transparent that shouldn't be.
	if (!ilLoadPal(context, FileName) || !ilConvertPal(context, IL_PAL_RGB24)) {
		ifree(NewData);
		context->impl->iCurImage = CurImage;
		return IL_FALSE;
	}

	NumColours = Image.Pal.PalSize / 3;  // RGB24 is 3 bytes per entry.
	PalInfo = (ILuint*)ialloc(context, NumColours * sizeof(ILuint));
	if (PalInfo == NULL) {
		ifree(NewData);
		context->impl->iCurImage = CurImage;
		return IL_FALSE;
	}

	NumPix = CurImage->SizeOfData / ilGetBppFormat(CurImage->Format);
	switch (CurImage->Format)
	{
		case IL_COLOUR_INDEX:
			context->impl->iCurImage = CurImage;
			if (!ilConvertPal(context, IL_PAL_RGB24)) {
				ifree(NewData);
				ifree(PalInfo);
				return IL_FALSE;
			}

			NumPix = context->impl->iCurImage->Pal.PalSize / ilGetBppPal(context->impl->iCurImage->Pal.PalType);
			for (i = 0; i < NumPix; i++) {
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)context->impl->iCurImage->Pal.Palette[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)context->impl->iCurImage->Pal.Palette[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)context->impl->iCurImage->Pal.Palette[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				context->impl->iCurImage->Pal.Palette[i] = DistEntry;
			}

			for (i = 0; i < context->impl->iCurImage->SizeOfData; i++) {
				NewData[i] = context->impl->iCurImage->Pal.Palette[context->impl->iCurImage->Data[i]];
			}
			break;
		case IL_RGB:
		case IL_RGBA:
			/*Cube = (COL_CUBE*)ialloc(context, NumColours * sizeof(COL_CUBE));
			// @TODO:  Check if ialloc failed here!
			for (i = 0; i < NumColours; i++)
				memcpy(&Cubes[i].Val, Image.Pal.Palette[i * 3], 3);
			for (j = 0; j < 3; j++) {
				qsort(Cubes, NumColours, sizeof(COL_CUBE), sort_func);
				Cubes[0].Min = 0;
				Cubes[NumColours-1] = UCHAR_MAX;
				NumColours--;
				for (i = 1; i < NumColours; i++) {
					Cubes[i].Min[CurSort] = Cubes[i-1].Val[CurSort] + 1;
					Cubes[i-1].Max[CurSort] = Cubes[i].Val[CurSort] - 1;
				}
				CurSort++;
				NumColours++;
			}*/
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				Same = IL_TRUE;
				if (i != 0) {
					for (j = 0; j < CurImage->Bpp; j++) {
						if (CurImage->Data[i-CurImage->Bpp+j] != CurImage->Data[i+j]) {
							Same = IL_FALSE;
							break;
						}
					}
				}
				if (Same) {
					NewData[i / CurImage->Bpp] = DistEntry;
					continue;
				}
				for (j = 0; j < Image.Pal.PalSize; j += 3) {
					// No need to perform a sqrt.
					Dist1 = (ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j];
					Dist2 = (ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j+1];
					Dist3 = (ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j+2];
					PalInfo[j / 3] = Dist1 * Dist1 + Dist2 * Dist2 + Dist3 * Dist3;
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_BGR:
		case IL_BGRA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) *
						((ILint)CurImage->Data[i+2] - (ILint)Image.Pal.Palette[j * 3]) + 
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) *
						((ILint)CurImage->Data[i+1] - (ILint)Image.Pal.Palette[j * 3 + 1]) +
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]) *
						((ILint)CurImage->Data[i] - (ILint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i / CurImage->Bpp] = DistEntry;
			}

			break;

		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			for (i = 0; i < CurImage->SizeOfData; i += CurImage->Bpp ) {
				for (j = 0; j < NumColours; j++) {
					// No need to perform a sqrt.
					PalInfo[j] = ((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3]) + 
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 1]) +
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]) *
						((ILuint)CurImage->Data[i] - (ILuint)Image.Pal.Palette[j * 3 + 2]);
				}
				MaxDist = UINT_MAX;
				DistEntry = 0;
				for (j = 0; j < NumColours; j++) {
					if (PalInfo[j] < MaxDist) {
						DistEntry = j;
						MaxDist = PalInfo[j];
					}
				}
				NewData[i] = DistEntry;
			}

			break;

		default:  // Should be no other!
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	context->impl->iCurImage = CurImage;
	Origin = context->impl->iCurImage->Origin;
	if (!ilTexImage(context, context->impl->iCurImage->Width, context->impl->iCurImage->Height, context->impl->iCurImage->Depth, 1,
		IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NewData)) {
		ifree(Image.Pal.Palette);
		ifree(PalInfo);
		ifree(NewData);
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = Origin;

	context->impl->iCurImage->Pal.Palette = Image.Pal.Palette;
	context->impl->iCurImage->Pal.PalSize = Image.Pal.PalSize;
	context->impl->iCurImage->Pal.PalType = Image.Pal.PalType;
	ifree(PalInfo);
	ifree(NewData);

	return IL_TRUE;
}
