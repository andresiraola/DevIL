//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/09/2009
//
// Filename: src-IL/src/il_bmp.cpp
//
// Description: Reads from and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------

#define IL_BMP_C

#include "il_internal.h"

#ifndef IL_NO_BMP

#include "il_bmp.h"
#include "il_endian.h"

#include <stdio.h>

#ifndef NOINLINE
INLINE void GetShiftFromMask(const ILuint Mask, ILuint * CONST_RESTRICT ShiftLeft, ILuint * CONST_RESTRICT ShiftRight) {
	ILuint Temp, i;

	if (Mask == 0) {
		*ShiftLeft = *ShiftRight = 0;
		return;
	}

	Temp = Mask;
	for (i = 0; i < 32; i++, Temp >>= 1) {
		if (Temp & 1)
			break;
	}
	*ShiftRight = i;

	// Temp is preserved, so use it again:
	for (i = 0; i < 8; i++, Temp >>= 1) {
		if (!(Temp & 1))
			break;
	}
	*ShiftLeft = 8 - i;

	return;
}
#endif

BmpHandler::BmpHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid .bmp file.
ILboolean BmpHandler::isValid(ILconst_string CONST_RESTRICT FileName)
{
	ILHANDLE	BitmapFile;
	ILboolean	bBitmap = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("bmp"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bBitmap;
	}

	BitmapFile = context->impl->iopenr(FileName);
	if (BitmapFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bBitmap;
	}

	bBitmap = isValidF(BitmapFile);
	context->impl->icloser(BitmapFile);

	return bBitmap;
}

