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

#ifndef IL_NO_PSP

#include "il_psp.h"

// Block identifiers
enum PSPBlockID {
	PSP_IMAGE_BLOCK = 0,			// (0)  General Image Attributes Block (main)
	PSP_CREATOR_BLOCK,				// (1)  Creator Data Block (main)
	PSP_COLOR_BLOCK,				// (2)  Color Palette Block (main and sub)
	PSP_LAYER_START_BLOCK,			// (3)  Layer Bank Block (main)
	PSP_LAYER_BLOCK,				// (4)  Layer Block (sub)
	PSP_CHANNEL_BLOCK,				// (5)  Channel Block (sub)
	PSP_SELECTION_BLOCK,			// (6)  Selection Block (main)
	PSP_ALPHA_BANK_BLOCK,			// (7)  Alpha Bank Block (main)
	PSP_ALPHA_CHANNEL_BLOCK,		// (8)  Alpha Channel Block (sub)
	PSP_COMPOSITE_IMAGE_BLOCK,		// (9)  Composite Image Block (sub)
	PSP_EXTENDED_DATA_BLOCK,		// (10) Extended Data Block (main)
	PSP_TUBE_BLOCK,					// (11) Picture Tube Data Block (main)
	PSP_ADJUSTMENT_EXTENSION_BLOCK,	// (12) Adjustment Layer Block (sub)
	PSP_VECTOR_EXTENSION_BLOCK,		// (13) Vector Layer Block (sub)
	PSP_SHAPE_BLOCK,				// (14) Vector Shape Block (sub)
	PSP_PAINTSTYLE_BLOCK,			// (15) Paint Style Block (sub)
	PSP_COMPOSITE_IMAGE_BANK_BLOCK, // (16) Composite Image Bank (main)
	PSP_COMPOSITE_ATTRIBUTES_BLOCK, // (17) Composite Image Attr. (sub)
	PSP_JPEG_BLOCK,					// (18) JPEG Image Block (sub)
	PSP_LINESTYLE_BLOCK,			// (19) Line Style Block (sub)
	PSP_TABLE_BANK_BLOCK,			// (20) Table Bank Block (main)
	PSP_TABLE_BLOCK,				// (21) Table Block (sub)
	PSP_PAPER_BLOCK,				// (22) Vector Table Paper Block (sub)
	PSP_PATTERN_BLOCK,				// (23) Vector Table Pattern Block (sub)
};

// Bitmap type
enum PSPDIBType {
	PSP_DIB_IMAGE = 0,	// Layer color bitmap
	PSP_DIB_TRANS_MASK,	// Layer transparency mask bitmap
	PSP_DIB_USER_MASK,	// Layer user mask bitmap
	PSP_DIB_SELECTION,	// Selection mask bitmap
	PSP_DIB_ALPHA_MASK,	// Alpha channel mask bitmap
	PSP_DIB_THUMBNAIL	// Thumbnail bitmap
};

// Channel types
enum PSPChannelType {
	PSP_CHANNEL_COMPOSITE = 0,	// Channel of single channel bitmap
	PSP_CHANNEL_RED,			// Red channel of 24 bit bitmap
	PSP_CHANNEL_GREEN,			// Green channel of 24 bit bitmap
	PSP_CHANNEL_BLUE			// Blue channel of 24 bit bitmap
};

// Possible metrics used to measure resolution
enum PSP_METRIC { 
	PSP_METRIC_UNDEFINED = 0,	// Metric unknown
	PSP_METRIC_INCH,			// Resolution is in inches
	PSP_METRIC_CM				// Resolution is in centimeters
};

// Possible types of compression.
enum PSPCompression {
	PSP_COMP_NONE = 0,	// No compression
	PSP_COMP_RLE,		// RLE compression
	PSP_COMP_LZ77,		// LZ77 compression
	PSP_COMP_JPEG		// JPEG compression (only used by thumbnail and composite image)
};

// Picture tube placement mode.
enum TubePlacementMode {
	tpmRandom,		// Place tube images in random intervals
	tpmConstant		// Place tube images in constant intervals
};

// Picture tube selection mode.
enum TubeSelectionMode {
	tsmRandom,		// Randomly select the next image in tube to display
	tsmIncremental,	// Select each tube image in turn
	tsmAngular,		// Select image based on cursor direction
	tsmPressure,	// Select image based on pressure (from pressure-sensitive pad)
	tsmVelocity		// Select image based on cursor speed
};

// Extended data field types.
enum PSPExtendedDataID {
	PSP_XDATA_TRNS_INDEX = 0	// Transparency index field
};

// Creator field types.
enum PSPCreatorFieldID {
	PSP_CRTR_FLD_TITLE = 0,		// Image document title field
	PSP_CRTR_FLD_CRT_DATE,		// Creation date field
	PSP_CRTR_FLD_MOD_DATE,		// Modification date field
	PSP_CRTR_FLD_ARTIST,		// Artist name field
	PSP_CRTR_FLD_CPYRGHT,		// Copyright holder name field
	PSP_CRTR_FLD_DESC,			// Image document description field
	PSP_CRTR_FLD_APP_ID,		// Creating app id field
	PSP_CRTR_FLD_APP_VER,		// Creating app version field
};

