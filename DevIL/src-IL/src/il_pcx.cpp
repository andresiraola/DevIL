//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pcx.cpp
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_PCX
#include "il_pcx.h"


//! Checks if the file specified in FileName is a valid .pcx file.
ILboolean ilIsValidPcx(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PcxFile;
	ILboolean	bPcx = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("pcx"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bPcx;
	}

	PcxFile = context->impl->iopenr(FileName);
	if (PcxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPcx;
	}

	bPcx = ilIsValidPcxF(context, PcxFile);
	context->impl->icloser(PcxFile);

	return bPcx;
}


//! Checks if the ILHANDLE contains a valid .pcx file at the current position.
ILboolean ilIsValidPcxF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidPcx(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid .pcx lump.
ILboolean ilIsValidPcxL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidPcx(context);
}


// Internal function obtain the .pcx header from the current file.
ILboolean iGetPcxHead(ILcontext* context, PCXHEAD *Head)
{
	Head->Manufacturer = context->impl->igetc(context);
	Head->Version = context->impl->igetc(context);
	Head->Encoding = context->impl->igetc(context);
	Head->Bpp = context->impl->igetc(context);
	Head->Xmin = GetLittleUShort(context);
	Head->Ymin = GetLittleUShort(context);
	Head->Xmax = GetLittleUShort(context);
	Head->Ymax = GetLittleUShort(context);
	Head->HDpi = GetLittleUShort(context);
	Head->VDpi = GetLittleUShort(context);
	context->impl->iread(context, Head->ColMap, 1, 48);
	Head->Reserved = context->impl->igetc(context);
	Head->NumPlanes = context->impl->igetc(context);
	Head->Bps = GetLittleUShort(context);
	Head->PaletteInfo = GetLittleUShort(context);
	Head->HScreenSize = GetLittleUShort(context);
	Head->VScreenSize = GetLittleUShort(context);
	context->impl->iread(context, Head->Filler, 1, 54);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPcx(ILcontext* context)
{
	PCXHEAD Head;

	if (!iGetPcxHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(PCXHEAD), IL_SEEK_CUR);

	return iCheckPcx(&Head);
}


// Internal function used to check if the HEADER is a valid .pcx header.
// Should we also do a check on Header->Bpp?
ILboolean iCheckPcx(PCXHEAD *Header)
{
	ILuint	Test;

	//	Got rid of the Reserved check, because I've seen some .pcx files with invalid values in it.
	if (Header->Manufacturer != 10 || Header->Encoding != 1/* || Header->Reserved != 0*/)
		return IL_FALSE;

	// Try to support all pcx versions, as they only differ in allowed formats...
	// Let's hope it works.
	if(Header->Version != 5 && Header->Version != 0 && Header->Version != 2 &&
		 Header->VDpi != 3 && Header->VDpi != 4)
		return IL_FALSE;

	// See if the padding size is correct
	Test = Header->Xmax - Header->Xmin + 1;
	if (Header->Bpp >= 8) {
		if (Test & 1) {
			if (Header->Bps != Test + 1)
				return IL_FALSE;
		}
		else {
			if (Header->Bps != Test)  // No padding
				return IL_FALSE;
		}
	}

	/* for (i = 0; i < 54; i++) { useless check
		if (Header->Filler[i] != 0)
			return IL_FALSE;
	} */

	return IL_TRUE;
}


//! Reads a .pcx file
ILboolean ilLoadPcx(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PcxFile;
	ILboolean	bPcx = IL_FALSE;

	PcxFile = context->impl->iopenr(FileName);
	if (PcxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPcx;
	}

	bPcx = ilLoadPcxF(context, PcxFile);
	context->impl->icloser(PcxFile);

	return bPcx;
}


//! Reads an already-opened .pcx file
ILboolean ilLoadPcxF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadPcxInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .pcx
ILboolean ilLoadPcxL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadPcxInternal(context);
}