//! Checks if the ILHANDLE contains a valid .bmp file at the current position.
ILboolean BmpHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid .bmp lump.
ILboolean BmpHandler::isValidL(const void * Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the .bmp header from the current file.
ILboolean iGetBmpHead(ILcontext* context, BMPHEAD * const Header)
{
	Header->bfType = GetLittleUShort(context);
	Header->bfSize = GetLittleInt(context);
	Header->bfReserved = GetLittleUInt(context);
	Header->bfDataOff = GetLittleInt(context);
	Header->biSize = GetLittleInt(context);
	Header->biWidth = GetLittleInt(context);
	Header->biHeight = GetLittleInt(context);
	Header->biPlanes = GetLittleShort(context);
	Header->biBitCount = GetLittleShort(context);
	Header->biCompression = GetLittleInt(context);
	Header->biSizeImage = GetLittleInt(context);
	Header->biXPelsPerMeter = GetLittleInt(context);
	Header->biYPelsPerMeter = GetLittleInt(context);
	Header->biClrUsed = GetLittleInt(context);
	Header->biClrImportant = GetLittleInt(context);
	return IL_TRUE;
}

ILboolean iGetOS2Head(ILcontext* context, OS2_HEAD * const Header)
{
	if (context->impl->iread(context, Header, sizeof(OS2_HEAD), 1) != 1)
		return IL_FALSE;

	UShort(&Header->bfType);
	UInt(&Header->biSize);
	Short(&Header->xHotspot);
	Short(&Header->yHotspot);
	UInt(&Header->DataOff);
	UInt(&Header->cbFix);

	//2003-09-01 changed to UShort according to MSDN
	UShort(&Header->cx);
	UShort(&Header->cy);
	UShort(&Header->cPlanes);
	UShort(&Header->cBitCount);

	context->impl->iseek(context, (ILint)Header->cbFix - 12, IL_SEEK_CUR);  // Skip rest of header, if any.

	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean BmpHandler::isValidInternal()
{
	BMPHEAD		Head;
	OS2_HEAD	Os2Head;
	ILboolean	IsValid;

	iGetBmpHead(context, &Head);
	context->impl->iseek(context, -(ILint)sizeof(BMPHEAD), IL_SEEK_CUR);  // Go ahead and restore to previous state

	IsValid = check(&Head);
	if (!IsValid) {
		iGetOS2Head(context, &Os2Head);
		context->impl->iseek(context, -(ILint)sizeof(BMPHEAD), IL_SEEK_CUR);
		IsValid = iCheckOS2(&Os2Head);
	}
	return IsValid;
}

// Internal function used to check if the HEADER is a valid .bmp header.
ILboolean BmpHandler::check(const BMPHEAD * CONST_RESTRICT Header)
{
	//if ((Header->bfType != ('B'|('M'<<8))) || ((Header->biSize != 0x28) && (Header->biSize != 0x0C)))
	if ((Header->bfType != ('B' | ('M' << 8))) || (Header->biSize != 0x28))
		return IL_FALSE;
	if (Header->biHeight == 0 || Header->biWidth < 1)
		return IL_FALSE;
	if (Header->biPlanes > 1)  // Not sure...
		return IL_FALSE;
	if (Header->biCompression != 0 && Header->biCompression != 1 &&
		Header->biCompression != 2 && Header->biCompression != 3)
		return IL_FALSE;
	if (Header->biCompression == 3 && Header->biBitCount != 16 && Header->biBitCount != 32)
		return IL_FALSE;
	if (Header->biBitCount != 1 && Header->biBitCount != 4 && Header->biBitCount != 8 &&
		Header->biBitCount != 16 && Header->biBitCount != 24 && Header->biBitCount != 32)
		return IL_FALSE;
	return IL_TRUE;
}

ILboolean iCheckOS2(const OS2_HEAD * CONST_RESTRICT Header)
{
	if ((Header->bfType != ('B' | ('M' << 8))) || (Header->DataOff < 26) || (Header->cbFix < 12))
		return IL_FALSE;
	if (Header->cPlanes != 1)
		return IL_FALSE;
	if (Header->cx == 0 || Header->cy == 0)
		return IL_FALSE;
	if (Header->cBitCount != 1 && Header->cBitCount != 4 && Header->cBitCount != 8 &&
		Header->cBitCount != 24)
		return IL_FALSE;

	return IL_TRUE;
}

//! Reads a .bmp file
ILboolean BmpHandler::load(ILconst_string FileName) {
	ILHANDLE	BitmapFile;
	ILboolean	bBitmap = IL_FALSE;

	BitmapFile = context->impl->iopenr(FileName);
	if (BitmapFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bBitmap;
	}

	bBitmap = loadF(BitmapFile);
	context->impl->icloser(BitmapFile);

	return bBitmap;
}

//! Reads an already-opened .bmp file
ILboolean BmpHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a .bmp
ILboolean BmpHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the .bmp.
ILboolean BmpHandler::loadInternal()
{
	BMPHEAD		Header;
	OS2_HEAD	Os2Head;
	ILboolean	bBitmap;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iGetBmpHead(context, &Header);
	if (!check(&Header)) {
		context->impl->iseek(context, -(ILint)sizeof(BMPHEAD), IL_SEEK_CUR);
		iGetOS2Head(context, &Os2Head);
		if (!iCheckOS2(&Os2Head)) {
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return IL_FALSE;
		}
		else {
			return iGetOS2Bmp(context, &Os2Head);
		}
	}

	// Don't know what to do if it has more than one plane...
	if (Header.biPlanes != 1) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	switch (Header.biCompression)
	{
	case 0:	// No compression
	case 3:	// BITFIELDS compression is handled in 16 bit
					// and 32 bit code in ilReadUncompBmp()
		bBitmap = ilReadUncompBmp(context, &Header);
		break;
	case 1:  //	RLE 8-bit / pixel (BI_RLE8)
		bBitmap = ilReadRLE8Bmp(context, &Header);
		break;
	case 2:  // RLE 4-bit / pixel (BI_RLE4)
		bBitmap = ilReadRLE4Bmp(context, &Header);
		break;

	default:
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ilFixImage(context))
		return IL_FALSE;

	return bBitmap;
}

// Reads an uncompressed .bmp
//	One of the absolute ugliest functions I've ever written!
ILboolean ilReadUncompBmp(ILcontext* context, BMPHEAD * Header)
{
	ILuint i, j, k, c, Read;
	ILubyte Bpp, ByteData, PadSize, Padding[4];
	ILuint rMask, gMask, bMask; //required for bitfields packing
	ILuint rShiftR, gShiftR, bShiftR; //required for bitfields packing
	ILuint rShiftL, gShiftL, bShiftL; //required for bitfields packing
	ILushort Read16; //used for 16bit bmp loading

	if (Header->biBitCount < 8)
		Bpp = 1;  // We can't have an integral number less than one and greater than 0
	else
		Bpp = (ILubyte)(Header->biBitCount >> 3);  // Convert to bytes per pixel

	if (Bpp == 2 || Bpp == 4)
		Bpp = 3;

	// Update the current image with the new dimensions
	if (!ilTexImage(context, Header->biWidth, abs(Header->biHeight), 1, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	switch (Header->biBitCount)
	{
	case 1:
		//context->impl->iCurImage->Format = IL_LUMINANCE;
		context->impl->iCurImage->Format = IL_COLOUR_INDEX;
		context->impl->iCurImage->Pal.PalType = IL_PAL_BGR32;
		context->impl->iCurImage->Pal.PalSize = 2 * 4;
		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
		if (context->impl->iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		break;

	case 4:
	case 8:
		context->impl->iCurImage->Format = IL_COLOUR_INDEX;
		context->impl->iCurImage->Pal.PalType = IL_PAL_BGR32;

		// if there are 256 colors biClrUsed is 0
		context->impl->iCurImage->Pal.PalSize = Header->biClrUsed ?
			Header->biClrUsed * 4 : 256 * 4;

		if (Header->biBitCount == 4)  // biClrUsed is 0 for 4-bit bitmaps
			context->impl->iCurImage->Pal.PalSize = 16 * 4;
		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
		if (context->impl->iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		break;

	case 16:
	case 24:
	case 32:
		context->impl->iCurImage->Format = IL_BGR;
		break;

	default:
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	// A height of 0 is illegal
	if (Header->biHeight == 0) {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		if (context->impl->iCurImage->Pal.Palette)
			ifree(context->impl->iCurImage->Pal.Palette);
		return IL_FALSE;
	}

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	if (Header->biHeight < 0) {
		context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	}
	else {
		context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
	}

	// Read the palette
	context->impl->iseek(context, sizeof(BMPHEAD), IL_SEEK_SET);
	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, context->impl->iCurImage->Pal.PalSize) != context->impl->iCurImage->Pal.PalSize)
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	context->impl->iseek(context, Header->bfDataOff, IL_SEEK_SET);

	// We have to handle 1 and 4-bit images separately, because we have to expand them.
	switch (Header->biBitCount)
	{
	case 1:
		//changed 2003-09-01
		if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(context, context->impl->iCurImage->Width / 8 * context->impl->iCurImage->Height);

		PadSize = ((32 - (context->impl->iCurImage->Width % 32)) / 8) % 4;  // Has to truncate
		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			Read = 0;
			for (i = 0; i < context->impl->iCurImage->Width; ) {
				if (context->impl->iread(context, &ByteData, 1, 1) != 1) {
					iUnCache(context);
					return IL_FALSE;
				}
				Read++;
				k = 128;
				for (c = 0; c < 8; c++) {
					context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] =
						(!!(ByteData & k) == 1 ? 1 : 0);
					k >>= 1;
					if (++i >= context->impl->iCurImage->Width)
						break;
				}
			}
			//context->impl->iseek(context, PadSize, IL_SEEK_CUR);
			context->impl->iread(context, Padding, 1, PadSize);
		}

		iUnCache(context);
		break;

	case 4:
		//changed 2003-09-01
		if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(context, context->impl->iCurImage->Width / 2 * context->impl->iCurImage->Height);

		PadSize = ((8 - (context->impl->iCurImage->Width % 8)) / 2) % 4;  // Has to truncate
		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			for (i = 0; i < context->impl->iCurImage->Width; i++) {
				if (context->impl->iread(context, &ByteData, 1, 1) != 1) {
					iUnCache(context);
					return IL_FALSE;
				}
				context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] = ByteData >> 4;
				if (++i == context->impl->iCurImage->Width)
					break;
				context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] = ByteData & 0x0F;
			}
			context->impl->iread(context, Padding, 1, PadSize);//context->impl->iseek(context, PadSize, IL_SEEK_CUR);
		}

		iUnCache(context);
		break;

	case 16:
		//padding
		//2003-09-09: changed context->impl->iCurImage->Bps to context->impl->iCurImage->Width*2,
		//because context->impl->iCurImage->Bps refers to the 24 bit devil image
		PadSize = (4 - (context->impl->iCurImage->Width * 2 % 4)) % 4;

		//check if bitfield compression is used
		rMask = 0x7C00;
		gMask = 0x03E0;
		bMask = 0x001F;
		rShiftR = 10;
		gShiftR = 5;
		bShiftR = 0;
		rShiftL = 3;
		gShiftL = 3;
		bShiftL = 3;
		if (Header->biCompression == 3) //bitfields
		{
			context->impl->iseek(context, Header->bfDataOff - 12, IL_SEEK_SET); //seek to bitfield data
			context->impl->iread(context, &rMask, 4, 1);
			context->impl->iread(context, &gMask, 4, 1);
			context->impl->iread(context, &bMask, 4, 1);
			UInt(&rMask);
			UInt(&gMask);
			UInt(&bMask);
			GetShiftFromMask(rMask, &rShiftL, &rShiftR);
			GetShiftFromMask(gMask, &gShiftL, &gShiftR);
			GetShiftFromMask(bMask, &bShiftL, &bShiftR);
		}

		k = 0;

		//changed 2003-09-01
		if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(context, context->impl->iCurImage->Width * context->impl->iCurImage->Height);

		//@TODO: This may not be safe for Big Endian.
		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			for (i = 0; i < context->impl->iCurImage->Width; i++, k += 3) {
				if (context->impl->iread(context, &Read16, 2, 1) != 1) {
					iUnCache(context);
					return IL_FALSE;
				}
				context->impl->iCurImage->Data[k] = ((Read16 & bMask) >> bShiftR) << bShiftL;
				context->impl->iCurImage->Data[k + 1] = ((Read16 & gMask) >> gShiftR) << gShiftL;
				context->impl->iCurImage->Data[k + 2] = ((Read16 & rMask) >> rShiftR) << rShiftL;
			}
			context->impl->iread(context, Padding, 1, PadSize);
		}

		iUnCache(context);
		break;

	case 8:
	case 24:
		// For 8 and 24 bit, Bps is equal to the bmps bps
		PadSize = (4 - (context->impl->iCurImage->Bps % 4)) % 4;
		if (PadSize == 0) {
			if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfPlane) != context->impl->iCurImage->SizeOfPlane)
				return IL_FALSE;
		}
		else {	// Microsoft requires lines to be padded if the widths aren't multiples of 4.
			//changed 2003-09-01
			if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
				iPreCache(context, context->impl->iCurImage->Width * context->impl->iCurImage->Height);

			PadSize = (4 - (context->impl->iCurImage->Bps % 4));
			for (i = 0; i < context->impl->iCurImage->SizeOfPlane; i += context->impl->iCurImage->Bps) {
				if (context->impl->iread(context, context->impl->iCurImage->Data + i, 1, context->impl->iCurImage->Bps) != context->impl->iCurImage->Bps) {
					iUnCache(context);
					return IL_FALSE;
				}
				//context->impl->iseek(context, PadSize, IL_SEEK_CUR);
				context->impl->iread(context, Padding, 1, PadSize);
			}

			iUnCache(context);
		}
		break;

	case 32:
		//32bit files are always padded to 4 byte...
		//check if bitfield compression is used
		rMask = 0xFF0000;
		gMask = 0x00FF00;
		bMask = 0x0000FF;
		rShiftR = 16;
		gShiftR = 8;
		bShiftR = 0;
		rShiftL = 0;
		gShiftL = 0;
		bShiftL = 0;
		if (Header->biCompression == 3) //bitfields
		{
			context->impl->iseek(context, Header->bfDataOff - 12, IL_SEEK_SET); //seek to bitfield data
			context->impl->iread(context, &rMask, 4, 1);
			context->impl->iread(context, &gMask, 4, 1);
			context->impl->iread(context, &bMask, 4, 1);
			UInt(&rMask);
			UInt(&gMask);
			UInt(&bMask);
			GetShiftFromMask(rMask, &rShiftL, &rShiftR);
			GetShiftFromMask(gMask, &gShiftL, &gShiftR);
			GetShiftFromMask(bMask, &bShiftL, &bShiftR);
		}

		//TODO: win98 supports per-pixel alpha, so
		//load to rgba????

		//changed 2003-09-01
		if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(context, context->impl->iCurImage->Width * context->impl->iCurImage->Height);

		for (i = 0; i < context->impl->iCurImage->SizeOfData; i += 3) {
			if (context->impl->iread(context, &Read, 4, 1) != 1) {
				iUnCache(context);
				return IL_FALSE;
			}

			context->impl->iCurImage->Data[i] = ((Read & bMask) >> bShiftR) << bShiftL;
			context->impl->iCurImage->Data[i + 1] = ((Read & gMask) >> gShiftR) << gShiftL;
			context->impl->iCurImage->Data[i + 2] = ((Read & rMask) >> rShiftR) << rShiftL;
		}

		iUnCache(context);
		break;

	default:
		return IL_FALSE; //shouldn't happen, we checked that before
	}

	return IL_TRUE;
}

