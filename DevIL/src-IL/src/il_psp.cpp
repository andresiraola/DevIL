//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_psp.cpp
//
// Description: Reads a Paint Shop Pro file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_psp.h"
#ifndef IL_NO_PSP


ILubyte PSPSignature[32] = {
	0x50, 0x61, 0x69, 0x6E, 0x74, 0x20, 0x53, 0x68, 0x6F, 0x70, 0x20, 0x50, 0x72, 0x6F, 0x20, 0x49,
	0x6D, 0x61, 0x67, 0x65, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x0A, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00
};

ILubyte GenAttHead[4] = {
	0x7E, 0x42, 0x4B, 0x00
};


// Make these global, since they contain most of the image information.
GENATT_CHUNK	AttChunk;
PSPHEAD			Header;
ILuint			NumChannels;
ILubyte			**Channels = NULL;
ILubyte			*Alpha = NULL;
ILpal			Pal;



//! Checks if the file specified in FileName is a valid Psp file.
ILboolean ilIsValidPsp(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PspFile;
	ILboolean	bPsp = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("psp"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bPsp;
	}

	PspFile = context->impl->iopenr(FileName);
	if (PspFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPsp;
	}

	bPsp = ilIsValidPspF(context, PspFile);
	context->impl->icloser(PspFile);

	return bPsp;
}


//! Checks if the ILHANDLE contains a valid Psp file at the current position.
ILboolean ilIsValidPspF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidPsp(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid Psp lump.
ILboolean ilIsValidPspL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidPsp(context);
}


// Internal function used to get the Psp header from the current file.
ILboolean iGetPspHead(ILcontext* context)
{
	if (context->impl->iread(context, Header.FileSig, 1, 32) != 32)
		return IL_FALSE;
	Header.MajorVersion = GetLittleUShort(context);
	Header.MinorVersion = GetLittleUShort(context);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPsp(ILcontext* context)
{
	if (!iGetPspHead(context))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(PSPHEAD), IL_SEEK_CUR);

	return iCheckPsp();
}


// Internal function used to check if the HEADER is a valid Psp header.
ILboolean iCheckPsp()
{
	if (stricmp(Header.FileSig, "Paint Shop Pro Image File\n\x1a"))
		return IL_FALSE;
	if (Header.MajorVersion < 3 || Header.MajorVersion > 5)
		return IL_FALSE;
	if (Header.MinorVersion != 0)
		return IL_FALSE;


	return IL_TRUE;
}


//! Reads a PSP file
ILboolean ilLoadPsp(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PSPFile;
	ILboolean	bPsp = IL_FALSE;

	PSPFile = context->impl->iopenr(FileName);
	if (PSPFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPsp;
	}

	bPsp = ilLoadPspF(context, PSPFile);
	context->impl->icloser(PSPFile);

	return bPsp;
}


//! Reads an already-opened PSP file
ILboolean ilLoadPspF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadPspInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a PSP
ILboolean ilLoadPspL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadPspInternal(context);
}


// Internal function used to load the PSP.
ILboolean iLoadPspInternal(ILcontext* context)
{
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Channels = NULL;
	Alpha = NULL;
	Pal.Palette = NULL;

	if (!iGetPspHead(context))
		return IL_FALSE;
	if (!iCheckPsp()) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadGenAttributes(context))
		return IL_FALSE;
	if (!ParseChunks(context))
		return IL_FALSE;
	if (!AssembleImage(context))
		return IL_FALSE;

	Cleanup();
	return ilFixImage(context);
}


ILboolean ReadGenAttributes(ILcontext* context)
{
	BLOCKHEAD		AttHead;
	ILint			Padding;
	ILuint			ChunkLen;

	if (context->impl->iread(context, &AttHead, sizeof(AttHead), 1) != 1)
		return IL_FALSE;
	UShort(&AttHead.BlockID);
	UInt(&AttHead.BlockLen);

	if (AttHead.HeadID[0] != 0x7E || AttHead.HeadID[1] != 0x42 ||
		AttHead.HeadID[2] != 0x4B || AttHead.HeadID[3] != 0x00) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if (AttHead.BlockID != PSP_IMAGE_BLOCK) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	ChunkLen = GetLittleUInt(context);
	if (Header.MajorVersion != 3)
		ChunkLen -= 4;
	if (context->impl->iread(context, &AttChunk, IL_MIN(sizeof(AttChunk), ChunkLen), 1) != 1)
		return IL_FALSE;

	// Can have new entries in newer versions of the spec (4.0).
	Padding = (ChunkLen) - sizeof(AttChunk);
	if (Padding > 0)
		context->impl->iseek(context, Padding, IL_SEEK_CUR);

	// @TODO:  Anything but 24 not supported yet...
	if (AttChunk.BitDepth != 24 && AttChunk.BitDepth != 8) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO;  Add support for compression...
	if (AttChunk.Compression != PSP_COMP_NONE && AttChunk.Compression != PSP_COMP_RLE) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// @TODO: Check more things in the general attributes chunk here.

	return IL_TRUE;
}


