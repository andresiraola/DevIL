//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_wbmp.cpp
//
// Description: Reads from a Wireless Bitmap (.wbmp) file.  Specs available from
//				http://www.ibm.com/developerworks/wireless/library/wi-wbmp/
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_WBMP
#include "il_bits.h"


ILboolean	iLoadWbmpInternal(ILcontext* context);
ILuint		WbmpGetMultibyte(ILcontext* context);
ILboolean	iSaveWbmpInternal(ILcontext* context);

// Reads a .wbmp file
ILboolean ilLoadWbmp(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	WbmpFile;
	ILboolean	bWbmp = IL_FALSE;

	WbmpFile = context->impl->iopenr(FileName);
	if (WbmpFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bWbmp;
	}

	iSetInputFile(context, WbmpFile);

	bWbmp = ilLoadWbmpF(context, WbmpFile);

	context->impl->icloser(WbmpFile);

	return bWbmp;
}


//! Reads an already-opened .wbmp file
ILboolean ilLoadWbmpF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadWbmpInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .wbmp
ILboolean ilLoadWbmpL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadWbmpInternal(context);
}


ILboolean iLoadWbmpInternal(ILcontext* context)
{
	ILuint	Width, Height, BitPadding, i;
	BITFILE	*File;
	ILubyte	Padding[8];

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (context->impl->igetc(context) != 0 || context->impl->igetc(context) != 0) {  // The first two bytes have to be 0 (the "header")
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	Width = WbmpGetMultibyte(context);  // Next follows the width and height.
	Height = WbmpGetMultibyte(context);

	if (Width == 0 || Height == 0) {  // Must have at least some height and width.
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilTexImage(context, Width, Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;  // Always has origin in the upper left.

	BitPadding = (8 - (Width % 8)) % 8;  // Has to be aligned on a byte boundary.  The rest is padding.
	File = bfile(context, iGetFile(context));
	if (File == NULL)
		return IL_FALSE;  //@TODO: Error?

	//@TODO: Do this without bread?  Would be faster, since we would not have to do
	//  the second loop.

	// Reads the bits
	for (i = 0; i < context->impl->iCurImage->Height; i++) {
		bread(context, &context->impl->iCurImage->Data[context->impl->iCurImage->Width * i], 1, context->impl->iCurImage->Width, File);
		//bseek(File, BitPadding, IL_SEEK_CUR);  //@TODO: This function does not work correctly.
		bread(context, Padding, 1, BitPadding, File);  // Skip padding bits.
	}
	// Converts bit value of 1 to white and leaves 0 at 0 (2-colour images only).
	for (i = 0; i < context->impl->iCurImage->SizeOfData; i++) {
		if (context->impl->iCurImage->Data[i] == 1)
			context->impl->iCurImage->Data[i] = 0xFF;  // White
	}

	bclose(File);

	return IL_TRUE;
}


ILuint WbmpGetMultibyte(ILcontext* context)
{
	ILuint Val = 0, i;
	ILubyte Cur;

	for (i = 0; i < 5; i++) {  // Should not be more than 5 bytes.
		Cur = context->impl->igetc(context);
		Val = (Val << 7) | (Cur & 0x7F);  // Drop the MSB of Cur.
		if (!(Cur & 0x80)) {  // Check the MSB and break if 0.
			break;
		}
	}

	return Val;
}


ILboolean WbmpPutMultibyte(ILcontext* context, ILuint Val)
{
	ILint	i, NumBytes = 0;
	ILuint	MultiVal = Val;

	do {
		MultiVal >>= 7;
		NumBytes++;
	} while (MultiVal != 0);

	for (i = NumBytes - 1; i >= 0; i--) {
		MultiVal = (Val >> (i * 7)) & 0x7F;
		if (i != 0)
			MultiVal |= 0x80;
		context->impl->iputc(context, MultiVal);
	}

	return IL_TRUE;
}


//! Writes a Wbmp file
ILboolean ilSaveWbmp(ILcontext* context, const ILstring FileName)
{
	ILHANDLE	WbmpFile;
	ILuint		WbmpSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	WbmpFile = context->impl->iopenw(FileName);
	if (WbmpFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	WbmpSize = ilSaveWbmpF(context, WbmpFile);
	context->impl->iclosew(WbmpFile);

	if (WbmpSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Writes a .wbmp to an already-opened file
ILuint ilSaveWbmpF(ILcontext* context, ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (iSaveWbmpInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


//! Writes a .wbmp to a memory "lump"
ILuint ilSaveWbmpL(ILcontext* context, void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (iSaveWbmpInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


// In il_quantizer.c
ILimage *iQuantizeImage(ILcontext* context, ILimage *Image, ILuint NumCols);
// In il_neuquant.c
ILimage *iNeuQuant(ILcontext* context, ILimage *Image, ILuint NumCols);


// Internal function used to save the Wbmp.
ILboolean iSaveWbmpInternal(ILcontext* context)
{
	ILimage	*TempImage = NULL;
	ILuint	i, j;
	ILint	k;
	ILubyte	Val;
	ILubyte	*TempData;

	context->impl->iputc(context, 0);  // First two header values
	context->impl->iputc(context, 0);  //  must be 0.

	WbmpPutMultibyte(context, context->impl->iCurImage->Width);  // Write the width
	WbmpPutMultibyte(context, context->impl->iCurImage->Height); //  and the height.

	//TempImage = iConvertImage(context->impl->iCurImage, IL_LUMINANCE, IL_UNSIGNED_BYTE);
	if (iGetInt(context, IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
		TempImage = iNeuQuant(context, context->impl->iCurImage, 2);
	else // Assume IL_WU_QUANT otherwise.
		TempImage = iQuantizeImage(context, context->impl->iCurImage, 2);

	if (TempImage == NULL)
		return IL_FALSE;

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	} else {
		TempData = TempImage->Data;
	}

	for (i = 0; i < TempImage->Height; i++) {
		for (j = 0; j < TempImage->Width; j += 8) {
			Val = 0;
			for (k = 0; k < 8; k++) {
				if (j + k < TempImage->Width) {
					//Val |= ((TempData[TempImage->Width * i + j + k] > 0x7F) ? (0x80 >> k) : 0x00);
					Val |= ((TempData[TempImage->Width * i + j + k] == 1) ? (0x80 >> k) : 0x00);
				}
			}
			context->impl->iputc(context, Val);
		}
	}

	if (TempData != TempImage->Data)
		ifree(TempData);
	ilCloseImage(TempImage);

	return IL_TRUE;
}

#endif//IL_NO_WBMP

