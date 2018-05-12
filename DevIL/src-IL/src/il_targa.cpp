//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_targa.cpp
//
// Description: Reads from and writes to a Targa (.tga) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_TGA
#include "il_targa.h"
//#include <time.h>  // for ilMakeString()
#include <string.h>
#include "il_bits.h"

#ifdef DJGPP
#include <dos.h>
#endif

void		iGetDateTime(ILuint *Month, ILuint *Day, ILuint *Yr, ILuint *Hr, ILuint *Min, ILuint *Sec);

TargaHandler::TargaHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid Targa file.
ILboolean TargaHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	TargaFile;
	ILboolean	bTarga = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("tga")) &&
		!iCheckExtension(FileName, IL_TEXT("vda")) &&
		!iCheckExtension(FileName, IL_TEXT("icb")) &&
		!iCheckExtension(FileName, IL_TEXT("vst"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bTarga;
	}
	
	TargaFile = context->impl->iopenr(FileName);
	if (TargaFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bTarga;
	}
	
	bTarga = isValidF(TargaFile);
	context->impl->icloser(TargaFile);
	
	return bTarga;
}

//! Checks if the ILHANDLE contains a valid Targa file at the current position.
ILboolean TargaHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Checks if Lump is a valid Targa lump.
ILboolean TargaHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the Targa header from the current file.
ILboolean TargaHandler::iGetTgaHead(TARGAHEAD *Header)
{
	Header->IDLen = (ILubyte)context->impl->igetc(context);
	Header->ColMapPresent = (ILubyte)context->impl->igetc(context);
	Header->ImageType = (ILubyte)context->impl->igetc(context);
	Header->FirstEntry = GetLittleShort(context);
	Header->ColMapLen = GetLittleShort(context);
	Header->ColMapEntSize = (ILubyte)context->impl->igetc(context);

	Header->OriginX = GetLittleShort(context);
	Header->OriginY = GetLittleShort(context);
	Header->Width = GetLittleUShort(context);
	Header->Height = GetLittleUShort(context);
	Header->Bpp = (ILubyte)context->impl->igetc(context);
	Header->ImageDesc = (ILubyte)context->impl->igetc(context);
	
	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean TargaHandler::isValidInternal()
{
	TARGAHEAD	Head;
	
	if (!iGetTgaHead(&Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(TARGAHEAD), IL_SEEK_CUR);
	
	return check(&Head);
}

// Internal function used to check if the HEADER is a valid Targa header.
ILboolean TargaHandler::check(TARGAHEAD *Header)
{
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	if (Header->Bpp != 8 && Header->Bpp != 15 && Header->Bpp != 16 
		&& Header->Bpp != 24 && Header->Bpp != 32)
		return IL_FALSE;
	if (Header->ImageDesc & BIT_4)	// Supposed to be set to 0
		return IL_FALSE;
	
	// check type (added 20040218)
	if (Header->ImageType   != TGA_NO_DATA
	   && Header->ImageType != TGA_COLMAP_UNCOMP
	   && Header->ImageType != TGA_UNMAP_UNCOMP
	   && Header->ImageType != TGA_BW_UNCOMP
	   && Header->ImageType != TGA_COLMAP_COMP
	   && Header->ImageType != TGA_UNMAP_COMP
	   && Header->ImageType != TGA_BW_COMP)
		return IL_FALSE;
	
	// Doesn't work well with the bitshift so change it.
	if (Header->Bpp == 15)
		Header->Bpp = 16;
	
	return IL_TRUE;
}

//! Reads a Targa file
ILboolean TargaHandler::load(ILconst_string FileName)
{
	ILHANDLE	TargaFile;
	ILboolean	bTarga = IL_FALSE;
	
	TargaFile = context->impl->iopenr(FileName);
	if (TargaFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bTarga;
	}

	bTarga = loadF(TargaFile);
	context->impl->icloser(TargaFile);

	return bTarga;
}

//! Reads an already-opened Targa file
ILboolean TargaHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Reads from a memory "lump" that contains a Targa
ILboolean TargaHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the Targa.
ILboolean TargaHandler::loadInternal()
{
	TARGAHEAD	Header;
	ILboolean	bTarga;
	ILenum		iOrigin;
	
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!iGetTgaHead(&Header))
		return IL_FALSE;
	if (!check(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	
	switch (Header.ImageType)
	{
		case TGA_NO_DATA:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		case TGA_COLMAP_UNCOMP:
		case TGA_COLMAP_COMP:
			bTarga = iReadColMapTga(&Header);
			break;
		case TGA_UNMAP_UNCOMP:
		case TGA_UNMAP_COMP:
			bTarga = iReadUnmapTga(&Header);
			break;
		case TGA_BW_UNCOMP:
		case TGA_BW_COMP:
			bTarga = iReadBwTga(&Header);
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}
	
	// @JASON Extra Code to manipulate the image depending on
	// the Image Descriptor's origin bits.
	iOrigin = Header.ImageDesc & IMAGEDESC_ORIGIN_MASK;
	
	switch (iOrigin)
	{
		case IMAGEDESC_TOPLEFT:
			context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
			break;
			
		case IMAGEDESC_TOPRIGHT:
			context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
			iMirror(context);
			break;
			
		case IMAGEDESC_BOTLEFT:
			context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			break;
			
		case IMAGEDESC_BOTRIGHT:
			context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			iMirror(context);
			break;
	}
	
	return ilFixImage(context);
}

ILboolean TargaHandler::iReadColMapTga(TARGAHEAD *Header)
{
	char		ID[255];
	ILuint		i;
	ILushort	Pixel;
	
	if (context->impl->iread(context, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	if (!ilTexImage(context, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize)
		ifree(context->impl->iCurImage->Pal.Palette);
	
	context->impl->iCurImage->Format = IL_COLOUR_INDEX;
	context->impl->iCurImage->Pal.PalSize = Header->ColMapLen * (Header->ColMapEntSize >> 3);
	
	switch (Header->ColMapEntSize)
	{
		case 16:
			context->impl->iCurImage->Pal.PalType = IL_PAL_BGRA32;
			context->impl->iCurImage->Pal.PalSize = Header->ColMapLen * 4;
			break;
		case 24:
			context->impl->iCurImage->Pal.PalType = IL_PAL_BGR24;
			break;
		case 32:
			context->impl->iCurImage->Pal.PalType = IL_PAL_BGRA32;
			break;
		default:
			// Should *never* reach here
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}
	
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		return IL_FALSE;
	}
	
	// Do we need to do something with FirstEntry?	Like maybe:
	//	context->impl->iread(context, Image->Pal + Targa->FirstEntry, 1, Image->Pal.PalSize);  ??
	if (Header->ColMapEntSize != 16)
	{
		if (context->impl->iread(context, context->impl->iCurImage->Pal.Palette, 1, context->impl->iCurImage->Pal.PalSize) != context->impl->iCurImage->Pal.PalSize)
			return IL_FALSE;
	}
	else {
		// 16 bit palette, so we have to break it up.
		for (i = 0; i < context->impl->iCurImage->Pal.PalSize; i += 4)
		{
			Pixel = GetBigUShort(context);
			if (context->impl->ieof(context))
				return IL_FALSE;
			context->impl->iCurImage->Pal.Palette[3] = (Pixel & 0x8000) >> 12;
			context->impl->iCurImage->Pal.Palette[0] = (Pixel & 0xFC00) >> 7;
			context->impl->iCurImage->Pal.Palette[1] = (Pixel & 0x03E0) >> 2;
			context->impl->iCurImage->Pal.Palette[2] = (Pixel & 0x001F) << 3;
		}
	}
	
	if (Header->ImageType == TGA_COLMAP_COMP)
	{
		if (!iUncompressTgaData(context->impl->iCurImage))
		{
			return IL_FALSE;
		}
	}
	else
	{
		if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData)
		{
			return IL_FALSE;
		}
	}
	
	return IL_TRUE;
}

ILboolean TargaHandler::iReadUnmapTga(TARGAHEAD *Header)
{
	ILubyte Bpp;
	char	ID[255];
	
	if (context->impl->iread(context, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	/*if (Header->Bpp == 16)
		Bpp = 3;
	else*/
	Bpp = (ILubyte)(Header->Bpp >> 3);
	
	if (!ilTexImage(context, Header->Width, Header->Height, 1, Bpp, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	
	switch (context->impl->iCurImage->Bpp)
	{
		case 1:
			context->impl->iCurImage->Format = IL_COLOUR_INDEX;  // wtf?  How is this possible?
			break;
		case 2:  // 16-bit is not supported directly!
				 //context->impl->iCurImage->Format = IL_RGB5_A1;
			/*context->impl->iCurImage->Format = IL_RGBA;
			context->impl->iCurImage->Type = IL_UNSIGNED_SHORT_5_5_5_1_EXT;*/
			//context->impl->iCurImage->Type = IL_UNSIGNED_SHORT_5_6_5_REV;
			
			// Remove?
			//ilCloseImage(context->impl->iCurImage);
			//ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			//return IL_FALSE;
			
			/*context->impl->iCurImage->Bpp = 4;
			context->impl->iCurImage->Format = IL_BGRA;
			context->impl->iCurImage->Type = IL_UNSIGNED_SHORT_1_5_5_5_REV;*/
			
			context->impl->iCurImage->Format = IL_BGR;
			
			break;
		case 3:
			context->impl->iCurImage->Format = IL_BGR;
			break;
		case 4:
			context->impl->iCurImage->Format = IL_BGRA;
			break;
		default:
			ilSetError(context, IL_INVALID_VALUE);
			return IL_FALSE;
	}
	
	
	// @TODO:  Determine this:
	// We assume that no palette is present, but it's possible...
	//	Should we mess with it or not?
	
	
	if (Header->ImageType == TGA_UNMAP_COMP) {
		if (!iUncompressTgaData(context->impl->iCurImage)) {
			return IL_FALSE;
		}
	}
	else {
		if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData) {
			return IL_FALSE;
		}
	}
	
	// Go ahead and expand it to 24-bit.
	if (Header->Bpp == 16) {
		if (!i16BitTarga(context->impl->iCurImage))
			return IL_FALSE;
		return IL_TRUE;
	}
	
	return IL_TRUE;
}

ILboolean TargaHandler::iReadBwTga(TARGAHEAD *Header)
{
	char ID[255];
	
	if (context->impl->iread(context, ID, 1, Header->IDLen) != Header->IDLen)
		return IL_FALSE;
	
	// We assume that no palette is present, but it's possible...
	//	Should we mess with it or not?
	
	if (!ilTexImage(context, Header->Width, Header->Height, 1, (ILubyte)(Header->Bpp >> 3), IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	
	if (Header->ImageType == TGA_BW_COMP) {
		if (!iUncompressTgaData(context->impl->iCurImage)) {
			return IL_FALSE;
		}
	}
	else {
		if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData) {
			return IL_FALSE;
		}
	}
	
	return IL_TRUE;
}

ILboolean TargaHandler::iUncompressTgaData(ILimage *Image)
{
	ILuint	BytesRead = 0, Size, RunLen, i, ToRead;
	ILubyte Header, Color[4];
	ILint	c;
	
	Size = Image->Width * Image->Height * Image->Depth * Image->Bpp;
	
	if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
		iPreCache(context, context->impl->iCurImage->SizeOfData / 2);
	
	while (BytesRead < Size) {
		Header = (ILubyte)context->impl->igetc(context);
		if (Header & BIT_7) {
			ClearBits(Header, BIT_7);
			if (context->impl->iread(context, Color, 1, Image->Bpp) != Image->Bpp) {
				iUnCache(context);
				return IL_FALSE;
			}
			RunLen = (Header+1) * Image->Bpp;
			for (i = 0; i < RunLen; i += Image->Bpp) {
				// Read the color in, but we check to make sure that we do not go past the end of the image.
				for (c = 0; c < Image->Bpp && BytesRead+i+c < Size; c++) {
					Image->Data[BytesRead+i+c] = Color[c];
				}
			}
			BytesRead += RunLen;
		}
		else {
			RunLen = (Header+1) * Image->Bpp;
			// We have to check that we do not go past the end of the image data.
			if (BytesRead + RunLen > Size)
				ToRead = Size - BytesRead;
			else
				ToRead = RunLen;
			if (context->impl->iread(context, Image->Data + BytesRead, 1, ToRead) != ToRead) {
				iUnCache(context);  //@TODO: Error needed here?
				return IL_FALSE;
			}
			BytesRead += RunLen;

			if (BytesRead + RunLen > Size)
				context->impl->iseek(context, RunLen - ToRead, IL_SEEK_CUR);
		}
	}
	
	iUnCache(context);
	
	return IL_TRUE;
}

// Pretty damn unoptimized
ILboolean TargaHandler::i16BitTarga(ILimage *Image)
{
	ILushort	*Temp1;
	ILubyte 	*Data, *Temp2;
	ILuint		x, PixSize = Image->Width * Image->Height;
	
	Data = (ILubyte*)ialloc(context, Image->Width * Image->Height * 3);
	Temp1 = (ILushort*)Image->Data;
	Temp2 = Data;
	
	if (Data == NULL)
		return IL_FALSE;
	
	for (x = 0; x < PixSize; x++) {
		*Temp2++ = (*Temp1 & 0x001F) << 3;	// Blue
		*Temp2++ = (*Temp1 & 0x03E0) >> 2;	// Green
		*Temp2++ = (*Temp1 & 0x7C00) >> 7;	// Red
		
		Temp1++;
		
		
		/*s = *Temp;
		s = SwapShort(s);
		a = !!(s & BIT_15);
		
		s = s << 1;
		
		//if (a) {
		SetBits(s, BIT_0);
		//}
		
		//SetBits(s, BIT_15);
		
		*Temp++ = s;*/
	}
	
	if (!ilTexImage(context, Image->Width, Image->Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, Data)) {
		ifree(Data);
		return IL_FALSE;
	}
	
	ifree(Data);
	
	return IL_TRUE;
}

//! Writes a Targa file
ILboolean TargaHandler::save(const ILstring FileName)
{
	ILHANDLE	TargaFile;
	ILuint		TargaSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	TargaFile = context->impl->iopenw(FileName);
	if (TargaFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	TargaSize = saveF(TargaFile);
	context->impl->iclosew(TargaFile);

	if (TargaSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a Targa to an already-opened file
ILuint TargaHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a Targa to a memory "lump"
ILuint TargaHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos = context->impl->itellw(context);
	iSetOutputLump(context, Lump, Size);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Internal function used to save the Targa.
ILboolean TargaHandler::saveInternal()
{
	const char	*ID = iGetString(context, IL_TGA_ID_STRING);
	const char	*AuthName = iGetString(context, IL_TGA_AUTHNAME_STRING);
	const char	*AuthComment = iGetString(context, IL_TGA_AUTHCOMMENT_STRING);
	ILubyte 	IDLen = 0, UsePal, Type, PalEntSize;
	ILshort 	ColMapStart = 0, PalSize;
	ILubyte		Temp;
	ILenum		Format;
	ILboolean	Compress;
	ILuint		RleLen;
	ILubyte 	*Rle;
	ILpal		*TempPal = NULL;
	ILimage 	*TempImage = NULL;
	ILuint		ExtOffset, i;
	char		*Footer = (char*)"TRUEVISION-XFILE.\0";
	char		*idString = (char*)"Developer's Image Library (DevIL)";
	ILuint		Day, Month, Year, Hour, Minute, Second;
	char		*TempData;
	ILshort		zero_short = 0;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (iGetInt(context, IL_TGA_RLE) == IL_TRUE)
		Compress = IL_TRUE;
	else
		Compress = IL_FALSE;
	
	if (ID)
		IDLen = (ILubyte)ilCharStrLen(ID);
	
	if (context->impl->iCurImage->Pal.Palette && context->impl->iCurImage->Pal.PalSize && context->impl->iCurImage->Pal.PalType != IL_PAL_NONE)
		UsePal = IL_TRUE;
	else
		UsePal = IL_FALSE;
	
	context->impl->iwrite(context, &IDLen, sizeof(ILubyte), 1);
	context->impl->iwrite(context, &UsePal, sizeof(ILubyte), 1);

	Format = context->impl->iCurImage->Format;
	switch (Format) {
		case IL_COLOUR_INDEX:
			if (Compress)
				Type = 9;
			else
				Type = 1;
			break;
		case IL_BGR:
		case IL_BGRA:
			if (Compress)
				Type = 10;
			else
				Type = 2;
			break;
		case IL_RGB:
		case IL_RGBA:
			ilSwapColours(context);
			if (Compress)
				Type = 10;
			else
				Type = 2;
			break;
		case IL_LUMINANCE:
			if (Compress)
				Type = 11;
			else
				Type = 3;
			break;
		default:
			// Should convert the types here...
			ilSetError(context, IL_INVALID_VALUE);
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
	}
	
	context->impl->iwrite(context, &Type, sizeof(ILubyte), 1);
	SaveLittleShort(context, ColMapStart);
	
	switch (context->impl->iCurImage->Pal.PalType)
	{
		case IL_PAL_NONE:
			PalSize = 0;
			PalEntSize = 0;
			break;
		case IL_PAL_BGR24:
			PalSize = (ILshort)(context->impl->iCurImage->Pal.PalSize / 3);
			PalEntSize = 24;
			TempPal = &context->impl->iCurImage->Pal;
			break;
			
		case IL_PAL_RGB24:
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			TempPal = iConvertPal(context, &context->impl->iCurImage->Pal, IL_PAL_BGR24);
			if (TempPal == NULL)
				return IL_FALSE;
				PalSize = (ILshort)(TempPal->PalSize / 3);
			PalEntSize = 24;
			break;
		default:
			ilSetError(context, IL_INVALID_VALUE);
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			PalSize = 0;
			PalEntSize = 0;
			return IL_FALSE;
	}
	SaveLittleShort(context, PalSize);
	context->impl->iwrite(context, &PalEntSize, sizeof(ILubyte), 1);
	
	if (context->impl->iCurImage->Bpc > 1) {
		TempImage = iConvertImage(context, context->impl->iCurImage, context->impl->iCurImage->Format, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			ifree(ID);
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
		}
	}
	else {
		TempImage = context->impl->iCurImage;
	}
	
	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
		TempData = (char*)iGetFlipped(context, TempImage);
	}
	else
		TempData = (char*)TempImage->Data;
	
	// Write out the origin stuff.
	context->impl->iwrite(context, &zero_short, sizeof(ILshort), 1);
	context->impl->iwrite(context, &zero_short, sizeof(ILshort), 1);
	
	Temp = context->impl->iCurImage->Bpp << 3;  // Changes to bits per pixel 
	SaveLittleUShort(context, (ILushort)context->impl->iCurImage->Width);
	SaveLittleUShort(context, (ILushort)context->impl->iCurImage->Height);
	context->impl->iwrite(context, &Temp, sizeof(ILubyte), 1);
	
	// Still don't know what exactly this is for...
	// It's actually the 'Image Descriptor Byte'
	// from wiki: Image descriptor (1 byte): bits 3-0 give the alpha channel depth, bits 5-4 give direction
	Temp = 0;
	if (context->impl->iCurImage->Bpp > 3)
		Temp = 8;
	if (TempImage->Origin == IL_ORIGIN_UPPER_LEFT)
		Temp |= 0x20; //set 5th bit
	context->impl->iwrite(context, &Temp, sizeof(ILubyte), 1);
	context->impl->iwrite(context, ID, sizeof(char), IDLen);
	ifree(ID);
	//context->impl->iwrite((ID, sizeof(ILbyte), IDLen - sizeof(ILuint));
	//context->impl->iwrite(context, &context->impl->iCurImage->Depth, sizeof(ILuint), 1);
	
	// Write out the colormap
	if (UsePal)
		context->impl->iwrite(context, TempPal->Palette, sizeof(ILubyte), TempPal->PalSize);
	// else do nothing
	
	if (!Compress)
		context->impl->iwrite(context, TempData, sizeof(ILubyte), TempImage->SizeOfData);
	else {
		Rle = (ILubyte*)ialloc(context, TempImage->SizeOfData + TempImage->SizeOfData / 2 + 1);	// max
		if (Rle == NULL) {
			ifree(AuthName);
			ifree(AuthComment);
			return IL_FALSE;
		}
		RleLen = ilRleCompress(context, (unsigned char*)TempData, TempImage->Width, TempImage->Height,
		                       TempImage->Depth, TempImage->Bpp, Rle, IL_TGACOMP, NULL);
		
		context->impl->iwrite(context, Rle, 1, RleLen);
		ifree(Rle);
	}
	
	// Write the extension area.
	ExtOffset = context->impl->itellw(context);
	SaveLittleUShort(context, 495);	// Number of bytes in the extension area (TGA 2.0 spec)
	context->impl->iwrite(context, AuthName, 1, ilCharStrLen(AuthName));
	ipad(context, 41 - ilCharStrLen(AuthName));
	context->impl->iwrite(context, AuthComment, 1, ilCharStrLen(AuthComment));
	ipad(context, 324 - ilCharStrLen(AuthComment));
	ifree(AuthName);
	ifree(AuthComment);
	
	// Write time/date
	iGetDateTime(&Month, &Day, &Year, &Hour, &Minute, &Second);
	SaveLittleUShort(context, (ILushort)Month);
	SaveLittleUShort(context, (ILushort)Day);
	SaveLittleUShort(context, (ILushort)Year);
	SaveLittleUShort(context, (ILushort)Hour);
	SaveLittleUShort(context, (ILushort)Minute);
	SaveLittleUShort(context, (ILushort)Second);
	
	for (i = 0; i < 6; i++) {  // Time created
		SaveLittleUShort(context, 0);
	}
	for (i = 0; i < 41; i++) {	// Job name/ID
		context->impl->iputc(context, 0);
	}
	for (i = 0; i < 3; i++) {  // Job time
		SaveLittleUShort(context, 0);
	}
	
	context->impl->iwrite(context, idString, 1, ilCharStrLen(idString));	// Software ID
	for (i = 0; i < 41 - ilCharStrLen(idString); i++) {
		context->impl->iputc(context, 0);
	}
	SaveLittleUShort(context, IL_VERSION);  // Software version
	context->impl->iputc(context, ' ');  // Release letter (not beta anymore, so use a space)
	
	SaveLittleUInt(context, 0);	// Key colour
	SaveLittleUInt(context, 0);	// Pixel aspect ratio
	SaveLittleUInt(context, 0);	// Gamma correction offset
	SaveLittleUInt(context, 0);	// Colour correction offset
	SaveLittleUInt(context, 0);	// Postage stamp offset
	SaveLittleUInt(context, 0);	// Scan line offset
	context->impl->iputc(context, 3);  // Attributes type
	
	// Write the footer.
	SaveLittleUInt(context, ExtOffset);	// No extension area
	SaveLittleUInt(context, 0);	// No developer directory
	context->impl->iwrite(context, Footer, 1, ilCharStrLen(Footer)+1);
	
	if (TempImage->Origin != IL_ORIGIN_LOWER_LEFT) {
		ifree(TempData);
	}
	if (Format == IL_RGB || Format == IL_RGBA) {
		ilSwapColours(context);
	}
	
	if (TempPal != &context->impl->iCurImage->Pal && TempPal != NULL) {
		ifree(TempPal->Palette);
		ifree(TempPal);
	}
	
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}

// Only to be called by ilDetermineSize.  Returns the buffer size needed to save the
//  current image as a Targa file.
ILuint TargaHandler::size()
{
	ILuint	Size, Bpp;
	ILubyte	IDLen = 0;
	const char	*ID = iGetString(context, IL_TGA_ID_STRING);
	const char	*AuthName = iGetString(context, IL_TGA_AUTHNAME_STRING);
	const char	*AuthComment = iGetString(context, IL_TGA_AUTHCOMMENT_STRING);

	//@TODO: Support color indexed images.
	if (iGetInt(context, IL_TGA_RLE) == IL_TRUE || context->impl->iCurImage->Format == IL_COLOUR_INDEX) {
		// Use the slower method, since we are using compression.  We do a "fake" write.
		saveL(NULL, 0);
	}

	if (ID)
		IDLen = (ILubyte)ilCharStrLen(ID);

	Size = 18 + IDLen;  // Header + ID

	// Bpp may not be context->impl->iCurImage->Bpp.
	switch (context->impl->iCurImage->Format)
	{
		case IL_BGR:
		case IL_RGB:
			Bpp = 3;
			break;
		case IL_BGRA:
		case IL_RGBA:
			Bpp = 4;
			break;
		case IL_LUMINANCE:
			Bpp = 1;
			break;
		default:  //@TODO: Do not know what to do with the others yet.
			return 0;
	}

	Size += context->impl->iCurImage->Width * context->impl->iCurImage->Height * Bpp;
	Size += 532;  // Size of the extension area

	return Size;
}

/*// Makes a neat string to go into the id field of the .tga
void iMakeString(char *Str)
{
	char		*PSG = "Generated by Developer's Image Library: ";
	char		TimeStr[255];
	
	time_t		Time;
	struct tm	*CurTime;
	
	time(&Time);
#ifdef _WIN32
	_tzset();
#endif
	CurTime = localtime(&Time);
	
	strftime(TimeStr, 255 - ilCharStrLen(PSG), "%#c (%z)", CurTime);
	//strftime(TimeStr, 255 - ilCharStrLen(PSG), "%C (%Z)", CurTime);
	sprintf(Str, "%s%s", PSG, TimeStr);
	
	return;
}*/

//changed name to iGetDateTime on 20031221 to fix bug 830196
void iGetDateTime(ILuint *Month, ILuint *Day, ILuint *Yr, ILuint *Hr, ILuint *Min, ILuint *Sec)
{
#ifdef DJGPP	
	struct date day;
	struct time curtime;
	
	gettime(&curtime);
	getdate(&day);
	
	*Month = day.da_mon;
	*Day = day.da_day;
	*Yr = day.da_year;
	
	*Hr = curtime.ti_hour;
	*Min = curtime.ti_min;
	*Sec = curtime.ti_sec;
	
	return;
#else
	
#ifdef _WIN32
	SYSTEMTIME Time;
	
	GetSystemTime(&Time);
	
	*Month = Time.wMonth;
	*Day = Time.wDay;
	*Yr = Time.wYear;
	
	*Hr = Time.wHour;
	*Min = Time.wMinute;
	*Sec = Time.wSecond;
	
	return;
#else
	
	*Month = 0;
	*Day = 0;
	*Yr = 0;
	
	*Hr = 0;
	*Min = 0;
	*Sec = 0;
	
	return;
#endif
#endif
}

#endif//IL_NO_TGA