ILboolean ilReadRLE8Bmp(ILcontext* context, BMPHEAD *Header)
{
	ILubyte	Bytes[2];
	size_t	offset = 0, count, endOfLine = Header->biWidth;

	// Update the current image with the new dimensions
	if (!ilTexImage(context, Header->biWidth, abs(Header->biHeight), 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;

	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	// A height of 0 is illegal
	if (Header->biHeight == 0)
		return IL_FALSE;

	context->impl->iCurImage->Format = IL_COLOUR_INDEX;
	context->impl->iCurImage->Pal.PalType = IL_PAL_BGR32;
	context->impl->iCurImage->Pal.PalSize = Header->biClrUsed * 4;  // 256 * 4 for most images
	if (context->impl->iCurImage->Pal.PalSize == 0)
		context->impl->iCurImage->Pal.PalSize = 256 * 4;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (context->impl->iCurImage->Pal.Palette == NULL)
		return IL_FALSE;

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	context->impl->iCurImage->Origin = Header->biHeight < 0 ?
		IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

	// Read the palette
	context->impl->iseek(context, sizeof(BMPHEAD), IL_SEEK_SET);
	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, context->impl->iCurImage->Pal.PalSize, 1) != 1)
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	context->impl->iseek(context, Header->bfDataOff, IL_SEEK_SET);

	while (offset < context->impl->iCurImage->SizeOfData) {
		if (context->impl->iread(context, Bytes, sizeof(Bytes), 1) != 1)
			return IL_FALSE;
		if (Bytes[0] == 0x00) {  // Escape sequence
			switch (Bytes[1])
			{
			case 0x00:  // End of line
				offset = endOfLine;
				endOfLine += context->impl->iCurImage->Width;
				break;
			case 0x01:  // End of bitmap
				offset = context->impl->iCurImage->SizeOfData;
				break;
			case 0x2:
				if (context->impl->iread(context, Bytes, sizeof(Bytes), 1) != 1)
					return IL_FALSE;
				offset += Bytes[0] + Bytes[1] * context->impl->iCurImage->Width;
				endOfLine += Bytes[1] * context->impl->iCurImage->Width;
				break;
			default:
				count = IL_MIN(Bytes[1], context->impl->iCurImage->SizeOfData - offset);
				if (context->impl->iread(context, context->impl->iCurImage->Data + offset, (ILuint)count, 1) != 1)
					return IL_FALSE;
				offset += count;
				if ((count & 1) == 1)  // Must be on a word boundary
					if (context->impl->iread(context, Bytes, 1, 1) != 1)
						return IL_FALSE;
				break;
			}
		}
		else {
			count = IL_MIN(Bytes[0], context->impl->iCurImage->SizeOfData - offset);
			memset(context->impl->iCurImage->Data + offset, Bytes[1], count);
			offset += count;
		}
	}
	return IL_TRUE;
}