// Internal function used to load the .pcx.
ILboolean iLoadPcxInternal(ILcontext* context)
{
	PCXHEAD	Header;
	ILboolean bPcx = IL_FALSE;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return bPcx;
	}

	if (!iGetPcxHead(context, &Header))
		return IL_FALSE;
	if (!iCheckPcx(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	bPcx = iUncompressPcx(context, &Header);

	if (!bPcx)
		return IL_FALSE;
	return ilFixImage(context);
}


// Internal function to uncompress the .pcx (all .pcx files are rle compressed)
ILboolean iUncompressPcx(ILcontext* context, PCXHEAD *Header)
{
	//changed decompression loop 2003-09-01
	//From the pcx spec: "There should always
	//be a decoding break at the end of each scan line.
	//But there will not be a decoding break at the end of
	//each plane within each scan line."
	//This is now handled correctly (hopefully ;) )

	ILubyte	ByteHead, Colour, *ScanLine /* For all planes */;
	ILuint ScanLineSize;
	ILuint	c, i, x, y;

	if (Header->Bpp < 8) {
		/*ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;*/
		return iUncompressSmall(context, Header);
	}

	if (!ilTexImage(context, Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, Header->NumPlanes, 0, IL_UNSIGNED_BYTE, NULL))
		return IL_FALSE;
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (context->impl->iCurImage->Bpp)
	{
		case 1:
			context->impl->iCurImage->Format = IL_COLOUR_INDEX;
			context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
			context->impl->iCurImage->Pal.PalSize = 256 * 3;  // Need to find out for sure...
			context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
			if (context->impl->iCurImage->Pal.Palette == NULL) {
				return IL_FALSE;
			}
			break;
		//case 2:  // No 16-bit images in the pcx format!
		case 3:
			context->impl->iCurImage->Format = IL_RGB;
			context->impl->iCurImage->Pal.Palette = NULL;
			context->impl->iCurImage->Pal.PalSize = 0;
			context->impl->iCurImage->Pal.PalType = IL_PAL_NONE;
			break;
		case 4:
			context->impl->iCurImage->Format = IL_RGBA;
			context->impl->iCurImage->Pal.Palette = NULL;
			context->impl->iCurImage->Pal.PalSize = 0;
			context->impl->iCurImage->Pal.PalType = IL_PAL_NONE;
			break;

		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	ScanLineSize = context->impl->iCurImage->Bpp*Header->Bps;
	ScanLine = (ILubyte*)ialloc(context, ScanLineSize);
	if (ScanLine == NULL) {
		return IL_FALSE;
	}


	//changed 2003-09-01
	//having the decoding code twice is error-prone,
	//so I made iUnCache() smart enough to grasp
	//if iPreCache() wasn't called and call it
	//anyways.
	if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
		iPreCache(context, context->impl->iCurImage->SizeOfData / 4);

	for (y = 0; y < context->impl->iCurImage->Height; y++) {
		x = 0;
		//read scanline
		while (x < ScanLineSize) {
			if (context->impl->iread(context, &ByteHead, 1, 1) != 1) {
				iUnCache(context);
				goto file_read_error;
			}
			if ((ByteHead & 0xC0) == 0xC0) {
				ByteHead &= 0x3F;
				if (context->impl->iread(context, &Colour, 1, 1) != 1) {
					iUnCache(context);
					goto file_read_error;
				}
				if (x + ByteHead > ScanLineSize) {
					iUnCache(context);
					goto file_read_error;
				}
				for (i = 0; i < ByteHead; i++) {
					ScanLine[x++] = Colour;
				}
			}
			else {
				ScanLine[x++] = ByteHead;
			}
		}

		//convert plane-separated scanline into index, rgb or rgba pixels.
		//there might be a padding byte at the end of each scanline...
		for (x = 0; x < context->impl->iCurImage->Width; x++) {
			for(c = 0; c < context->impl->iCurImage->Bpp; c++) {
				context->impl->iCurImage->Data[y * context->impl->iCurImage->Bps + x * context->impl->iCurImage->Bpp + c] =
						ScanLine[x + c * Header->Bps];
			}
		}
	}

	iUnCache(context);

	// Read in the palette
	if (Header->Version == 5 && context->impl->iCurImage->Bpp == 1) {
		x = context->impl->itell(context);
		if (context->impl->iread(context, &ByteHead, 1, 1) == 0) {  // If true, assume that we have a luminance image.
			ilGetError(context);  // Get rid of the IL_FILE_READ_ERROR.
			context->impl->iCurImage->Format = IL_LUMINANCE;
			if (context->impl->iCurImage->Pal.Palette)
				ifree(context->impl->iCurImage->Pal.Palette);
			context->impl->iCurImage->Pal.PalSize = 0;
			context->impl->iCurImage->Pal.PalType = IL_PAL_NONE;
		}
		else {
			if (ByteHead != 12)  // Some Quake2 .pcx files don't have this byte for some reason.
				context->impl->iseek(context, -1, IL_SEEK_CUR);
			if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, context->impl->iCurImage->Pal.PalSize) != context->impl->iCurImage->Pal.PalSize)
				goto file_read_error;
		}
	}

	ifree(ScanLine);

	return IL_TRUE;

file_read_error:
	ifree(ScanLine);

	//added 2003-09-01
	ilSetError(context, IL_FILE_READ_ERROR);
	return IL_FALSE;
}