ILboolean ParseChunks(ILcontext* context)
{
	BLOCKHEAD	Block;
	ILuint		Pos;

	do {
		if (context->impl->iread(context, &Block, 1, sizeof(Block)) != sizeof(Block)) {
			ilGetError(context);  // Get rid of the erroneous IL_FILE_READ_ERROR.
			return IL_TRUE;
		}
		if (Header.MajorVersion == 3)
			Block.BlockLen = GetLittleUInt(context);
		else
			UInt(&Block.BlockLen);

		if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
			Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
				return IL_TRUE;
		}
		UShort(&Block.BlockID);
		UInt(&Block.BlockLen);

		Pos = context->impl->itell(context);

		switch (Block.BlockID)
		{
			case PSP_LAYER_START_BLOCK:
				if (!ReadLayerBlock(context, Block.BlockLen))
					return IL_FALSE;
				break;

			case PSP_ALPHA_BANK_BLOCK:
				if (!ReadAlphaBlock(context, Block.BlockLen))
					return IL_FALSE;
				break;

			case PSP_COLOR_BLOCK:
				if (!ReadPalette(context, Block.BlockLen))
					return IL_FALSE;
				break;

			// Gets done in the next iseek, so this is now commented out.
			//default:
				//iseek(Block.BlockLen, IL_SEEK_CUR);
		}

		// Skip to next block just in case we didn't read the entire block.
		context->impl->iseek(context, Pos + Block.BlockLen, IL_SEEK_SET);

		// @TODO: Do stuff here.

	} while (1);

	return IL_TRUE;
}