//changed 2003-09-01
//deleted ilReadRLE8Bmp() USE_POINTER version

ILboolean ilReadRLE4Bmp(ILcontext* context, BMPHEAD *Header)
{
	ILubyte	Bytes[2];
	ILuint	i;
	size_t	offset = 0, count, endOfLine = Header->biWidth;

	// Update the current image with the new dimensions
	if (!ilTexImage(context, Header->biWidth, abs(Header->biHeight), 1, 1, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	// A height of 0 is illegal
	if (Header->biHeight == 0) {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	context->impl->iCurImage->Format = IL_COLOUR_INDEX;
	context->impl->iCurImage->Pal.PalType = IL_PAL_BGR32;
	context->impl->iCurImage->Pal.PalSize = 16 * 4; //Header->biClrUsed * 4;  // 16 * 4 for most images
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (context->impl->iCurImage->Pal.Palette == NULL)
		return IL_FALSE;

	// If the image height is negative, then the image is flipped
	//	(Normal is IL_ORIGIN_LOWER_LEFT)
	context->impl->iCurImage->Origin = Header->biHeight < 0 ?
		IL_ORIGIN_UPPER_LEFT : IL_ORIGIN_LOWER_LEFT;

	// Read the palette
	context->impl->iseek(context, sizeof(BMPHEAD), IL_SEEK_SET);

	if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, context->impl->iCurImage->Pal.PalSize, 1) != 1)
		return IL_FALSE;

	// Seek to the data from the "beginning" of the file
	context->impl->iseek(context, Header->bfDataOff, IL_SEEK_SET);

	while (offset < context->impl->iCurImage->SizeOfData) {
		int align;
		if (context->impl->iread(context, &Bytes[0], sizeof(Bytes), 1) != 1)
			return IL_FALSE;
		if (Bytes[0] == 0x0) {				// Escape sequence
			switch (Bytes[1]) {
			case 0x0:	// End of line
				offset = endOfLine;
				endOfLine += context->impl->iCurImage->Width;
				break;
			case 0x1:	// End of bitmap
				offset = context->impl->iCurImage->SizeOfData;
				break;
			case 0x2:
				if (context->impl->iread(context, &Bytes[0], sizeof(Bytes), 1) != 1)
					return IL_FALSE;
				offset += Bytes[0] + Bytes[1] * context->impl->iCurImage->Width;
				endOfLine += Bytes[1] * context->impl->iCurImage->Width;
				break;
			default:	  // Run of pixels
				count = IL_MIN(Bytes[1], context->impl->iCurImage->SizeOfData - offset);

				for (i = 0; i < count; i++) {
					int byte;

					if ((i & 0x01) == 0) {
						if (context->impl->iread(context, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
							return IL_FALSE;
						byte = (Bytes[0] >> 4);
					}
					else
						byte = (Bytes[0] & 0x0F);
					context->impl->iCurImage->Data[offset++] = byte;
				}

				align = Bytes[1] % 4;

				if (align == 1 || align == 2)	// Must be on a word boundary
					if (context->impl->iread(context, &Bytes[0], sizeof(Bytes[0]), 1) != 1)
						return IL_FALSE;
			}
		}
		else {
			count = IL_MIN(Bytes[0], context->impl->iCurImage->SizeOfData - offset);
			Bytes[0] = (Bytes[1] >> 4);
			Bytes[1] &= 0x0F;
			for (i = 0; i < count; i++)
				context->impl->iCurImage->Data[offset++] = Bytes[i & 1];
		}
	}

	return IL_TRUE;
}

//changed 2003-09-01
//deleted ilReadRLE4Bmp() USE_POINTER version

ILboolean iGetOS2Bmp(ILcontext* context, OS2_HEAD *Header)
{
	ILuint	PadSize, Read, i, j, k, c;
	ILubyte	ByteData;

	if (Header->cBitCount == 1) {
		if (!ilTexImage(context, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
		context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 2 * 3);
		if (context->impl->iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		context->impl->iCurImage->Pal.PalSize = 2 * 3;
		context->impl->iCurImage->Pal.PalType = IL_PAL_BGR24;

		if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 2 * 3) != 6)
			return IL_FALSE;

		PadSize = ((32 - (context->impl->iCurImage->Width % 32)) / 8) % 4;  // Has to truncate.
		context->impl->iseek(context, Header->DataOff, IL_SEEK_SET);

		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			Read = 0;
			for (i = 0; i < context->impl->iCurImage->Width; ) {
				if (context->impl->iread(context, &ByteData, 1, 1) != 1)
					return IL_FALSE;
				Read++;
				k = 128;
				for (c = 0; c < 8; c++) {
					context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] =
						(!!(ByteData & k) == 1 ? 1 : 0);
					k >>= 1;
					if (++i >= context->impl->iCurImage->Width)
						break;
				}
			}
			context->impl->iseek(context, PadSize, IL_SEEK_CUR);
		}
		return IL_TRUE;
	}

	if (Header->cBitCount == 4) {
		if (!ilTexImage(context, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
		context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 16 * 3);
		if (context->impl->iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		context->impl->iCurImage->Pal.PalSize = 16 * 3;
		context->impl->iCurImage->Pal.PalType = IL_PAL_BGR24;

		if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 16 * 3) != 16 * 3)
			return IL_FALSE;

		PadSize = ((8 - (context->impl->iCurImage->Width % 8)) / 2) % 4;  // Has to truncate
		context->impl->iseek(context, Header->DataOff, IL_SEEK_SET);

		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			for (i = 0; i < context->impl->iCurImage->Width; i++) {
				if (context->impl->iread(context, &ByteData, 1, 1) != 1)
					return IL_FALSE;
				context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] = ByteData >> 4;
				if (++i == context->impl->iCurImage->Width)
					break;
				context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i] = ByteData & 0x0F;
			}
			context->impl->iseek(context, PadSize, IL_SEEK_CUR);
		}

		return IL_TRUE;
	}

	if (Header->cBitCount == 8) {
		//added this line 2003-09-01...strange no-one noticed before...
		if (!ilTexImage(context, Header->cx, Header->cy, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;

		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 256 * 3);
		if (context->impl->iCurImage->Pal.Palette == NULL) {
			return IL_FALSE;
		}
		context->impl->iCurImage->Pal.PalSize = 256 * 3;
		context->impl->iCurImage->Pal.PalType = IL_PAL_BGR24;

		if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 256 * 3) != 256 * 3)
			return IL_FALSE;
	}
	else { //has to be 24 bpp
		if (!ilTexImage(context, Header->cx, Header->cy, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
			return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	context->impl->iseek(context, Header->DataOff, IL_SEEK_SET);

	PadSize = (4 - (context->impl->iCurImage->Bps % 4)) % 4;
	if (PadSize == 0) {
		if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData)
			return IL_FALSE;
	}
	else {
		for (i = 0; i < context->impl->iCurImage->Height; i++) {
			if (context->impl->iread(context, context->impl->iCurImage->Data + i * context->impl->iCurImage->Bps, 1, context->impl->iCurImage->Bps) != context->impl->iCurImage->Bps)
				return IL_FALSE;
			context->impl->iseek(context, PadSize, IL_SEEK_CUR);
		}
	}

	return IL_TRUE;
}

//! Writes a Bmp file
ILboolean BmpHandler::save(const ILstring FileName)
{
	ILHANDLE	BitmapFile;
	ILuint		BitmapSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	BitmapFile = context->impl->iopenw(FileName);
	if (BitmapFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	BitmapSize = saveF(BitmapFile);
	context->impl->iclosew(BitmapFile);

	if (BitmapSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a Bmp to an already-opened file
ILuint BmpHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a Bmp to a memory "lump"
ILuint BmpHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Internal function used to save the .bmp.
ILboolean BmpHandler::saveInternal()
{
	//int compress_rle8 = ilGetInteger(context, IL_BMP_RLE);
	int compress_rle8 = IL_FALSE; // disabled BMP RLE compression. broken
	ILuint	FileSize, i, PadSize, Padding = 0;
	ILimage	*TempImage = NULL;
	ILpal	*TempPal;
	ILubyte	*TempData;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	context->impl->iputc(context, 'B');  // Comprises the
	context->impl->iputc(context, 'M');  //  "signature"

	SaveLittleUInt(context, 0);  // Will come back and change later in this function (filesize)
	SaveLittleUInt(context, 0);  // Reserved

	if (compress_rle8 == IL_TRUE)
	{
		TempImage = iConvertImage(context, context->impl->iCurImage, IL_COLOR_INDEX, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
			return IL_FALSE;
		TempPal = iConvertPal(context, &TempImage->Pal, IL_PAL_BGR32);
		if (TempPal == NULL)
		{
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}

	// If the current image has a palette, take care of it
	TempPal = &context->impl->iCurImage->Pal;
	if (context->impl->iCurImage->Pal.PalSize && context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		// If the palette in .bmp format, write it directly
		if (context->impl->iCurImage->Pal.PalType == IL_PAL_BGR32) {
			TempPal = &context->impl->iCurImage->Pal;
		}
		else {
			TempPal = iConvertPal(context, &context->impl->iCurImage->Pal, IL_PAL_BGR32);
			if (TempPal == NULL) {
				return IL_FALSE;
			}
		}
	}

	SaveLittleUInt(context, 54 + TempPal->PalSize);  // Offset of the data

	//Changed 20040923: moved this block above writing of
	//BITMAPINFOHEADER, so that the written header refers to
	//TempImage instead of the original image

	// @TODO LUMINANCE converted to BGR insteaf of beign saved to luminance
	if (context->impl->iCurImage->Format != IL_BGR && context->impl->iCurImage->Format != IL_BGRA && context->impl->iCurImage->Format != IL_COLOUR_INDEX) {
		if (context->impl->iCurImage->Format == IL_RGBA) {
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_BGRA, IL_UNSIGNED_BYTE);
		}
		else {
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_BGR, IL_UNSIGNED_BYTE);
		}
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else if (context->impl->iCurImage->Bpc > 1) {
		TempImage = iConvertImage(context, context->impl->iCurImage, context->impl->iCurImage->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else {
		TempImage = context->impl->iCurImage;
	}

	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	SaveLittleUInt(context, 0x28);  // Header size
	SaveLittleUInt(context, context->impl->iCurImage->Width);

	// Removed because some image viewers don't like the negative height.
	// even if it is standard. @TODO should be enabled or disabled
	// usually enabled.
	/*if (context->impl->iCurImage->Origin == IL_ORIGIN_UPPER_LEFT)
		SaveLittleInt(context, -(ILint)context->impl->iCurImage->Height);
	else*/
	SaveLittleInt(context, TempImage->Height);

	SaveLittleUShort(context, 1);  // Number of planes
	SaveLittleUShort(context, (ILushort)((ILushort)TempImage->Bpp << 3));  // Bpp
	if (compress_rle8 == IL_TRUE) {
		SaveLittleInt(context, 1); // rle8 compression
	}
	else {
		SaveLittleInt(context, 0);
	}
	SaveLittleInt(context, 0);  // Size of image (Obsolete)
	SaveLittleInt(context, 0);  // (Obsolete)
	SaveLittleInt(context, 0);  // (Obsolete)

	if (TempImage->Pal.PalType != IL_PAL_NONE) {
		SaveLittleInt(context, ilGetInteger(context, IL_PALETTE_NUM_COLS));  // Num colours used
	}
	else {
		SaveLittleInt(context, 0);
	}
	SaveLittleInt(context, 0);  // Important colour (none)

	context->impl->iwrite(context, TempPal->Palette, 1, TempPal->PalSize);


	if (compress_rle8 == IL_TRUE) {
		//@TODO compress and save
		ILubyte *Dest = (ILubyte*)ialloc(context, (long)((double)TempImage->SizeOfPlane * 130 / 127));
		FileSize = ilRleCompress(context, TempImage->Data, TempImage->Width, TempImage->Height,
			TempImage->Depth, TempImage->Bpp, Dest, IL_BMPCOMP, NULL);
		context->impl->iwrite(context, Dest, 1, FileSize);
	}
	else {
		PadSize = (4 - (TempImage->Bps % 4)) % 4;
		// No padding, so write data directly.
		if (PadSize == 0) {
			context->impl->iwrite(context, TempData, 1, TempImage->SizeOfPlane);
		}
		else {  // Odd width, so we must pad each line.
			for (i = 0; i < TempImage->SizeOfPlane; i += TempImage->Bps) {
				context->impl->iwrite(context, TempData + i, 1, TempImage->Bps); // Write data
				context->impl->iwrite(context, &Padding, 1, PadSize); // Write pad byte(s)
			}
		}
	}

	// Write the filesize
	FileSize = context->impl->itellw(context);
	context->impl->iseekw(context, 2, IL_SEEK_SET);
	SaveLittleUInt(context, FileSize);

	if (TempPal != &context->impl->iCurImage->Pal) {
		ifree(TempPal->Palette);
		ifree(TempPal);
	}
	if (TempData != TempImage->Data)
		ifree(TempData);
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	context->impl->iseekw(context, FileSize, IL_SEEK_SET);

	return IL_TRUE;
}

#endif//IL_NO_BMP