//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_psd.cpp
//
// Description: Reads and writes Photoshop (.psd) files.
//
//-----------------------------------------------------------------------------


// Information about the .psd format was taken from Adobe's PhotoShop SDK at
//  http://partners.adobe.com/asn/developer/gapsdk/PhotoshopSDK.html
//  Information about the Packbits compression scheme was found at
//	http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf

#include "il_internal.h"
#ifndef IL_NO_PSD
#include "il_psd.h"

static float ubyte_to_float(ILubyte val)
{
	return ((float)val) / 255.0f;
}
static float ushort_to_float(ILushort val)
{
	return ((float)val) / 65535.0f;
}

static ILubyte float_to_ubyte(float val)
{
	return (ILubyte)(val * 255.0f);
}
static ILushort float_to_ushort(float val)
{
	return (ILushort)(val * 65535.0f);
}


//! Checks if the file specified in FileName is a valid Psd file.
ILboolean ilIsValidPsd(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PsdFile;
	ILboolean	bPsd = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("psd")) &&
		!iCheckExtension(FileName, IL_TEXT("pdd"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bPsd;
	}

	PsdFile = context->impl->iopenr(FileName);
	if (PsdFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPsd;
	}

	bPsd = ilIsValidPsdF(context, PsdFile);
	context->impl->icloser(PsdFile);

	return bPsd;
}


//! Checks if the ILHANDLE contains a valid Psd file at the current position.
ILboolean ilIsValidPsdF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidPsd(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid Psd lump.
ILboolean ilIsValidPsdL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidPsd(context);
}


// Internal function used to get the Psd header from the current file.
ILboolean iGetPsdHead(ILcontext* context, PSDHEAD *Header)
{
	context->impl->iread(context, Header->Signature, 1, 4);
	Header->Version = GetBigUShort(context);
	context->impl->iread(context, Header->Reserved, 1, 6);
	Header->Channels = GetBigUShort(context);
	Header->Height = GetBigUInt(context);
	Header->Width = GetBigUInt(context);
	Header->Depth = GetBigUShort(context);
	Header->Mode = GetBigUShort(context);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPsd(ILcontext* context)
{
	PSDHEAD	Head;

	iGetPsdHead(context, &Head);
	context->impl->iseek(context, -(ILint)sizeof(PSDHEAD), IL_SEEK_CUR);

	return iCheckPsd(&Head);
}


// Internal function used to check if the HEADER is a valid Psd header.
ILboolean iCheckPsd(PSDHEAD *Header)
{
	ILuint i;

	if (strncmp((char*)Header->Signature, "8BPS", 4))
		return IL_FALSE;
	if (Header->Version != 1)
		return IL_FALSE;
	for (i = 0; i < 6; i++) {
		if (Header->Reserved[i] != 0)
			return IL_FALSE;
	}
	if (Header->Channels < 1 || Header->Channels > 24)
		return IL_FALSE;
	if (Header->Height < 1 || Header->Width < 1)
		return IL_FALSE;
	if (Header->Depth != 1 && Header->Depth != 8 && Header->Depth != 16)
		return IL_FALSE;

	return IL_TRUE;
}


//! Reads a Psd file
ILboolean ilLoadPsd(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PsdFile;
	ILboolean	bPsd = IL_FALSE;

	PsdFile = context->impl->iopenr(FileName);
	if (PsdFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPsd;
	}

	bPsd = ilLoadPsdF(context, PsdFile);
	context->impl->icloser(PsdFile);

	return bPsd;
}


//! Reads an already-opened Psd file
ILboolean ilLoadPsdF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadPsdInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Psd
ILboolean ilLoadPsdL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadPsdInternal(context);
}


