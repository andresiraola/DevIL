//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_sgi.cpp
//
// Description: Reads and writes Silicon Graphics Inc. (.sgi) files.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_SGI
#include "il_sgi.h"
#include <limits.h>

static char *FName = NULL;

/*----------------------------------------------------------------------------*/

/*! Checks if the file specified in FileName is a valid .sgi file. */
ILboolean ilIsValidSgi(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	SgiFile;
	ILboolean	bSgi = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("sgi"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bSgi;
	}

	FName = (char*)FileName;

	SgiFile = context->impl->iopenr(FileName);
	if (SgiFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bSgi;
	}

	bSgi = ilIsValidSgiF(context, SgiFile);
	context->impl->icloser(SgiFile);

	return bSgi;
}

/*----------------------------------------------------------------------------*/

/*! Checks if the ILHANDLE contains a valid .sgi file at the current position.*/
ILboolean ilIsValidSgiF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidSgi(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Checks if Lump is a valid .sgi lump.
ILboolean ilIsValidSgiL(ILcontext* context, const void *Lump, ILuint Size)
{
	FName = NULL;
	iSetInputLump(context, Lump, Size);
	return iIsValidSgi(context);
}

/*----------------------------------------------------------------------------*/

// Internal function used to get the .sgi header from the current file.
ILboolean iGetSgiHead(ILcontext* context, iSgiHeader *Header)
{
	Header->MagicNum = GetBigUShort(context);
	Header->Storage = (ILbyte)context->impl->igetc(context);
	Header->Bpc = (ILbyte)context->impl->igetc(context);
	Header->Dim = GetBigUShort(context);
	Header->XSize = GetBigUShort(context);
	Header->YSize = GetBigUShort(context);
	Header->ZSize = GetBigUShort(context);
	Header->PixMin = GetBigInt(context);
	Header->PixMax = GetBigInt(context);
	Header->Dummy1 = GetBigInt(context);
	context->impl->iread(context, Header->Name, 1, 80);
	Header->ColMap = GetBigInt(context);
	context->impl->iread(context, Header->Dummy, 1, 404);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/* Internal function to get the header and check it. */
ILboolean iIsValidSgi(ILcontext* context)
{
	iSgiHeader	Head;

	if (!iGetSgiHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(iSgiHeader), IL_SEEK_CUR);  // Go ahead and restore to previous state

	return iCheckSgi(&Head);
}

/*----------------------------------------------------------------------------*/

/* Internal function used to check if the HEADER is a valid .sgi header. */
ILboolean iCheckSgi(iSgiHeader *Header)
{
	if (Header->MagicNum != SGI_MAGICNUM)
		return IL_FALSE;
	if (Header->Storage != SGI_RLE && Header->Storage != SGI_VERBATIM)
		return IL_FALSE;
	if (Header->Bpc == 0 || Header->Dim == 0)
		return IL_FALSE;
	if (Header->XSize == 0 || Header->YSize == 0 || Header->ZSize == 0)
		return IL_FALSE;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

/*! Reads a SGI file */
ILboolean ilLoadSgi(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	SgiFile;
	ILboolean	bSgi = IL_FALSE;

	SgiFile = context->impl->iopenr(FileName);
	if (SgiFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bSgi;
	}

	bSgi = ilLoadSgiF(context, SgiFile);
	context->impl->icloser(SgiFile);

	return bSgi;
}

/*----------------------------------------------------------------------------*/

/*! Reads an already-opened SGI file */
ILboolean ilLoadSgiF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadSgiInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

/*! Reads from a memory "lump" that contains a SGI image */
ILboolean ilLoadSgiL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadSgiInternal(context);
}

/*----------------------------------------------------------------------------*/

/* Internal function used to load the SGI image */
ILboolean iLoadSgiInternal(ILcontext* context)
{
	iSgiHeader	Header;
	ILboolean	bSgi;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetSgiHead(context, &Header))
		return IL_FALSE;
	if (!iCheckSgi(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Bugfix for #1060946.
	//  The ZSize should never really be 2 by the specifications.  Some
	//  application is outputting these, and it looks like the ZSize
	//  should really be 1.
	if (Header.ZSize == 2)
		Header.ZSize = 1;
	
	if (Header.Storage == SGI_RLE) {  // RLE
		bSgi = iReadRleSgi(context, &Header);
	}
	else {  // Non-RLE  //(Header.Storage == SGI_VERBATIM)
		bSgi = iReadNonRleSgi(context, &Header);
	}

	if (!bSgi)
		return IL_FALSE;
	return ilFixImage(context);
}

/*----------------------------------------------------------------------------*/

ILboolean iReadRleSgi(ILcontext* context, iSgiHeader *Head)
{
	#ifdef __LITTLE_ENDIAN__
	ILuint ixTable;
	#endif
	ILuint 		ChanInt = 0;
	ILuint  	ixPlane, ixHeight,ixPixel, RleOff, RleLen;
	ILuint		*OffTable=NULL, *LenTable=NULL, TableSize, Cur;
	ILubyte		**TempData=NULL;

	if (!iNewSgi(context, Head))
		return IL_FALSE;

	TableSize = Head->YSize * Head->ZSize;
	OffTable = (ILuint*)ialloc(context, TableSize * sizeof(ILuint));
	LenTable = (ILuint*)ialloc(context, TableSize * sizeof(ILuint));
	if (OffTable == NULL || LenTable == NULL)
		goto cleanup_error;
	if (context->impl->iread(context, OffTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;
	if (context->impl->iread(context, LenTable, TableSize * sizeof(ILuint), 1) != 1)
		goto cleanup_error;

#ifdef __LITTLE_ENDIAN__
	// Fix the offset/len table (it's big endian format)
	for (ixTable = 0; ixTable < TableSize; ixTable++) {
		iSwapUInt(OffTable + ixTable);
		iSwapUInt(LenTable + ixTable);
	}
#endif //__LITTLE_ENDIAN__

	// We have to create a temporary buffer for the image, because SGI
	//	images are plane-separated.
	TempData = (ILubyte**)ialloc(context, Head->ZSize * sizeof(ILubyte*));
	if (TempData == NULL)
		goto cleanup_error;
	imemclear(TempData, Head->ZSize * sizeof(ILubyte*));  // Just in case ialloc fails then cleanup_error.
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		TempData[ixPlane] = (ILubyte*)ialloc(context, Head->XSize * Head->YSize * Head->Bpc);
		if (TempData[ixPlane] == NULL)
			goto cleanup_error;
	}

	// Read the Planes into the temporary memory
	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		for (ixHeight = 0, Cur = 0;	ixHeight < Head->YSize;
			ixHeight++, Cur += Head->XSize * Head->Bpc) {

			RleOff = OffTable[ixHeight + ixPlane * Head->YSize];
			RleLen = LenTable[ixHeight + ixPlane * Head->YSize];
			
			// Seeks to the offset table position
			context->impl->iseek(context, RleOff, IL_SEEK_SET);
			if (iGetScanLine(context, (TempData[ixPlane]) + (ixHeight * Head->XSize * Head->Bpc),
				Head, RleLen) != Head->XSize * Head->Bpc) {
					ilSetError(context, IL_ILLEGAL_FILE_VALUE);
					goto cleanup_error;
			}
		}
	}

	// DW: Removed on 05/25/2002.
	/*// Check if an alphaplane exists and invert it
	if (Head->ZSize == 4) {
		for (ixPixel=0; (ILint)ixPixel<Head->XSize * Head->YSize; ixPixel++) {
 			TempData[3][ixPixel] = TempData[3][ixPixel] ^ 255;
 		}	
	}*/
	
	// Assemble the image from its planes
	for (ixPixel = 0; ixPixel < context->impl->iCurImage->SizeOfData;
		ixPixel += Head->ZSize * Head->Bpc, ChanInt += Head->Bpc) {
		for (ixPlane = 0; (ILint)ixPlane < Head->ZSize * Head->Bpc;	ixPlane += Head->Bpc) {
			context->impl->iCurImage->Data[ixPixel + ixPlane] = TempData[ixPlane][ChanInt];
			if (Head->Bpc == 2)
				context->impl->iCurImage->Data[ixPixel + ixPlane + 1] = TempData[ixPlane][ChanInt + 1];
		}
	}

	#ifdef __LITTLE_ENDIAN__
	if (Head->Bpc == 2)
		sgiSwitchData(context->impl->iCurImage->Data, context->impl->iCurImage->SizeOfData);
	#endif

	ifree(OffTable);
	ifree(LenTable);

	for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
		ifree(TempData[ixPlane]);
	}
	ifree(TempData);

	return IL_TRUE;

cleanup_error:
	ifree(OffTable);
	ifree(LenTable);
	if (TempData) {
		for (ixPlane = 0; ixPlane < Head->ZSize; ixPlane++) {
			ifree(TempData[ixPlane]);
		}
		ifree(TempData);
	}

	return IL_FALSE;
}

/*----------------------------------------------------------------------------*/

ILint iGetScanLine(ILcontext* context, ILubyte *ScanLine, iSgiHeader *Head, ILuint Length)
{
	ILushort Pixel, Count;  // For current pixel
	ILuint	 BppRead = 0, CurPos = 0, Bps = Head->XSize * Head->Bpc;

	while (BppRead < Length && CurPos < Bps)
	{
		Pixel = 0;
		if (context->impl->iread(context, &Pixel, Head->Bpc, 1) != 1)
			return -1;
		
#ifndef __LITTLE_ENDIAN__
		iSwapUShort(&Pixel);
#endif

		if (!(Count = (Pixel & 0x7f)))  // If 0, line ends
			return CurPos;
		if (Pixel & 0x80) {  // If top bit set, then it is a "run"
			if (context->impl->iread(context, ScanLine, Head->Bpc, Count) != Count)
				return -1;
			BppRead += Head->Bpc * Count + Head->Bpc;
			ScanLine += Head->Bpc * Count;
			CurPos += Head->Bpc * Count;
		}
		else {
			if (context->impl->iread(context, &Pixel, Head->Bpc, 1) != 1)
				return -1;
#ifndef __LITTLE_ENDIAN__
			iSwapUShort(&Pixel);
#endif
			if (Head->Bpc == 1) {
				while (Count--) {
					*ScanLine = (ILubyte)Pixel;
					ScanLine++;
					CurPos++;
				}
			}
			else {
				while (Count--) {
					*(ILushort*)ScanLine = Pixel;
					ScanLine += 2;
					CurPos += 2;
				}
			}
			BppRead += Head->Bpc + Head->Bpc;
		}
	}

	return CurPos;
}


/*----------------------------------------------------------------------------*/

// Much easier to read - just assemble from planes, no decompression
ILboolean iReadNonRleSgi(ILcontext* context, iSgiHeader *Head)
{
	ILuint		i, c;
	// ILint		ChanInt = 0; Unused
	ILint 		ChanSize;
	ILboolean	Cache = IL_FALSE;

	if (!iNewSgi(context, Head)) {
		return IL_FALSE;
	}

	if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST) {
		Cache = IL_TRUE;
		ChanSize = Head->XSize * Head->YSize * Head->Bpc;
		iPreCache(context, ChanSize);
	}

	for (c = 0; c < context->impl->iCurImage->Bpp; c++) {
		for (i = c; i < context->impl->iCurImage->SizeOfData; i += context->impl->iCurImage->Bpp) {
			if (context->impl->iread(context, context->impl->iCurImage->Data + i, 1, 1) != 1) {
				if (Cache)
					iUnCache(context);
				return IL_FALSE;
			}
		}
	}

	if (Cache)
		iUnCache(context);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

void sgiSwitchData(ILubyte *Data, ILuint SizeOfData)
{	
	ILubyte	Temp;
	ILuint	i;
	#ifdef ALTIVEC_GCC
		i = 0;
		union {
			vector unsigned char vec;
			vector unsigned int load;
		}inversion_vector;

		inversion_vector.load  = (vector unsigned int)\
			{0x01000302,0x05040706,0x09080B0A,0x0D0C0F0E};
		while( i <= SizeOfData-16 ) {
			vector unsigned char data = vec_ld(i,Data);
			vec_perm(data,data,inversion_vector.vec);
			vec_st(data,i,Data);
			i+=16;
		}
		SizeOfData -= i;
	#endif
	for (i = 0; i < SizeOfData; i += 2) {
		Temp = Data[i];
		Data[i] = Data[i+1];
		Data[i+1] = Temp;
	}
	return;
}

/*----------------------------------------------------------------------------*/

// Just an internal convenience function for reading SGI files
ILboolean iNewSgi(ILcontext* context, iSgiHeader *Head)
{
	if (!ilTexImage(context, Head->XSize, Head->YSize, Head->Bpc, (ILubyte)Head->ZSize, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	switch (Head->ZSize)
	{
		case 1:
			context->impl->iCurImage->Format = IL_LUMINANCE;
			break;
		/*case 2:
			context->impl->iCurImage->Format = IL_LUMINANCE_ALPHA; 
			break;*/
		case 3:
			context->impl->iCurImage->Format = IL_RGB;
			break;
		case 4:
			context->impl->iCurImage->Format = IL_RGBA;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	switch (Head->Bpc)
	{
		case 1:
			if (Head->PixMin < 0)
				context->impl->iCurImage->Type = IL_BYTE;
			else
				context->impl->iCurImage->Type = IL_UNSIGNED_BYTE;
			break;
		case 2:
			if (Head->PixMin < 0)
				context->impl->iCurImage->Type = IL_SHORT;
			else
				context->impl->iCurImage->Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}

	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

//! Writes a SGI file
ILboolean ilSaveSgi(ILcontext* context, const ILstring FileName)
{
	ILHANDLE	SgiFile;
	ILuint		SgiSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	SgiFile = context->impl->iopenw(FileName);
	if (SgiFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	SgiSize = ilSaveSgiF(context, SgiFile);
	context->impl->iclosew(SgiFile);

	if (SgiSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Writes a Sgi to an already-opened file
ILuint ilSaveSgiF(ILcontext* context, ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (iSaveSgiInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


//! Writes a Sgi to a memory "lump"
ILuint ilSaveSgiL(ILcontext* context, void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (iSaveSgiInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


ILenum DetermineSgiType(ILcontext* context, ILenum Type)
{
	if (Type > IL_UNSIGNED_SHORT) {
		if (context->impl->iCurImage->Type == IL_INT)
			return IL_SHORT;
		return IL_UNSIGNED_SHORT;
	}
	return Type;
}

/*----------------------------------------------------------------------------*/

// Rle does NOT work yet.

// Internal function used to save the Sgi.
ILboolean iSaveSgiInternal(ILcontext* context)
{
	ILuint		i, c;
	ILboolean	Compress;
	ILimage		*Temp = context->impl->iCurImage;
	ILubyte		*TempData;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Format != IL_LUMINANCE
	    //while the sgi spec doesn't directly forbid rgb files with 2
	    //channels, they are quite uncommon and most apps don't support
	    //them. so convert lum_a images to rgba before writing.
	    //&& context->impl->iCurImage->Format != IL_LUMINANCE_ALPHA
	    && context->impl->iCurImage->Format != IL_RGB
	    && context->impl->iCurImage->Format != IL_RGBA) {
		if (context->impl->iCurImage->Format == IL_BGRA || context->impl->iCurImage->Format == IL_LUMINANCE_ALPHA)
			Temp = iConvertImage(context, context->impl->iCurImage, IL_RGBA, DetermineSgiType(context, context->impl->iCurImage->Type));
		else
			Temp = iConvertImage(context, context->impl->iCurImage, IL_RGB, DetermineSgiType(context, context->impl->iCurImage->Type));
	}
	else if (context->impl->iCurImage->Type > IL_UNSIGNED_SHORT) {
		Temp = iConvertImage(context, context->impl->iCurImage, context->impl->iCurImage->Format, DetermineSgiType(context, context->impl->iCurImage->Type));
	}
	
	//compression of images with 2 bytes per channel doesn't work yet
	Compress = iGetInt(context, IL_SGI_RLE) && Temp->Bpc == 1;

	if (Temp == NULL)
		return IL_FALSE;

	SaveBigUShort(context, SGI_MAGICNUM);  // 'Magic' number
	if (Compress)
		context->impl->iputc(context, 1);
	else
		context->impl->iputc(context, 0);

	if (Temp->Type == IL_UNSIGNED_BYTE)
		context->impl->iputc(context, 1);
	else if (Temp->Type == IL_UNSIGNED_SHORT)
		context->impl->iputc(context, 2);
	// Need to error here if not one of the two...

	if (Temp->Format == IL_LUMINANCE || Temp->Format == IL_COLOUR_INDEX)
		SaveBigUShort(context, 2);
	else
		SaveBigUShort(context, 3);

	SaveBigUShort(context, (ILushort)Temp->Width);
	SaveBigUShort(context, (ILushort)Temp->Height);
	SaveBigUShort(context, (ILushort)Temp->Bpp);

	switch (Temp->Type)
	{
		case IL_BYTE:
			SaveBigInt(context, SCHAR_MIN);	// Minimum pixel value
			SaveBigInt(context, SCHAR_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_BYTE:
			SaveBigInt(context, 0);			// Minimum pixel value
			SaveBigInt(context, UCHAR_MAX);	// Maximum pixel value
			break;
		case IL_SHORT:
			SaveBigInt(context, SHRT_MIN);	// Minimum pixel value
			SaveBigInt(context, SHRT_MAX);	// Maximum pixel value
			break;
		case IL_UNSIGNED_SHORT:
			SaveBigInt(context, 0);			// Minimum pixel value
			SaveBigInt(context, USHRT_MAX);	// Maximum pixel value
			break;
	}

	SaveBigInt(context, 0);  // Dummy value

	if (FName) {
		c = ilCharStrLen(FName);
		c = c < 79 ? 79 : c;
		context->impl->iwrite(context, FName, 1, c);
		c = 80 - c;
		for (i = 0; i < c; i++) {
			context->impl->iputc(context, 0);
		}
	}
	else {
		for (i = 0; i < 80; i++) {
			context->impl->iputc(context, 0);
		}
	}

	SaveBigUInt(context, 0);  // Colormap

	// Padding
	for (i = 0; i < 101; i++) {
		SaveLittleInt(context, 0);
	}


	if (context->impl->iCurImage->Origin == IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(context, Temp);
		if (TempData == NULL) {
			if (Temp!= context->impl->iCurImage)
				ilCloseImage(Temp);
			return IL_FALSE;
		}
	}
	else {
		TempData = Temp->Data;
	}


	if (!Compress) {
		for (c = 0; c < Temp->Bpp; c++) {
			for (i = c; i < Temp->SizeOfData; i += Temp->Bpp) {
				context->impl->iputc(context, TempData[i]);  // Have to save each colour plane separately.
			}
		}
	}
	else {
		iSaveRleSgi(context, TempData, Temp->Width, Temp->Height, Temp->Bpp, Temp->Bps);
	}


	if (TempData != Temp->Data)
		ifree(TempData);
	if (Temp != context->impl->iCurImage)
		ilCloseImage(Temp);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

ILboolean iSaveRleSgi(ILcontext* context, ILubyte *Data, ILuint w, ILuint h, ILuint numChannels,
		ILuint bps)
{
	//works only for sgi files with only 1 bpc

	ILuint	c, i, y, j;
	ILubyte	*ScanLine = NULL, *CompLine = NULL;
	ILuint	*StartTable = NULL, *LenTable = NULL;
	ILuint	TableOff, DataOff = 0;

	ScanLine = (ILubyte*)ialloc(context, w);
	CompLine = (ILubyte*)ialloc(context, w * 2 + 1);  // Absolute worst case.
	StartTable = (ILuint*)ialloc(context, h * numChannels * sizeof(ILuint));
	LenTable = (ILuint*)icalloc(context, h * numChannels, sizeof(ILuint));
	if (!ScanLine || !CompLine || !StartTable || !LenTable) {
		ifree(ScanLine);
		ifree(CompLine);
		ifree(StartTable);
		ifree(LenTable);
		return IL_FALSE;
	}

	// These just contain dummy values at this point.
	TableOff = context->impl->itellw(context);
	context->impl->iwrite(context, StartTable, sizeof(ILuint), h * numChannels);
	context->impl->iwrite(context, LenTable, sizeof(ILuint), h * numChannels);

	DataOff = context->impl->itellw(context);
	for (c = 0; c < numChannels; c++) {
		for (y = 0; y < h; y++) {
			i = y * bps + c;
			for (j = 0; j < w; j++, i += numChannels) {
				ScanLine[j] = Data[i];
			}

			ilRleCompressLine(context, ScanLine, w, 1, CompLine, LenTable + h * c + y, IL_SGICOMP);
			context->impl->iwrite(context, CompLine, 1, *(LenTable + h * c + y));
		}
	}

	context->impl->iseekw(context, TableOff, IL_SEEK_SET);

	j = h * numChannels;
	for (y = 0; y < j; y++) {
		StartTable[y] = DataOff;
		DataOff += LenTable[y];
#ifdef __LITTLE_ENDIAN__
		iSwapUInt(&StartTable[y]);
 		iSwapUInt(&LenTable[y]);
#endif
	}

	context->impl->iwrite(context, StartTable, sizeof(ILuint), h * numChannels);
	context->impl->iwrite(context, LenTable, sizeof(ILuint), h * numChannels);

	ifree(ScanLine);
	ifree(CompLine);
	ifree(StartTable);
	ifree(LenTable);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_SGI
