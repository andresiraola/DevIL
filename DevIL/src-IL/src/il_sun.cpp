//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/06/2009
//
// Filename: src-IL/src/il_sun.cpp
//
// Description: Reads from a Sun RAS file.  Specifications available from
//				http://www.fileformat.info/format/sunraster/egff.htm.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_SUN

#include "il_bits.h"
#include "il_sun.h"

ILuint		iSunGetRle(ILcontext* context, ILubyte *Data, ILuint Length);

typedef struct SUNHEAD
{
	ILuint MagicNumber;      // Magic (identification) number
	ILuint Width;            // Width of image in pixels
	ILuint Height;           // Height of image in pixels
	ILuint Depth;            // Number of bits per pixel
	ILuint Length;           // Size of image data in bytes
	ILuint Type;             // Type of raster file
	ILuint ColorMapType;     // Type of color map
	ILuint ColorMapLength;   // Size of the color map in bytes
} SUNHEAD;

// Data storage types
#define IL_SUN_OLD		0x00
#define IL_SUN_STANDARD	0x01
#define IL_SUN_BYTE_ENC	0x02
#define IL_SUN_RGB		0x03
#define IL_SUN_TIFF		0x04
#define IL_SUN_IFF		0x05
#define IL_SUN_EXPER	0xFFFF  // Experimental, not supported.

// Colormap types
#define IL_SUN_NO_MAP	0x00
#define IL_SUN_RGB_MAP	0x01
#define IL_SUN_RAW_MAP	0x02

SunHandler::SunHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid Sun file.
ILboolean SunHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	SunFile;
	ILboolean	bSun = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("sun")) && !iCheckExtension(FileName, IL_TEXT("ras")) &&
		!iCheckExtension(FileName, IL_TEXT("im1")) && !iCheckExtension(FileName, IL_TEXT("im8")) &&
		!iCheckExtension(FileName, IL_TEXT("im24")) && !iCheckExtension(FileName, IL_TEXT("im32")) &&
		!iCheckExtension(FileName, IL_TEXT("rs"))) {  // Lots of names possible...
		ilSetError(context, IL_INVALID_EXTENSION);
		return bSun;
	}
	
	SunFile = context->impl->iopenr(FileName);
	if (SunFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bSun;
	}
	
	bSun = isValidF(SunFile);
	context->impl->icloser(SunFile);
	
	return bSun;
}

//! Checks if the ILHANDLE contains a valid Sun file at the current position.
ILboolean SunHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Checks if Lump is a valid Sun lump.
ILboolean SunHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the Sun header from the current file.
ILboolean iGetSunHead(ILcontext* context, SUNHEAD *Header)
{
	Header->MagicNumber = GetBigUInt(context);
	Header->Width = GetBigUInt(context);
	Header->Height = GetBigUInt(context);
	Header->Depth = GetBigUInt(context);
	Header->Length = GetBigUInt(context);
	Header->Type = GetBigUInt(context);
	Header->ColorMapType = GetBigUInt(context);
	Header->ColorMapLength = GetBigUInt(context);

	return IL_TRUE;
}