ILboolean ReadLayerBlock(ILcontext* context, ILuint BlockLen)
{
	BLOCKHEAD			Block;
	LAYERINFO_CHUNK		LayerInfo;
	LAYERBITMAP_CHUNK	Bitmap;
	ILuint				ChunkSize, Padding, i, j;
	ILushort			NumChars;

	BlockLen;

	// Layer sub-block header
	if (context->impl->iread(context, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(context);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_LAYER_BLOCK)
		return IL_FALSE;


	if (Header.MajorVersion == 3) {
		context->impl->iseek(context, 256, IL_SEEK_CUR);  // We don't care about the name of the layer.
		context->impl->iread(context, &LayerInfo, sizeof(LayerInfo), 1);
		if (context->impl->iread(context, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
	}
	else {  // Header.MajorVersion >= 4
		ChunkSize = GetLittleUInt(context);
		NumChars = GetLittleUShort(context);
		context->impl->iseek(context, NumChars, IL_SEEK_CUR);  // We don't care about the layer's name.

		ChunkSize -= (2 + 4 + NumChars);

		if (context->impl->iread(context, &LayerInfo, IL_MIN(sizeof(LayerInfo), ChunkSize), 1) != 1)
			return IL_FALSE;

		// Can have new entries in newer versions of the spec (5.0).
		Padding = (ChunkSize) - sizeof(LayerInfo);
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(context);
		if (context->impl->iread(context, &Bitmap, sizeof(Bitmap), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4) - sizeof(Bitmap);
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);
	}


	Channels = (ILubyte**)ialloc(context, sizeof(ILubyte*) * Bitmap.NumChannels);
	if (Channels == NULL) {
		return IL_FALSE;
	}

	NumChannels = Bitmap.NumChannels;

	for (i = 0; i < NumChannels; i++) {
		Channels[i] = GetChannel(context);
		if (Channels[i] == NULL) {
			for (j = 0; j < i; j++)
				ifree(Channels[j]);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


ILboolean ReadAlphaBlock(ILcontext* context, ILuint BlockLen)
{
	BLOCKHEAD		Block;
	ALPHAINFO_CHUNK	AlphaInfo;
	ALPHA_CHUNK		AlphaChunk;
	ILushort		NumAlpha, StringSize;
	ILuint			ChunkSize, Padding;

	if (Header.MajorVersion == 3) {
		NumAlpha = GetLittleUShort(context);
	}
	else {
		ChunkSize = GetLittleUInt(context);
		NumAlpha = GetLittleUShort(context);
		Padding = (ChunkSize - 4 - 2);
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);
	}

	// Alpha channel header
	if (context->impl->iread(context, &Block, 1, sizeof(Block)) != sizeof(Block))
		return IL_FALSE;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(context);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			return IL_FALSE;
	}
	if (Block.BlockID != PSP_ALPHA_CHANNEL_BLOCK)
		return IL_FALSE;


	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(context);
		StringSize = GetLittleUShort(context);
		context->impl->iseek(context, StringSize, IL_SEEK_CUR);
		if (context->impl->iread(context, &AlphaInfo, sizeof(AlphaInfo), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - 2 - StringSize - sizeof(AlphaInfo));
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);

		ChunkSize = GetLittleUInt(context);
		if (context->impl->iread(context, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
		Padding = (ChunkSize - 4 - sizeof(AlphaChunk));
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);
	}
	else {
		context->impl->iseek(context, 256, IL_SEEK_CUR);
		context->impl->iread(context, &AlphaInfo, sizeof(AlphaInfo), 1);
		if (context->impl->iread(context, &AlphaChunk, sizeof(AlphaChunk), 1) != 1)
			return IL_FALSE;
	}


	/*Alpha = (ILubyte*)ialloc(context, AlphaInfo.AlphaRect.x2 * AlphaInfo.AlphaRect.y2);
	if (Alpha == NULL) {
		return IL_FALSE;
	}*/


	Alpha = GetChannel(context);
	if (Alpha == NULL)
		return IL_FALSE;

	return IL_TRUE;
}


ILubyte *GetChannel(ILcontext* context)
{
	BLOCKHEAD		Block;
	CHANNEL_CHUNK	Channel;
	ILubyte			*CompData, *Data;
	ILuint			ChunkSize, Padding;

	if (context->impl->iread(context, &Block, 1, sizeof(Block)) != sizeof(Block))
		return NULL;
	if (Header.MajorVersion == 3)
		Block.BlockLen = GetLittleUInt(context);
	else
		UInt(&Block.BlockLen);

	if (Block.HeadID[0] != 0x7E || Block.HeadID[1] != 0x42 ||
		Block.HeadID[2] != 0x4B || Block.HeadID[3] != 0x00) {
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return NULL;
	}
	if (Block.BlockID != PSP_CHANNEL_BLOCK) {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return NULL;
	}


	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(context);
		if (context->impl->iread(context, &Channel, sizeof(Channel), 1) != 1)
			return NULL;

		Padding = (ChunkSize - 4) - sizeof(Channel);
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);
	}
	else {
		if (context->impl->iread(context, &Channel, sizeof(Channel), 1) != 1)
			return NULL;
	}


	CompData = (ILubyte*)ialloc(context, Channel.CompLen);
	Data = (ILubyte*)ialloc(context, AttChunk.Width * AttChunk.Height);
	if (CompData == NULL || Data == NULL) {
		ifree(Data);
		ifree(CompData);
		return NULL;
	}

	if (context->impl->iread(context, CompData, 1, Channel.CompLen) != Channel.CompLen) {
		ifree(CompData);
		ifree(Data);
		return NULL;
	}

	switch (AttChunk.Compression)
	{
		case PSP_COMP_NONE:
			ifree(Data);
			return CompData;
			break;

		case PSP_COMP_RLE:
			if (!UncompRLE(CompData, Data, Channel.CompLen)) {
				ifree(CompData);
				ifree(Data);
				return IL_FALSE;
			}
			break;

		default:
			ifree(CompData);
			ifree(Data);
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return NULL;
	}

	ifree(CompData);

	return Data;
}


ILboolean UncompRLE(ILubyte *CompData, ILubyte *Data, ILuint CompLen)
{
	ILubyte	Run, Colour;
	ILint	i, /*x, y,*/ Count/*, Total = 0*/;

	/*for (y = 0; y < AttChunk.Height; y++) {
		for (x = 0, Count = 0; x < AttChunk.Width; ) {
			Run = *CompData++;
			if (Run > 128) {
				Run -= 128;
				Colour = *CompData++;
				memset(Data, Colour, Run);
				Data += Run;
				Count += 2;
			}
			else {
				memcpy(Data, CompData, Run);
				CompData += Run;
				Data += Run;
				Count += Run;
			}
			x += Run;
		}

		Total += Count;

		if (Count % 4) {  // Has to be on a 4-byte boundary.
			CompData += (4 - (Count % 4)) % 4;
			Total += (4 - (Count % 4)) % 4;
		}

		if (Total >= CompLen)
			return IL_FALSE;
	}*/

	for (i = 0, Count = 0; i < (ILint)CompLen; ) {
		Run = *CompData++;
		i++;
		if (Run > 128) {
			Run -= 128;
			Colour = *CompData++;
			i++;
			memset(Data, Colour, Run);
		}
		else {
			memcpy(Data, CompData, Run);
			CompData += Run;
			i += Run;
		}
		Data += Run;
		Count += Run;
	}

	return IL_TRUE;
}


ILboolean ReadPalette(ILcontext* context, ILuint BlockLen)
{
	ILuint ChunkSize, PalCount, Padding;

	if (Header.MajorVersion >= 4) {
		ChunkSize = GetLittleUInt(context);
		PalCount = GetLittleUInt(context);
		Padding = (ChunkSize - 4 - 4);
		if (Padding > 0)
			context->impl->iseek(context, Padding, IL_SEEK_CUR);
	}
	else {
		PalCount = GetLittleUInt(context);
	}

	Pal.PalSize = PalCount * 4;
	Pal.PalType = IL_PAL_BGRA32;
	Pal.Palette = (ILubyte*)ialloc(context, Pal.PalSize);
	if (Pal.Palette == NULL)
		return IL_FALSE;

	if (context->impl->iread(context, Pal.Palette, Pal.PalSize, 1) != 1) {
		ifree(Pal.Palette);
		return IL_FALSE;
	}

	return IL_TRUE;
}


ILboolean AssembleImage(ILcontext* context)
{
	ILuint Size, i, j;

	Size = AttChunk.Width * AttChunk.Height;

	if (NumChannels == 1) {
		ilTexImage(context, AttChunk.Width, AttChunk.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
		for (i = 0; i < Size; i++) {
			context->impl->iCurImage->Data[i] = Channels[0][i];
		}

		if (Pal.Palette) {
			context->impl->iCurImage->Format = IL_COLOUR_INDEX;
			context->impl->iCurImage->Pal.PalSize = Pal.PalSize;
			context->impl->iCurImage->Pal.PalType = Pal.PalType;
			context->impl->iCurImage->Pal.Palette = Pal.Palette;
		}
	}
	else {
		if (Alpha) {
			ilTexImage(context, AttChunk.Width, AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 4) {
				context->impl->iCurImage->Data[j  ] = Channels[0][i];
				context->impl->iCurImage->Data[j+1] = Channels[1][i];
				context->impl->iCurImage->Data[j+2] = Channels[2][i];
				context->impl->iCurImage->Data[j+3] = Alpha[i];
			}
		}

		else if (NumChannels == 4) {

			ilTexImage(context, AttChunk.Width, AttChunk.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);

			for (i = 0, j = 0; i < Size; i++, j += 4) {

				context->impl->iCurImage->Data[j  ] = Channels[0][i];

				context->impl->iCurImage->Data[j+1] = Channels[1][i];

				context->impl->iCurImage->Data[j+2] = Channels[2][i];

				context->impl->iCurImage->Data[j+3] = Channels[3][i];

			}

		}
		else if (NumChannels == 3) {
			ilTexImage(context, AttChunk.Width, AttChunk.Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			for (i = 0, j = 0; i < Size; i++, j += 3) {
				context->impl->iCurImage->Data[j  ] = Channels[0][i];
				context->impl->iCurImage->Data[j+1] = Channels[1][i];
				context->impl->iCurImage->Data[j+2] = Channels[2][i];
			}
		}
		else
			return IL_FALSE;
	}

	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}


ILboolean Cleanup()
{
	ILuint	i;

	if (Channels) {
		for (i = 0; i < NumChannels; i++) {
			ifree(Channels[i]);
		}
		ifree(Channels);
	}

	if (Alpha) {
		ifree(Alpha);
	}

	Channels = NULL;
	Alpha = NULL;
	Pal.Palette = NULL;

	return IL_TRUE;
}



#endif//IL_NO_PSP
