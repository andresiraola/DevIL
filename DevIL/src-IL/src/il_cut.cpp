//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 10/10/2006
//
// Filename: src-IL/src/il_cut.cpp
//
// Description: Reads a Dr. Halo .cut file
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_CUT

#include "il_bits.h"
#include "il_cut.h"
#include "il_pal.h"

// Wrap it just in case...
#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif

typedef struct CUT_HEAD
{
	ILushort	Width;
	ILushort	Height;
	ILint		Dummy;
} IL_PACKSTRUCT CUT_HEAD;

#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

CutHandler::CutHandler(ILcontext* context) :
	context(context)
{

}

//! Reads a .cut file
ILboolean CutHandler::load(ILconst_string FileName)
{
	ILHANDLE	CutFile;
	ILboolean	bCut = IL_FALSE;

	CutFile = context->impl->iopenr(FileName);
	if (CutFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bCut;
	}

	bCut = loadF(CutFile);
	context->impl->icloser(CutFile);

	return bCut;
}

//! Reads an already-opened .cut file
ILboolean CutHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a .cut
ILboolean CutHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

//	Note:  .Cut support has not been tested yet!
// A .cut can only have 1 bpp.
//	We need to add support for the .pal's PSP outputs with these...
ILboolean CutHandler::loadInternal()
{
	CUT_HEAD	Header;
	ILuint		Size, i = 0, j;
	ILubyte		Count, Run;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Header.Width = GetLittleShort(context);
	Header.Height = GetLittleShort(context);
	Header.Dummy = GetLittleInt(context);

	if (Header.Width == 0 || Header.Height == 0) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(context, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {  // always 1 bpp
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	Size = Header.Width * Header.Height;

	while (i < Size) {
		Count = context->impl->igetc(context);
		if (Count == 0) { // end of row
			context->impl->igetc(context);  // Not supposed to be here, but
			context->impl->igetc(context);  //  PSP is putting these two bytes here...WHY?!
			continue;
		}
		if (Count & BIT_7) {  // rle-compressed
			ClearBits(Count, BIT_7);
			Run = context->impl->igetc(context);
			for (j = 0; j < Count; j++) {
				context->impl->iCurImage->Data[i++] = Run;
			}
		}
		else {  // run of pixels
			for (j = 0; j < Count; j++) {
				context->impl->iCurImage->Data[i++] = context->impl->igetc(context);
			}
		}
	}

	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;  // Not sure

	/*context->impl->iCurImage->Pal.Palette = SharedPal.Palette;
	context->impl->iCurImage->Pal.PalSize = SharedPal.PalSize;
	context->impl->iCurImage->Pal.PalType = SharedPal.PalType;*/

	return ilFixImage(context);
}

/* ?????????
void ilPopToast() {
	ILstring flipCode = IL_TEXT("#flipCode and www.flipCode.com rule you all.");
	flipCode[0] = flipCode[0];
}
*/

#endif//IL_NO_CUT