// Internal function used to load the Psd.
ILboolean iLoadPsdInternal(ILcontext* context)
{
	PSDHEAD	Header;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iGetPsdHead(context, &Header);
	if (!iCheckPsd(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadPsd(context, &Header))
		return IL_FALSE;
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return ilFixImage(context);
}


ILboolean ReadPsd(ILcontext* context, PSDHEAD *Head)
{
	switch (Head->Mode)
	{
		case 1:  // Greyscale
			return ReadGrey(context, Head);
		case 2:  // Indexed
			return ReadIndexed(context, Head);
		case 3:  // RGB
			return ReadRGB(context, Head);
		case 4:  // CMYK
			return ReadCMYK(context, Head);
	}

	ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
	return IL_FALSE;
}


ILboolean ReadGrey(ILcontext* context, PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Type;
	ILubyte		*Resources = NULL;

	ColorMode = GetBigUInt(context);  // Skip over the 'color mode data section'
	context->impl->iseek(context, ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt(context);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(context, ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (context->impl->iread(context, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(context);
	context->impl->iseek(context, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(context);

	ChannelNum = Head->Channels;
	Head->Channels = 1;  // Temporary to read only one channel...some greyscale .psd files have 2.
	if (Head->Channels != 1) {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!ilTexImage(context, Head->Width, Head->Height, 1, 1, IL_LUMINANCE, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(context, Head, context->impl->iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;
	if (!ParseResources(context, ResourceSize, Resources))
		goto cleanup_error;
	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}


ILboolean ReadIndexed(ILcontext* context, PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo, i, j, NumEnt;
	ILushort	Compressed;
	ILubyte		*Palette = NULL, *Resources = NULL;

	ColorMode = GetBigUInt(context);  // Skip over the 'color mode data section'
	if (ColorMode % 3 != 0) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	Palette = (ILubyte*)ialloc(context, ColorMode);
	if (Palette == NULL)
		return IL_FALSE;
	if (context->impl->iread(context, Palette, 1, ColorMode) != ColorMode)
		goto cleanup_error;

	ResourceSize = GetBigUInt(context);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(context, ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (context->impl->iread(context, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(context);
	if (context->impl->ieof(context))
		goto cleanup_error;
	context->impl->iseek(context, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(context);
	if (context->impl->ieof(context))
		goto cleanup_error;

	if (Head->Channels != 1 || Head->Depth != 8) {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		goto cleanup_error;
	}
	ChannelNum = Head->Channels;

	if (!ilTexImage(context, Head->Width, Head->Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		goto cleanup_error;

	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, ColorMode);
	if (context->impl->iCurImage->Pal.Palette == NULL) {
		goto cleanup_error;
	}
	context->impl->iCurImage->Pal.PalSize = ColorMode;
	context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;

	NumEnt = context->impl->iCurImage->Pal.PalSize / 3;
	for (i = 0, j = 0; i < context->impl->iCurImage->Pal.PalSize; i += 3, j++) {
		context->impl->iCurImage->Pal.Palette[i  ] = Palette[j];
		context->impl->iCurImage->Pal.Palette[i+1] = Palette[j+NumEnt];
		context->impl->iCurImage->Pal.Palette[i+2] = Palette[j+NumEnt*2];
	}
	ifree(Palette);
	Palette = NULL;

	if (!PsdGetData(context, Head, context->impl->iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;

	ParseResources(context, ResourceSize, Resources);
	ifree(Resources);
	Resources = NULL;

	return IL_TRUE;

cleanup_error:
	ifree(Palette);
	ifree(Resources);

	return IL_FALSE;
}


ILboolean ReadRGB(ILcontext* context, PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte		*Resources = NULL;

	ColorMode = GetBigUInt(context);  // Skip over the 'color mode data section'
	context->impl->iseek(context, ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt(context);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(context, ResourceSize);
	if (Resources == NULL)
		return IL_FALSE;
	if (context->impl->iread(context, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(context);
	context->impl->iseek(context, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(context);

	ChannelNum = Head->Channels;
	if (Head->Channels == 3)
 	{
		Format = IL_RGB;
	}
	else if (Head->Channels == 4)
	{
		Format = IL_RGBA;
	}
	else if (Head->Channels >= 5)
	{
		// Additional channels are accumulated as a single alpha channel, since
		// if an image does not have a layer set as the "background", but also
		// has a real alpha channel, there will be 5 channels (or more).
		Format = IL_RGBA;
	}
	else
	{
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	if (!ilTexImage(context, Head->Width, Head->Height, 1, (Format==IL_RGB) ? 3 : 4, Format, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(context, Head, context->impl->iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;
	if (!ParseResources(context, ResourceSize, Resources))
		goto cleanup_error;
	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}


ILboolean ReadCMYK(ILcontext* context, PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo, Size, i, j;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte		*Resources = NULL, *KChannel = NULL;

	ColorMode = GetBigUInt(context);  // Skip over the 'color mode data section'
	context->impl->iseek(context, ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt(context);  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(context, ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (context->impl->iread(context, Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt(context);
	context->impl->iseek(context, MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort(context);

	switch (Head->Channels)
	{
		case 4:
			Format = IL_RGB;
			ChannelNum = 4;
			Head->Channels = 3;
			break;
		case 5:
			Format = IL_RGBA;
			ChannelNum = 5;
			Head->Channels = 4;
			break;
		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	if (!ilTexImage(context, Head->Width, Head->Height, 1, (ILubyte)Head->Channels, Format, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(context, Head, context->impl->iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;

	Size = context->impl->iCurImage->Bpc * context->impl->iCurImage->Width * context->impl->iCurImage->Height;
	KChannel = (ILubyte*)ialloc(context, Size);
	if (KChannel == NULL)
		goto cleanup_error;
	if (!GetSingleChannel(context, Head, KChannel, (ILboolean)Compressed))
		goto cleanup_error;

	if (Format == IL_RGB) {
		for (i = 0, j = 0; i < context->impl->iCurImage->SizeOfData; i += 3, j++) {
			context->impl->iCurImage->Data[i  ] = (context->impl->iCurImage->Data[i  ] * KChannel[j]) >> 8;
			context->impl->iCurImage->Data[i+1] = (context->impl->iCurImage->Data[i+1] * KChannel[j]) >> 8;
			context->impl->iCurImage->Data[i+2] = (context->impl->iCurImage->Data[i+2] * KChannel[j]) >> 8;
		}
	}
	else {  // IL_RGBA
		// The KChannel array really holds the alpha channel on this one.
		for (i = 0, j = 0; i < context->impl->iCurImage->SizeOfData; i += 4, j++) {
			context->impl->iCurImage->Data[i  ] = (context->impl->iCurImage->Data[i  ] * context->impl->iCurImage->Data[i+3]) >> 8;
			context->impl->iCurImage->Data[i+1] = (context->impl->iCurImage->Data[i+1] * context->impl->iCurImage->Data[i+3]) >> 8;
			context->impl->iCurImage->Data[i+2] = (context->impl->iCurImage->Data[i+2] * context->impl->iCurImage->Data[i+3]) >> 8;
			context->impl->iCurImage->Data[i+3] = KChannel[j];  // Swap 'K' with alpha channel.
		}
	}

	if (!ParseResources(context, ResourceSize, Resources))
		goto cleanup_error;

	ifree(Resources);
	ifree(KChannel);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	ifree(KChannel);
	return IL_FALSE;
}


ILuint *GetCompChanLen(ILcontext* context, PSDHEAD *Head)
{
	ILushort	*RleTable;
	ILuint		*ChanLen, c, i, j;

	RleTable = (ILushort*)ialloc(context, Head->Height * ChannelNum * sizeof(ILushort));
	ChanLen = (ILuint*)ialloc(context, ChannelNum * sizeof(ILuint));
	if (RleTable == NULL || ChanLen == NULL) {
		return NULL;
	}

	if (context->impl->iread(context, RleTable, sizeof(ILushort), Head->Height * ChannelNum) != Head->Height * ChannelNum) {
		ifree(RleTable);
		ifree(ChanLen);
		return NULL;
	}
#ifdef __LITTLE_ENDIAN__
	for (i = 0; i < Head->Height * ChannelNum; i++) {
		iSwapUShort(&RleTable[i]);
	}
#endif

	imemclear(ChanLen, ChannelNum * sizeof(ILuint));
	for (c = 0; c < ChannelNum; c++) {
		j = c * Head->Height;
		for (i = 0; i < Head->Height; i++) {
			ChanLen[c] += RleTable[i + j];
		}
	}

	ifree(RleTable);

	return ChanLen;
}



static const ILuint READ_COMPRESSED_SUCCESS					= 0;
static const ILuint READ_COMPRESSED_ERROR_FILE_CORRUPT		= 1;
static const ILuint READ_COMPRESSED_ERROR_FILE_READ_ERROR	= 2;

static ILuint ReadCompressedChannel(ILcontext* context, const ILuint ChanLen, ILuint Size, ILubyte* Channel)
{
	ILuint		i;
	ILint		Run;
	ILboolean	PreCache = IL_FALSE;
	ILbyte		HeadByte;

	if (iGetHint(context, IL_MEM_SPEED_HINT) == IL_FASTEST)
		PreCache = IL_TRUE;

	if (PreCache)
		iPreCache(context, ChanLen);
	for (i = 0; i < Size; ) {
		HeadByte = context->impl->igetc(context);

		if (HeadByte >= 0) {  //  && HeadByte <= 127
			if (i + HeadByte > Size)
			{
				if (PreCache)
					iUnCache(context);
				return READ_COMPRESSED_ERROR_FILE_CORRUPT;
			}
			if (context->impl->iread(context, Channel + i, HeadByte + 1, 1) != 1)
			{
				if (PreCache)
					iUnCache(context);
				return READ_COMPRESSED_ERROR_FILE_READ_ERROR;
			}

			i += HeadByte + 1;
		}
		if (HeadByte >= -127 && HeadByte <= -1) {
			Run = context->impl->igetc(context);
			if (Run == IL_EOF)
			{
				if (PreCache)
					iUnCache(context);
				return READ_COMPRESSED_ERROR_FILE_READ_ERROR;
			}
			if (i + (-HeadByte + 1) > Size)
			{
				if (PreCache)
					iUnCache(context);
				return READ_COMPRESSED_ERROR_FILE_CORRUPT;
			}

			memset(Channel + i, Run, -HeadByte + 1);
			i += -HeadByte + 1;
		}
		if (HeadByte == -128)
		{ }  // Noop
	}
	if (PreCache)
		iUnCache(context);

	return READ_COMPRESSED_SUCCESS;
}


ILboolean PsdGetData(ILcontext* context, PSDHEAD *Head, void *Buffer, ILboolean Compressed)
{
	ILuint		c, x, y, i, Size, ReadResult, NumChan;
	ILubyte		*Channel = NULL;
	ILushort	*ShortPtr;
	ILuint		*ChanLen = NULL;

	// Added 01-07-2009: This is needed to correctly load greyscale and
	//  paletted images.
	switch (Head->Mode)
	{
		case 1:
		case 2:
			NumChan = 1;
			break;
		default:
			NumChan = 3;
	}

	Channel = (ILubyte*)ialloc(context, Head->Width * Head->Height * context->impl->iCurImage->Bpc);
	if (Channel == NULL) {
		return IL_FALSE;
	}
	ShortPtr = (ILushort*)Channel;

	// @TODO: Add support for this in, though I have yet to run across a .psd
	//	file that uses this.
	if (Compressed && context->impl->iCurImage->Type == IL_UNSIGNED_SHORT) {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	if (!Compressed) {
		if (context->impl->iCurImage->Bpc == 1) {
			for (c = 0; c < NumChan; c++) {
				i = 0;
				if (context->impl->iread(context, Channel, Head->Width * Head->Height, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
					for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
						context->impl->iCurImage->Data[y + x + c] = Channel[i];
					}
				}
			}
			// Accumulate any remaining channels into a single alpha channel
			//@TODO: This needs to be changed for greyscale images.
			for (; c < Head->Channels; c++) {
				i = 0;
				if (context->impl->iread(context, Channel, Head->Width * Head->Height, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
					for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
						float curVal = ubyte_to_float(context->impl->iCurImage->Data[y + x + 3]);
						float newVal = ubyte_to_float(Channel[i]);
						context->impl->iCurImage->Data[y + x + 3] = float_to_ubyte(curVal * newVal);
					}
				}
			}
		}
		else {  // context->impl->iCurImage->Bpc == 2
			for (c = 0; c < NumChan; c++) {
				i = 0;
				if (context->impl->iread(context, Channel, Head->Width * Head->Height * 2, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				context->impl->iCurImage->Bps /= 2;
				for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
					for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
					 #ifndef WORDS_BIGENDIAN
						iSwapUShort(ShortPtr+i);
					 #endif
						((ILushort*)context->impl->iCurImage->Data)[y + x + c] = ShortPtr[i];
					}
				}
				context->impl->iCurImage->Bps *= 2;
			}
			// Accumulate any remaining channels into a single alpha channel
			//@TODO: This needs to be changed for greyscale images.
			for (; c < Head->Channels; c++) {
				i = 0;
				if (context->impl->iread(context, Channel, Head->Width * Head->Height * 2, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				context->impl->iCurImage->Bps /= 2;
				for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
					for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
						float curVal = ushort_to_float(((ILushort*)context->impl->iCurImage->Data)[y + x + 3]);
						float newVal = ushort_to_float(ShortPtr[i]);
						((ILushort*)context->impl->iCurImage->Data)[y + x + 3] = float_to_ushort(curVal * newVal);
					}
				}
				context->impl->iCurImage->Bps *= 2;
			}
		}
	}
	else {
		ChanLen = GetCompChanLen(context, Head);

		Size = Head->Width * Head->Height;
		for (c = 0; c < NumChan; c++) {
			ReadResult = ReadCompressedChannel(context, ChanLen[c], Size, Channel);
			if (ReadResult == READ_COMPRESSED_ERROR_FILE_CORRUPT)
				goto file_corrupt;
			else if (ReadResult == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
				goto file_read_error;

			i = 0;
			for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
				for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
					context->impl->iCurImage->Data[y + x + c] = Channel[i];
				}
			}
		}

		// Initialize the alpha channel to solid
		//@TODO: This needs to be changed for greyscale images.
		if (Head->Channels >= 4) {
			for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
				for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp) {
					context->impl->iCurImage->Data[y + x + 3] = 255;
				}
			}
					
			for (; c < Head->Channels; c++) {
				ReadResult = ReadCompressedChannel(context, ChanLen[c], Size, Channel);
				if (ReadResult == READ_COMPRESSED_ERROR_FILE_CORRUPT)
					goto file_corrupt;
				else if (ReadResult == READ_COMPRESSED_ERROR_FILE_READ_ERROR)
					goto file_read_error;

				i = 0;
				for (y = 0; y < Head->Height * context->impl->iCurImage->Bps; y += context->impl->iCurImage->Bps) {
					for (x = 0; x < context->impl->iCurImage->Bps; x += context->impl->iCurImage->Bpp, i++) {
						float curVal = ubyte_to_float(context->impl->iCurImage->Data[y + x + 3]);
						float newVal = ubyte_to_float(Channel[i]);
						context->impl->iCurImage->Data[y + x + 3] = float_to_ubyte(curVal * newVal);
					}
				}
			}
		}

		ifree(ChanLen);
	}

	ifree(Channel);

	return IL_TRUE;

file_corrupt:
	ifree(ChanLen);
	ifree(Channel);
	ilSetError(context, IL_ILLEGAL_FILE_VALUE);
	return IL_FALSE;

file_read_error:
	ifree(ChanLen);
	ifree(Channel);
	return IL_FALSE;
}


ILboolean ParseResources(ILcontext* context, ILuint ResourceSize, ILubyte *Resources)
{
	ILushort	ID;
	ILubyte		NameLen;
	ILuint		Size;

	if (Resources == NULL) {
		ilSetError(context, IL_INTERNAL_ERROR);
		return IL_FALSE;
	}

	while (ResourceSize > 13) {  // Absolutely has to be larger than this.
		if (strncmp("8BIM", (const char*)Resources, 4)) {
			//return IL_FALSE;
			return IL_TRUE;  // 05-30-2002: May not necessarily mean corrupt data...
		}
		Resources += 4;

		ID = *((ILushort*)Resources);
		BigUShort(&ID);
		Resources += 2;

		NameLen = *Resources++;
		// NameLen + the byte it occupies must be padded to an even number, so NameLen must be odd.
		NameLen = NameLen + (NameLen & 1 ? 0 : 1);
		Resources += NameLen;

		// Get the resource data size.
		Size = *((ILuint*)Resources);
		BigUInt(&Size);
		Resources += 4;

		ResourceSize -= (4 + 2 + 1 + NameLen + 4);

		switch (ID)
		{
			case 0x040F:  // ICC Profile
				if (Size > ResourceSize) {  // Check to make sure we are not going past the end of Resources.
					ilSetError(context, IL_ILLEGAL_FILE_VALUE);
					return IL_FALSE;
				}
				context->impl->iCurImage->Profile = (ILubyte*)ialloc(context, Size);
				if (context->impl->iCurImage->Profile == NULL) {
					return IL_FALSE;
				}
				memcpy(context->impl->iCurImage->Profile, Resources, Size);
				context->impl->iCurImage->ProfileSize = Size;
				break;

			default:
				break;
		}

		if (Size & 1)  // Must be an even number.
			Size++;
		ResourceSize -= Size;
		Resources += Size;
	}

	return IL_TRUE;
}


ILboolean GetSingleChannel(ILcontext* context, PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed)
{
	ILuint		i;
	ILushort	*ShortPtr;
	ILbyte		HeadByte;
	ILint		Run;

	ShortPtr = (ILushort*)Buffer;

	if (!Compressed) {
		if (context->impl->iCurImage->Bpc == 1) {
			if (context->impl->iread(context, Buffer, Head->Width * Head->Height, 1) != 1)
				return IL_FALSE;
		}
		else {  // context->impl->iCurImage->Bpc == 2
			if (context->impl->iread(context, Buffer, Head->Width * Head->Height * 2, 1) != 1)
				return IL_FALSE;
		}
	}
	else {
		for (i = 0; i < Head->Width * Head->Height; ) {
			HeadByte = context->impl->igetc(context);

			if (HeadByte >= 0) {  //  && HeadByte <= 127
				if (context->impl->iread(context, Buffer + i, HeadByte + 1, 1) != 1)
					return IL_FALSE;
				i += HeadByte + 1;
			}
			if (HeadByte >= -127 && HeadByte <= -1) {
				Run = context->impl->igetc(context);
				if (Run == IL_EOF)
					return IL_FALSE;
				memset(Buffer + i, Run, -HeadByte + 1);
				i += -HeadByte + 1;
			}
			if (HeadByte == -128)
			{ }  // Noop
		}
	}

	return IL_TRUE;
}



//! Writes a Psd file
ILboolean ilSavePsd(ILcontext* context, const ILstring FileName)
{
	ILHANDLE	PsdFile;
	ILuint		PsdSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	PsdFile = context->impl->iopenw(FileName);
	if (PsdFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	PsdSize = ilSavePsdF(context, PsdFile);
	context->impl->iclosew(PsdFile);

	if (PsdSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Writes a Psd to an already-opened file
ILuint ilSavePsdF(ILcontext* context, ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (iSavePsdInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


//! Writes a Psd to a memory "lump"
ILuint ilSavePsdL(ILcontext* context, void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (iSavePsdInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


// Internal function used to save the Psd.
ILboolean iSavePsdInternal(ILcontext* context)
{
	ILubyte		*Signature = (ILubyte*)"8BPS";
	ILimage		*TempImage;
	ILpal		*TempPal;
	ILuint		c, i;
	ILubyte		*TempData;
	ILushort	*ShortPtr;
	ILenum		Format, Type;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Format = context->impl->iCurImage->Format;
	Type = context->impl->iCurImage->Type;

	// All of these comprise the actual signature.
	context->impl->iwrite(context, Signature, 1, 4);
	SaveBigShort(context, 1);
	SaveBigInt(context, 0);
	SaveBigShort(context, 0);

	SaveBigShort(context, context->impl->iCurImage->Bpp);
	SaveBigInt(context, context->impl->iCurImage->Height);
	SaveBigInt(context, context->impl->iCurImage->Width);
	if (context->impl->iCurImage->Bpc > 2)
		Type = IL_UNSIGNED_SHORT;

	if (context->impl->iCurImage->Format == IL_BGR)
		Format = IL_RGB;
	else if (context->impl->iCurImage->Format == IL_BGRA)
		Format = IL_RGBA;

	if (Format != context->impl->iCurImage->Format || Type != context->impl->iCurImage->Type) {
		TempImage = iConvertImage(context, context->impl->iCurImage, Format, Type);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else {
		TempImage = context->impl->iCurImage;
	}
	SaveBigShort(context, (ILushort)(TempImage->Bpc * 8));

	// @TODO:  Put the other formats here.
	switch (TempImage->Format)
	{
		case IL_COLOUR_INDEX:
			SaveBigShort(context, 2);
			break;
		case IL_LUMINANCE:
			SaveBigShort(context, 1);
			break;
		case IL_RGB:
		case IL_RGBA:
			SaveBigShort(context, 3);
			break;
		default:
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	if (TempImage->Format == IL_COLOUR_INDEX) {
		// @TODO: We're currently making a potentially fatal assumption that
		//	iConvertImage was not called if the format is IL_COLOUR_INDEX.
		TempPal = iConvertPal(context, &TempImage->Pal, IL_PAL_RGB24);
		if (TempPal == NULL)
			return IL_FALSE;
		SaveBigInt(context, 768);

		// Have to save the palette in a planar format.
		for (c = 0; c < 3; c++) {
			for (i = c; i < TempPal->PalSize; i += 3) {
				context->impl->iputc(context, TempPal->Palette[i]);
			}
		}

		ifree(TempPal->Palette);
	}
	else {
		SaveBigInt(context, 0);  // No colour mode data.
	}

	SaveBigInt(context, 0);  // No image resources.
	SaveBigInt(context, 0);  // No layer information.
	SaveBigShort(context, 0);  // Psd data, no compression.

	// @TODO:  Add RLE compression.

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	if (TempImage->Bpc == 1) {
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				context->impl->iputc(context, TempData[i]);
			}
		}
	}
	else {  // TempImage->Bpc == 2
		ShortPtr = (ILushort*)TempData;
		TempImage->SizeOfPlane /= 2;
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				SaveBigUShort(context, ShortPtr[i]);
			}
		}
		TempImage->SizeOfPlane *= 2;
	}

	if (TempData != TempImage->Data)
		ifree(TempData);

	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);


	return IL_TRUE;
}


#endif//IL_NO_PSD