// Internal function used to check if the HEADER is a valid SUN header.
ILboolean iCheckSun(SUNHEAD *Header)
{
	if (Header->MagicNumber != 0x59A66A95)  // Magic number is always 0x59A66A95.
		return IL_FALSE;
	if (Header->Width == 0 || Header->Height == 0)  // 0 dimensions are meaningless.
		return IL_FALSE;
	// These are the only valid depths that I know of.
	if (Header->Depth != 1 && Header->Depth != 8 && Header->Depth != 24 && Header->Depth != 32)
		return IL_FALSE;
	if (Header->Type > IL_SUN_RGB)  //@TODO: Support further types.
		return IL_FALSE;
	if (Header->ColorMapType > IL_SUN_RGB_MAP)  //@TODO: Find out more about raw map.
		return IL_FALSE;
	// Color map cannot be 0 if there is a map indicated.
	if (Header->ColorMapType > IL_SUN_NO_MAP && Header->ColorMapLength == 0)
		return IL_FALSE;
	//@TODO: These wouldn't make sense.  Are they valid somehow?  Find out...
	if ((Header->Depth == 1 || Header->Depth == 32) && Header->Type == IL_SUN_BYTE_ENC)
		return IL_FALSE;

	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean SunHandler::isValidInternal()
{
	SUNHEAD Head;

	if (!iGetSunHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(SUNHEAD), IL_SEEK_CUR);
	
	return iCheckSun(&Head);
}

// Reads a Sun file
ILboolean SunHandler::load(ILconst_string FileName)
{
	ILHANDLE	SunFile;
	ILboolean	bSun = IL_FALSE;

	SunFile = context->impl->iopenr(FileName);
	if (SunFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bSun;
	}

	iSetInputFile(context, SunFile);

	bSun = loadF(SunFile);

	context->impl->icloser(SunFile);

	return bSun;
}

//! Reads an already-opened Sun file
ILboolean SunHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a Sun
ILboolean SunHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

ILboolean SunHandler::loadInternal()
{
	SUNHEAD	Header;
	BITFILE	*File;
	ILuint	i, j, Padding, Offset, BytesRead;
	ILubyte	PaddingData[16];

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//@TODO: Right now, iGetSunHead cannot fail.
	if (!iGetSunHead(context, &Header) || !iCheckSun(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	switch (Header.Depth)
	{
		case 1:  //@TODO: Find a file to test this on.
			File = bfile(context, iGetFile(context));
			if (File == NULL)
				return IL_FALSE;

			if (!ilTexImage(context, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			if (Header.ColorMapLength != 0) {
				// Data should be an index into the color map, but the color map should only be RGB (6 bytes, 2 entries).
				if (Header.ColorMapLength != 6) {
					ilSetError(context, IL_INVALID_FILE_HEADER);
					return IL_FALSE;
				}
			}
			context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, 6);  // Just need 2 entries in the color map.
			if (Header.ColorMapLength == 0) {  // Create the color map
				context->impl->iCurImage->Pal.Palette[0] = 0x00;  // Entry for black
				context->impl->iCurImage->Pal.Palette[1] = 0x00;
				context->impl->iCurImage->Pal.Palette[2] = 0x00;
				context->impl->iCurImage->Pal.Palette[3] = 0xFF;  // Entry for white
				context->impl->iCurImage->Pal.Palette[4] = 0xFF;
				context->impl->iCurImage->Pal.Palette[5] = 0xFF;
			}
			else {
				context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, 6);  // Read in the color map.
			}
			context->impl->iCurImage->Pal.PalSize = 6;
			context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;

			Padding = (16 - (context->impl->iCurImage->Width % 16)) % 16;  // Has to be aligned on a 16-bit boundary.  The rest is padding.

			// Reads the bits
			for (i = 0; i < context->impl->iCurImage->Height; i++) {
				bread(context, &context->impl->iCurImage->Data[context->impl->iCurImage->Width * i], 1, context->impl->iCurImage->Width, File);
				//bseek(File, BitPadding, IL_SEEK_CUR);  //@TODO: This function does not work correctly.
				bread(context, PaddingData, 1, Padding, File);  // Skip padding bits.
			}
			break;


		case 8:
			if (Header.ColorMapType == IL_SUN_NO_MAP) {  // Greyscale image
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
			}
			else {  // Colour-mapped image
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
				context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, Header.ColorMapLength);  // Allocate color map.
				if (context->impl->iCurImage->Pal.Palette == NULL)
					return IL_FALSE;
				if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, Header.ColorMapLength) != Header.ColorMapLength) {  // Read color map.
					ilSetError(context, IL_FILE_READ_ERROR);
					return IL_FALSE;
				}

				context->impl->iCurImage->Pal.PalSize = Header.ColorMapLength;
				context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
			}

			if (Header.Type != IL_SUN_BYTE_ENC) {  // Regular uncompressed image data
				Padding = (2 - (context->impl->iCurImage->Bps % 2)) % 2;  // Must be padded on a 16-bit boundary (2 bytes)
				for (i = 0; i < Header.Height; i++) {
					context->impl->iread(context, context->impl->iCurImage->Data + i * Header.Width, 1, context->impl->iCurImage->Bps);
					if (Padding)  // Only possible for padding to be 0 or 1.
						context->impl->igetc(context);
				}
			}
			else {  // RLE image data
				for (i = 0; i < context->impl->iCurImage->Height; i++) {
					BytesRead = iSunGetRle(context, context->impl->iCurImage->Data + context->impl->iCurImage->Bps * i, context->impl->iCurImage->Bps);
					if (BytesRead % 2)  // Each scanline must be aligned on a 2-byte boundary.
						context->impl->igetc(context);  // Skip padding
				}
			}
			break;

		case 24:
			if (Header.ColorMapLength > 0)  // Ignore any possible colormaps.
				context->impl->iseek(context, Header.ColorMapLength, IL_SEEK_CUR);

			if (Header.Type == IL_SUN_RGB) {
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
			}
			else {
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
			}

			if (Header.Type != IL_SUN_BYTE_ENC) {  // Regular uncompressed image data
				Padding = (2 - (context->impl->iCurImage->Bps % 2)) % 2;  // Must be padded on a 16-bit boundary (2 bytes)
				for (i = 0; i < Header.Height; i++) {
					context->impl->iread(context, context->impl->iCurImage->Data + i * Header.Width * 3, 1, context->impl->iCurImage->Bps);
					if (Padding)  // Only possible for padding to be 0 or 1.
						context->impl->igetc(context);
				}
			}
			else {  // RLE image data
				for (i = 0; i < context->impl->iCurImage->Height; i++) {
					BytesRead = iSunGetRle(context, context->impl->iCurImage->Data + context->impl->iCurImage->Bps * i, context->impl->iCurImage->Bps);
					if (BytesRead % 2)  // Each scanline must be aligned on a 2-byte boundary.
						context->impl->igetc(context);  // Skip padding
				}
			}

			break;

		case 32:
			if (Header.ColorMapLength > 0)  // Ignore any possible colormaps.
				context->impl->iseek(context, Header.ColorMapLength, IL_SEEK_CUR);

			if (Header.Type == IL_SUN_RGB) {
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
			}
			else {
				if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL))
					return IL_FALSE;
			}

			// There is no padding at the end of each scanline.
			Offset = 0;
			for (i = 0; i < Header.Height; i++) {
				for (j = 0; j < Header.Width; j++) {
					context->impl->igetc(context);  // There is a pad byte before each pixel.
					context->impl->iCurImage->Data[Offset]   = context->impl->igetc(context);
					context->impl->iCurImage->Data[Offset+1] = context->impl->igetc(context);
					context->impl->iCurImage->Data[Offset+2] = context->impl->igetc(context);
				}
			}
			break;


		default:  // Should have already been checked with iGetSunHead.
			return IL_FALSE;
	}

	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	return ilFixImage(context);
}

ILuint iSunGetRle(ILcontext* context, ILubyte *Data, ILuint Length)
{
	ILuint	i = 0, j;
	ILubyte	Flag, Value;
	ILuint	Count;

	for (i = 0; i < Length; ) {
		Flag = context->impl->igetc(context);
		if (Flag == 0x80) {  // Run follows (or 1 byte of 0x80)
			Count = context->impl->igetc(context);
			if (Count == 0) {  // 1 pixel of value (0x80)
				*Data = 0x80;
				Data++;
				i++;
			}
			else {  // Here we have a run.
				Value = context->impl->igetc(context);
				Count++;  // Should really be Count+1
				for (j = 0; j < Count && i + j < Length; j++, Data++) {
					*Data = Value;
				}
				i += Count;
			}
		}
		else {  // 1 byte of this value (cannot be 0x80)
			*Data = Flag;
			Data++;
			i++;
		}
	}

	return i;
}

#endif//IL_NO_SUN