// Creator application identifiers.
enum PSPCreatorAppID {
	PSP_CREATOR_APP_UNKNOWN = 0,	// Creator application unknown
	PSP_CREATOR_APP_PAINT_SHOP_PRO	// Creator is Paint Shop Pro
};

// Layer types.
enum PSPLayerType {
	PSP_LAYER_NORMAL = 0,			// Normal layer
	PSP_LAYER_FLOATING_SELECTION	// Floating selection layer
};

// Truth values.
/*enum PSP_BOOLEAN {
	FALSE = 0,
	TRUE
};*/

#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif

typedef struct PSPRECT
{
	ILuint x1,y1,x2,y2;
} IL_PACKSTRUCT PSPRECT;

typedef struct PSPHEAD
{
	char		FileSig[32];
	ILushort	MajorVersion;
	ILushort	MinorVersion;
} IL_PACKSTRUCT PSPHEAD;

typedef struct BLOCKHEAD
{
	ILubyte		HeadID[4];
	ILushort	BlockID;
	ILuint		BlockLen;
} IL_PACKSTRUCT BLOCKHEAD;

typedef struct GENATT_CHUNK
{
	ILint		Width;
	ILint		Height;
	ILdouble	Resolution;
	ILubyte		ResMetric;
	ILushort	Compression;
	ILushort	BitDepth;
	ILushort	PlaneCount;
	ILuint		ColourCount;
	ILubyte		GreyscaleFlag;
	ILuint		SizeOfImage;
	ILint		ActiveLayer;
	ILushort	LayerCount;
	ILuint		GraphicContents;
} IL_PACKSTRUCT GENATT_CHUNK;

typedef struct LAYERINFO_CHUNK
{
	ILubyte		LayerType;
	PSPRECT		ImageRect;
	PSPRECT		SavedImageRect;
	ILubyte		Opacity;
	ILubyte		BlendingMode;
	ILubyte		LayerFlags;
	ILubyte		TransProtFlag;
	ILubyte		LinkID;
	PSPRECT		MaskRect;
	PSPRECT		SavedMaskRect;
	ILubyte		MaskLinked;
	ILubyte		MaskDisabled;
	ILubyte		InvertMaskBlend;
	ILushort	BlendRange;
	ILubyte		SourceBlend1[4];
	ILubyte		DestBlend1[4];
	ILubyte		SourceBlend2[4];
	ILubyte		DestBlend2[4];
	ILubyte		SourceBlend3[4];
	ILubyte		DestBlend3[4];
	ILubyte		SourceBlend4[4];
	ILubyte		DestBlend4[4];
	ILubyte		SourceBlend5[4];
	ILubyte		DestBlend5[4];
} IL_PACKSTRUCT LAYERINFO_CHUNK;

typedef struct LAYERBITMAP_CHUNK
{
	ILushort	NumBitmaps;
	ILushort	NumChannels;
} IL_PACKSTRUCT LAYERBITMAP_CHUNK;

typedef struct CHANNEL_CHUNK
{
	ILuint		CompLen;
	ILuint		Length;
	ILushort	BitmapType;
	ILushort	ChanType;
} IL_PACKSTRUCT CHANNEL_CHUNK;

typedef struct ALPHAINFO_CHUNK
{
	PSPRECT		AlphaRect;
	PSPRECT		AlphaSavedRect;
} IL_PACKSTRUCT ALPHAINFO_CHUNK;

typedef struct ALPHA_CHUNK
{
	ILushort	BitmapCount;
	ILushort	ChannelCount;
} IL_PACKSTRUCT ALPHA_CHUNK;

#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

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

// Function definitions
ILboolean	iCheckPsp(void);
ILboolean	ReadGenAttributes(ILcontext* context);
ILboolean	ParseChunks(ILcontext* context);
ILboolean	ReadLayerBlock(ILcontext* context, ILuint BlockLen);
ILboolean	ReadAlphaBlock(ILcontext* context, ILuint BlockLen);
ILubyte		*GetChannel(ILcontext* context);
ILboolean	UncompRLE(ILubyte *CompData, ILubyte *Data, ILuint CompLen);
ILboolean	ReadPalette(ILcontext* context, ILuint BlockLen);
ILboolean	AssembleImage(ILcontext* context);
ILboolean	Cleanup(void);

PspHandler::PspHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid Psp file.
ILboolean PspHandler::isValid(ILconst_string FileName)
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

	bPsp = isValidF(PspFile);
	context->impl->icloser(PspFile);

	return bPsp;
}

//! Checks if the ILHANDLE contains a valid Psp file at the current position.
ILboolean PspHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid Psp lump.
ILboolean PspHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
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
ILboolean PspHandler::isValidInternal()
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
ILboolean PspHandler::load(ILconst_string FileName)
{
	ILHANDLE	PSPFile;
	ILboolean	bPsp = IL_FALSE;

	PSPFile = context->impl->iopenr(FileName);
	if (PSPFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPsp;
	}

	bPsp = loadF(PSPFile);
	context->impl->icloser(PSPFile);

	return bPsp;
}

//! Reads an already-opened PSP file
ILboolean PspHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a PSP
ILboolean PspHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the PSP.
ILboolean PspHandler::loadInternal()
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