ILboolean iUncompressSmall(ILcontext* context, PCXHEAD *Header)
{
	ILuint	i = 0, j, k, c, d, x, y, Bps;
	ILubyte	HeadByte, Colour, Data = 0, *ScanLine;

	if (!ilTexImage(context, Header->Xmax - Header->Xmin + 1, Header->Ymax - Header->Ymin + 1, 1, 1, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (Header->NumPlanes)
	{
		case 1:
			context->impl->iCurImage->Format = IL_LUMINANCE;
			break;
		case 4:
			context->impl->iCurImage->Format = IL_COLOUR_INDEX;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	if (Header->NumPlanes == 1 && Header->Bpp == 1) {
		for (j = 0; j < context->impl->iCurImage->Height; j++) {
			i = 0; //number of written pixels
			while (i < context->impl->iCurImage->Width) {
				if (context->impl->iread(context, &HeadByte, 1, 1) != 1)
					return IL_FALSE;
				if (HeadByte >= 192) {
					HeadByte -= 192;
					if (context->impl->iread(context, &Data, 1, 1) != 1)
						return IL_FALSE;

					for (c = 0; c < HeadByte; c++) {
						k = 128;
						for (d = 0; d < 8 && i < context->impl->iCurImage->Width; d++) {
							context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i++] = ((Data & k) != 0 ? 255 : 0);
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (c = 0; c < 8 && i < context->impl->iCurImage->Width; c++) {
						context->impl->iCurImage->Data[j * context->impl->iCurImage->Width + i++] = ((HeadByte & k) != 0 ? 255 : 0);
						k >>= 1;
					}
				}
			}

			//if(Data != 0)
			//changed 2003-09-01:
			//There has to be an even number of bytes per line in a pcx.
			//One byte can hold up to 8 bits, so Width/8 bytes
			//are needed to hold a 1 bit per pixel image line.
			//If Width/8 is even no padding is needed,
			//one pad byte has to be read otherwise.
			//(let's hope the above is true ;-))
			if(!((context->impl->iCurImage->Width >> 3) & 0x1))
				context->impl->igetc(context);	// Skip pad byte
		}
	}
	else if (Header->NumPlanes == 4 && Header->Bpp == 1){   // 4-bit images
		//changed decoding 2003-09-10 (was buggy)...could need a speedup

		Bps = Header->Bps * Header->NumPlanes * 8;
		context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 16 * 3);  // Size of palette always (48 bytes).
		ScanLine = (ILubyte*)ialloc(context, Bps);
		if (context->impl->iCurImage->Pal.Palette == NULL || ScanLine == NULL) {
			ifree(ScanLine);
			ifree(context->impl->iCurImage->Pal.Palette);
			return IL_FALSE;
		}
		memcpy(context->impl->iCurImage->Pal.Palette, Header->ColMap, 16 * 3);
		context->impl->iCurImage->Pal.PalSize = 16 * 3;
		context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;

		memset(context->impl->iCurImage->Data, 0, context->impl->iCurImage->SizeOfData);

		if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
			iPreCache(context, context->impl->iCurImage->SizeOfData / 4);
		for (y = 0; y < context->impl->iCurImage->Height; y++) {
			x = 0;
			while (x < Bps) {
				if (context->impl->iread(context, &HeadByte, 1, 1) != 1) {
					iUnCache(context);
					ifree(ScanLine);
					return IL_FALSE;
				}
				if ((HeadByte & 0xC0) == 0xC0) {
					HeadByte &= 0x3F;
					if (context->impl->iread(context, &Colour, 1, 1) != 1) {
						iUnCache(context);
						ifree(ScanLine);
						return IL_FALSE;
					}
					for (i = 0; i < HeadByte; i++) {
						k = 128;
						for (j = 0; j < 8 && x < Bps; j++) {
							ScanLine[x++] = (Colour & k)?1:0;
							k >>= 1;
						}
					}
				}
				else {
					k = 128;
					for (j = 0; j < 8 && x < Bps; j++) {
						ScanLine[x++] = (HeadByte & k)?1:0;
						k >>= 1;
					}
				}
			}

			for (x = 0; x < context->impl->iCurImage->Width; x++) {  // 'Cleverly' ignores the pad bytes. ;)
				for(c = 0; c < Header->NumPlanes; c++)
					context->impl->iCurImage->Data[y * context->impl->iCurImage->Width + x] |= ScanLine[x + c*Header->Bps*8] << c;
			}
		}
		iUnCache(context);
		ifree(ScanLine);
	}
	else {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	return IL_TRUE;
}


//! Writes a .pcx file
ILboolean ilSavePcx(ILcontext* context, const ILstring FileName)
{
	ILHANDLE	PcxFile;
	ILuint		PcxSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	PcxFile = context->impl->iopenw(FileName);
	if (PcxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	PcxSize = ilSavePcxF(context, PcxFile);
	context->impl->iclosew(PcxFile);

	if (PcxSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Writes a .pcx to an already-opened file
ILuint ilSavePcxF(ILcontext* context, ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (iSavePcxInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


//! Writes a .pcx to a memory "lump"
ILuint ilSavePcxL(ILcontext* context, void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (iSavePcxInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


// Internal function used to save the .pcx.
ILboolean iSavePcxInternal(ILcontext* context)
{
	ILuint	i, c, PalSize;
	ILpal	*TempPal;
	ILimage	*TempImage = context->impl->iCurImage;
	ILubyte	*TempData;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	switch (context->impl->iCurImage->Format)
	{
		case IL_LUMINANCE:
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGR:
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		case IL_BGRA:
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
			break;

		default:
			if (context->impl->iCurImage->Bpc > 1) {
				TempImage = iConvertImage(context, context->impl->iCurImage, context->impl->iCurImage->Format, IL_UNSIGNED_BYTE);
				if (TempImage == NULL)
					return IL_FALSE;
			}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			if (TempImage != context->impl->iCurImage) {
				ilCloseImage(TempImage);
			}
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}


	context->impl->iputc(context, 0xA);  // Manufacturer - always 10
	context->impl->iputc(context, 0x5);  // Version Number - always 5
	context->impl->iputc(context, 0x1);  // Encoding - always 1
	context->impl->iputc(context, 0x8);  // Bits per channel
	SaveLittleUShort(context, 0);  // X Minimum
	SaveLittleUShort(context, 0);  // Y Minimum
	SaveLittleUShort(context, (ILushort)(context->impl->iCurImage->Width - 1));
	SaveLittleUShort(context, (ILushort)(context->impl->iCurImage->Height - 1));
	SaveLittleUShort(context, 0);
	SaveLittleUShort(context, 0);

	// Useless palette info?
	for (i = 0; i < 48; i++) {
		context->impl->iputc(context, 0);
	}
	context->impl->iputc(context, 0x0);  // Reserved - always 0

	context->impl->iputc(context, context->impl->iCurImage->Bpp);  // Number of planes - only 1 is supported right now

	SaveLittleUShort(context, (ILushort)(context->impl->iCurImage->Width & 1 ? context->impl->iCurImage->Width + 1 : context->impl->iCurImage->Width));  // Bps
	SaveLittleUShort(context, 0x1);  // Palette type - ignored?

	// Mainly filler info
	for (i = 0; i < 58; i++) {
		context->impl->iputc(context, 0x0);
	}

	// Output data
	for (i = 0; i < TempImage->Height; i++) {
		for (c = 0; c < TempImage->Bpp; c++) {
			encLine(context, TempData + TempImage->Bps * i + c, TempImage->Width, (ILubyte)(TempImage->Bpp - 1));
		}
	}

	// Automatically assuming we have a palette...dangerous!
	//	Also assuming 3 bpp palette
	context->impl->iputc(context, 0xC);  // Pad byte must have this value

	// If the current image has a palette, take care of it
	if (TempImage->Format == IL_COLOUR_INDEX) {
		// If the palette in .pcx format, write it directly
		if (TempImage->Pal.PalType == IL_PAL_RGB24) {
			context->impl->iwrite(context, TempImage->Pal.Palette, 1, TempImage->Pal.PalSize);
		}
		else {
			TempPal = iConvertPal(context, &TempImage->Pal, IL_PAL_RGB24);
			if (TempPal == NULL) {
				if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
					ifree(TempData);
				if (TempImage != context->impl->iCurImage)
					ilCloseImage(TempImage);
				return IL_FALSE;
			}

			context->impl->iwrite(context, TempPal->Palette, 1, TempPal->PalSize);
			ifree(TempPal->Palette);
			ifree(TempPal);
		}
	}

	// If the palette is not all 256 colours, we have to pad it.
	PalSize = 768 - context->impl->iCurImage->Pal.PalSize;
	for (i = 0; i < PalSize; i++) {
		context->impl->iputc(context, 0x0);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}


// Routine used from ZSoft's pcx documentation
ILuint encput(ILcontext* context, ILubyte byt, ILubyte cnt)
{
	if (cnt) {
		if ((cnt == 1) && (0xC0 != (0xC0 & byt))) {
			if (IL_EOF == context->impl->iputc(context, byt))
				return(0);     /* disk write error (probably full) */
			return(1);
		}
		else {
			if (IL_EOF == context->impl->iputc(context, (ILubyte)((ILuint)0xC0 | cnt)))
				return (0);      /* disk write error */
			if (IL_EOF == context->impl->iputc(context, byt))
				return (0);      /* disk write error */
			return (2);
		}
	}

	return (0);
}

// This subroutine encodes one scanline and writes it to a file.
//  It returns number of bytes written into outBuff, 0 if failed.
ILuint encLine(ILcontext* context, ILubyte *inBuff, ILint inLen, ILubyte Stride)
{
	ILubyte _this, last;
	ILint srcIndex, i;
	ILint total;
	ILubyte runCount;     // max single runlength is 63
	total = 0;
	runCount = 1;
	last = *(inBuff);

	// Find the pixel dimensions of the image by calculating 
	//[XSIZE = Xmax - Xmin + 1] and [YSIZE = Ymax - Ymin + 1].  
	//Then calculate how many bytes are in a "run"

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		inBuff += Stride;
		_this = *(++inBuff);
		if (_this == last) {  // There is a "run" in the data, encode it
			runCount++;
			if (runCount == 63) {
				if (! (i = encput(context, last, runCount)))
						return (0);
				total += i;
				runCount = 0;
			}
		}
		else {  // No "run"  -  _this != last
			if (runCount) {
				if (! (i = encput(context, last, runCount)))
					return(0);
				total += i;
			}
			last = _this;
			runCount = 1;
		}
	}  // endloop

	if (runCount) {  // finish up
		if (! (i = encput(context, last, runCount)))
			return (0);
		if (inLen % 2)
			context->impl->iputc(context, 0);
		return (total + i);
	}
	else {
		if (inLen % 2)
			context->impl->iputc(context, 0);
	}

	return (total);
}

#endif//IL_NO_PCX
