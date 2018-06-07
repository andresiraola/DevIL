//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 10/10/2006
//
// Filename: src-IL/src/il_header.cpp
//
// Description: Generates a C-style header file for the current image.
//
//-----------------------------------------------------------------------------

#ifndef IL_NO_CHEAD

#include "il_internal.h"

#include "il_header.h"

// Just a guess...let's see what's purty!
#define MAX_LINE_WIDTH 14

CHeaderHandler::CHeaderHandler(ILcontext* context, char* internalName) :
	context(context), internalName(internalName)
{

}

//! Generates a C-style header file for the current image.
ILboolean CHeaderHandler::save(ILconst_string FileName)
{
	FILE		*HeadFile;
	ILuint		i = 0, j;
	ILimage		*TempImage;
	const char	*Name;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Name = iGetString(context, IL_CHEAD_HEADER_STRING);
	if (Name == NULL)
		Name = internalName;

	if (FileName == NULL || Name == NULL ||
		ilStrLen(FileName) < 1 || ilCharStrLen(Name) < 1) {
		ilSetError(context, IL_INVALID_VALUE);
		return IL_FALSE;
	}

	if (!iCheckExtension(FileName, IL_TEXT("h"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
            return IL_FALSE;
		}
	}

	if (context->impl->iCurImage->Bpc > 1) {
		TempImage = iConvertImage(context, context->impl->iCurImage, context->impl->iCurImage->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
           return IL_FALSE;
	} else {
		TempImage = context->impl->iCurImage;
	}

#ifndef _UNICODE
	HeadFile = fopen(FileName, "wb");
#else
	#ifdef _WIN32
		HeadFile = _wfopen(FileName, L"rb");
	#else
		HeadFile = fopen((char*)FileName, "rb");
	#endif

#endif//_UNICODE

	if (HeadFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
        return IL_FALSE;
	}

	fprintf(HeadFile, "//#include <il/il.h>\n");
	fprintf(HeadFile, "// C Image Header:\n\n\n");
	fprintf(HeadFile, "// IMAGE_BPP is in bytes per pixel, *not* bits\n");
    fprintf(HeadFile, "#define IMAGE_BPP %d\n",context->impl->iCurImage->Bpp);
	fprintf(HeadFile, "#define IMAGE_WIDTH   %d\n", context->impl->iCurImage->Width);
	fprintf(HeadFile, "#define IMAGE_HEIGHT  %d\n", context->impl->iCurImage->Height);	
	fprintf(HeadFile, "#define IMAGE_DEPTH   %d\n\n\n", context->impl->iCurImage->Depth);
	fprintf(HeadFile, "#define IMAGE_TYPE    0x%X\n", context->impl->iCurImage->Type);
	fprintf(HeadFile, "#define IMAGE_FORMAT  0x%X\n\n\n", context->impl->iCurImage->Format);
    fprintf(HeadFile, "ILubyte %s[] = {\n", Name);
        

	for (; i < TempImage->SizeOfData; i += MAX_LINE_WIDTH) {
		fprintf(HeadFile, "\t");
		for (j = 0; j < MAX_LINE_WIDTH; j++) {
			if (i + j >= TempImage->SizeOfData - 1) {
				fprintf(HeadFile, "%4d", TempImage->Data[i+j]);
				break;
			}
			else
				fprintf(HeadFile, "%4d,", TempImage->Data[i+j]);
		}
		fprintf(HeadFile, "\n");
	}
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	fprintf(HeadFile, "};\n");


	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		fprintf(HeadFile, "\n\n");
		fprintf(HeadFile, "#define IMAGE_PALSIZE %u\n\n", context->impl->iCurImage->Pal.PalSize);
		fprintf(HeadFile, "#define IMAGE_PALTYPE 0x%X\n\n", context->impl->iCurImage->Pal.PalType);
        fprintf(HeadFile, "ILubyte %sPal[] = {\n", Name);
		for (i = 0; i < context->impl->iCurImage->Pal.PalSize; i += MAX_LINE_WIDTH) {
			fprintf(HeadFile, "\t");
			for (j = 0; j < MAX_LINE_WIDTH; j++) {
				if (i + j >= context->impl->iCurImage->Pal.PalSize - 1) {
					fprintf(HeadFile, " %4d", context->impl->iCurImage->Pal.Palette[i+j]);
					break;
				}
				else
					fprintf(HeadFile, " %4d,", context->impl->iCurImage->Pal.Palette[i+j]);
			}
			fprintf(HeadFile, "\n");
		}

		fprintf(HeadFile, "};\n");
	}
	fclose(HeadFile);
	return IL_TRUE;
}

#endif//IL_NO_CHEAD