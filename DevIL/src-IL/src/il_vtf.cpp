//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_vtf.cpp
//
// Description: Reads from and writes to a Valve Texture Format (.vtf) file.
//                These are used in Valve's Source games.  VTF specs available
//                from http://developer.valvesoftware.com/wiki/VTF.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_VTF

#include "il_dds.h"
#include "il_vtf.h"

#ifdef _MSC_VER
#pragma pack(push, vtf_struct, 1)
#elif defined(MACOSX) || defined(__GNUC__)
#pragma pack(1)
#endif

typedef struct VTFHEAD
{
	ILubyte		Signature[4];		// File signature ("VTF\0").
	ILuint		Version[2];			// version[0].version[1] (currently 7.2).
	ILuint		HeaderSize;			// Size of the header struct (16 byte aligned; currently 80 bytes).
	ILushort	Width;				// Width of the largest mipmap in pixels. Must be a power of 2.
	ILushort	Height;				// Height of the largest mipmap in pixels. Must be a power of 2.
	ILuint		Flags;				// VTF flags.
	ILushort	Frames;				// Number of frames, if animated (1 for no animation).
	ILushort	FirstFrame;			// First frame in animation (0 based).
	ILubyte		Padding0[4];		// reflectivity padding (16 byte alignment).
	ILfloat		Reflectivity[3];	// reflectivity vector.
	ILubyte		Padding1[4];		// reflectivity padding (8 byte packing).
	ILfloat		BumpmapScale;		// Bumpmap scale.
	ILuint		HighResImageFormat;	// High resolution image format.
	ILubyte		MipmapCount;		// Number of mipmaps.
	ILuint		LowResImageFormat;	// Low resolution image format (always DXT1).
	ILubyte		LowResImageWidth;	// Low resolution image width.
	ILubyte		LowResImageHeight;	// Low resolution image height.
	ILushort	Depth;				// Depth of the largest mipmap in pixels.
									// Must be a power of 2. Can be 0 or 1 for a 2D texture (v7.2 only).
} IL_PACKSTRUCT VTFHEAD;

#if defined(MACOSX) || defined(__GNUC__)
#pragma pack()
#elif _MSC_VER
#pragma pack(pop, vtf_struct)
#endif

enum
{
	IMAGE_FORMAT_NONE = -1,
	IMAGE_FORMAT_RGBA8888 = 0,
	IMAGE_FORMAT_ABGR8888,
	IMAGE_FORMAT_RGB888,
	IMAGE_FORMAT_BGR888,
	IMAGE_FORMAT_RGB565,
	IMAGE_FORMAT_I8,
	IMAGE_FORMAT_IA88,
	IMAGE_FORMAT_P8,
	IMAGE_FORMAT_A8,
	IMAGE_FORMAT_RGB888_BLUESCREEN,
	IMAGE_FORMAT_BGR888_BLUESCREEN,
	IMAGE_FORMAT_ARGB8888,
	IMAGE_FORMAT_BGRA8888,
	IMAGE_FORMAT_DXT1,
	IMAGE_FORMAT_DXT3,
	IMAGE_FORMAT_DXT5,
	IMAGE_FORMAT_BGRX8888,
	IMAGE_FORMAT_BGR565,
	IMAGE_FORMAT_BGRX5551,
	IMAGE_FORMAT_BGRA4444,
	IMAGE_FORMAT_DXT1_ONEBITALPHA,
	IMAGE_FORMAT_BGRA5551,
	IMAGE_FORMAT_UV88,
	IMAGE_FORMAT_UVWQ8888,
	IMAGE_FORMAT_RGBA16161616F,
	IMAGE_FORMAT_RGBA16161616,
	IMAGE_FORMAT_UVLX8888
};

enum
{
	TEXTUREFLAGS_POINTSAMPLE = 0x00000001,
	TEXTUREFLAGS_TRILINEAR = 0x00000002,
	TEXTUREFLAGS_CLAMPS = 0x00000004,
	TEXTUREFLAGS_CLAMPT = 0x00000008,
	TEXTUREFLAGS_ANISOTROPIC = 0x00000010,
	TEXTUREFLAGS_HINT_DXT5 = 0x00000020,
	TEXTUREFLAGS_NOCOMPRESS = 0x00000040,
	TEXTUREFLAGS_NORMAL = 0x00000080,
	TEXTUREFLAGS_NOMIP = 0x00000100,
	TEXTUREFLAGS_NOLOD = 0x00000200,
	TEXTUREFLAGS_MINMIP = 0x00000400,
	TEXTUREFLAGS_PROCEDURAL = 0x00000800,
	TEXTUREFLAGS_ONEBITALPHA = 0x00001000,
	TEXTUREFLAGS_EIGHTBITALPHA = 0x00002000,
	TEXTUREFLAGS_ENVMAP = 0x00004000,
	TEXTUREFLAGS_RENDERTARGET = 0x00008000,
	TEXTUREFLAGS_DEPTHRENDERTARGET = 0x00010000,
	TEXTUREFLAGS_NODEBUGOVERRIDE = 0x00020000,
	TEXTUREFLAGS_SINGLECOPY = 0x00040000,
	TEXTUREFLAGS_ONEOVERMIPLEVELINALPHA = 0x00080000,
	TEXTUREFLAGS_PREMULTCOLORBYONEOVERMIPLEVEL = 0x00100000,
	TEXTUREFLAGS_NORMALTODUDV = 0x00200000,
	TEXTUREFLAGS_ALPHATESTMIPGENERATION = 0x00400000,
	TEXTUREFLAGS_NODEPTHBUFFER = 0x00800000,
	TEXTUREFLAGS_NICEFILTERED = 0x01000000,
	TEXTUREFLAGS_CLAMPU = 0x02000000
};

// Internal functions
ILboolean	iGetVtfHead(ILcontext* context, VTFHEAD *Header);
ILboolean	iCheckVtf(VTFHEAD *Header);
ILboolean	VtfInitFacesMipmaps(ILcontext* context, ILimage *BaseImage, ILuint NumFaces, VTFHEAD *Header);
ILboolean	VtfInitMipmaps(ILcontext* context, ILimage *BaseImage, VTFHEAD *Header);
ILboolean	VtfReadData(void);
ILboolean	VtfDecompressDXT1(ILimage *Image);
ILboolean	VtfDecompressDXT5(ILimage *Image);

VtfHandler::VtfHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid VTF file.
ILboolean VtfHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	VtfFile;
	ILboolean	bVtf = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("vtf"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bVtf;
	}

	VtfFile = context->impl->iopenr(FileName);
	if (VtfFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bVtf;
	}

	bVtf = isValidF(VtfFile);
	context->impl->icloser(VtfFile);
	
	return bVtf;
}

//! Checks if the ILHANDLE contains a valid VTF file at the current position.
ILboolean VtfHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid VTF lump.
ILboolean VtfHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the VTF header from the current file.
ILboolean iGetVtfHead(ILcontext* context, VTFHEAD *Header)
{
	context->impl->iread(context, Header->Signature, 1, 4);
	Header->Version[0] = GetLittleUInt(context);
	Header->Version[1] = GetLittleUInt(context);
	Header->HeaderSize = GetLittleUInt(context);
	Header->Width = GetLittleUShort(context);
	Header->Height = GetLittleUShort(context);
	Header->Flags = GetLittleUInt(context);
	Header->Frames = GetLittleUShort(context);
	Header->FirstFrame = GetLittleUShort(context);
	context->impl->iseek(context, 4, IL_SEEK_CUR);  // Padding
	Header->Reflectivity[0] = GetLittleFloat(context);
	Header->Reflectivity[1] = GetLittleFloat(context);
	Header->Reflectivity[2] = GetLittleFloat(context);
	context->impl->iseek(context, 4, IL_SEEK_CUR);  // Padding
	Header->BumpmapScale = GetLittleFloat(context);
	Header->HighResImageFormat = GetLittleUInt(context);
	Header->MipmapCount = (ILubyte)context->impl->igetc(context);
	Header->LowResImageFormat = GetLittleInt(context);
	Header->LowResImageWidth = (ILubyte)context->impl->igetc(context);
	Header->LowResImageHeight = (ILubyte)context->impl->igetc(context);
//@TODO: This is a hack for the moment.
	if (Header->HeaderSize == 64) {
		Header->Depth = context->impl->igetc(context);
		if (Header->Depth == 0)
			Header->Depth = 1;
	}
	else {
		Header->Depth = GetLittleUShort(context);
		context->impl->iseek(context, Header->HeaderSize - sizeof(VTFHEAD), IL_SEEK_CUR);
	}

	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean VtfHandler::isValidInternal()
{
	VTFHEAD Header;

	if (!iGetVtfHead(context, &Header))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(VTFHEAD), IL_SEEK_CUR);
	
	return iCheckVtf(&Header);
}

//@TODO: Add more checks.
// Should we check for Frames, MipmapCount and Depth != 0?

// Internal function used to check if the HEADER is a valid VTF header.
ILboolean iCheckVtf(VTFHEAD *Header)
{
	// The file signature is "VTF\0".
	if ((Header->Signature[0] != 'V') || (Header->Signature[1] != 'T') || (Header->Signature[2] != 'F')
		|| (Header->Signature[3] != 0))
		return IL_FALSE;
	// Are there other versions available yet?
	if (Header->Version[0] != 7)
		return IL_FALSE;
	// We have 7.0 through 7.4 as of 12/27/2008.
	if (Header->Version[1] > 4)
		return IL_FALSE;
	// May change in future version of the specifications.
	//  80 is through version 7.2, and 96/104 are through 7.4.
	//  This must be 16-byte aligned, but something is outputting headers with 104.
	if ((Header->HeaderSize != 80) && (Header->HeaderSize != 96) && (Header->HeaderSize != 104)
		&& (Header->HeaderSize != 64))
		return IL_FALSE;

	// 0 is an invalid dimension
	if (Header->Width == 0 || Header->Height == 0)
		return IL_FALSE;
	// Width and Height must be powers of 2.
	if ((ilNextPower2(Header->Width) != Header->Width) || (ilNextPower2(Header->Height) != Header->Height))
		return IL_FALSE;
	// It looks like width and height of zero are valid - i.e. no low res image.
	if (Header->LowResImageWidth != 0 && Header->LowResImageHeight != 0) {
		if ((ilNextPower2(Header->LowResImageWidth) != Header->LowResImageWidth)
			|| (ilNextPower2(Header->LowResImageHeight) != Header->LowResImageHeight))
			return IL_FALSE;
	}
	// In addition, the LowResImage has to have dimensions no greater than 16.
	if ((Header->LowResImageWidth > 16) || (Header->LowResImageHeight > 16))
		//|| (Header->LowResImageWidth == 0) || (Header->LowResImageHeight == 0))
		// It looks like width and height of zero are valid.
		return IL_FALSE;
	// And the LowResImage has to have dimensions less than or equal to the main image.
	if ((Header->LowResImageWidth > Header->Width) || (Header->LowResImageHeight > Header->Height))
		return IL_FALSE;
	// The LowResImage must be in DXT1 format, or if it does not exist, it is denoted by all bits set.
	if (Header->LowResImageFormat != IMAGE_FORMAT_DXT1 && Header->LowResImageFormat != 0xFFFFFFFF)
		return IL_FALSE;
	
	return IL_TRUE;
}

//! Reads a VTF file
ILboolean VtfHandler::load(ILconst_string FileName)
{
	ILHANDLE	VtfFile;
	ILboolean	bVtf = IL_FALSE;
	
	VtfFile = context->impl->iopenr(FileName);
	if (VtfFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bVtf;
	}

	bVtf = loadF(VtfFile);
	context->impl->icloser(VtfFile);

	return bVtf;
}

//! Reads an already-opened VTF file
ILboolean VtfHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Reads from a memory "lump" that contains a VTF
ILboolean VtfHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the VTF.
ILboolean VtfHandler::loadInternal()
{
	ILboolean	bVtf = IL_TRUE;
	ILimage		*Image, *BaseImage;
	ILenum		Format, Type;
	ILint		Frame, Face, Mipmap;
	ILuint		SizeOfData, Channels, k;
	ILubyte		*CompData = NULL, SwapVal, *Data16Bit, *Temp, NumFaces;
	VTFHEAD		Head;
	ILuint		CurName;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	CurName = ilGetCurName(context);
	
	if (!iGetVtfHead(context, &Head))
		return IL_FALSE;
	if (!iCheckVtf(&Head)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (Head.Flags & TEXTUREFLAGS_ENVMAP) {
		// If we have a spherical map in addition to the 6 sides of the cubemap, FirstFrame's bits
		//  are all set.  The only place I found out about this was VTFLib.
		NumFaces = Head.FirstFrame == 0xFFFF ? 6 : 7;
	}
	else
		NumFaces = 1;  // This is not an environmental map.

	// Skip the low resolution image.  This is just a thumbnail.
	//  The block size is 8, and the compression ratio is 6:1.
	SizeOfData = IL_MAX(Head.LowResImageWidth * Head.LowResImageHeight / 2, 8);
	if (Head.LowResImageWidth == 0 && Head.LowResImageHeight == 0)
		SizeOfData = 0;  // No low resolution image present.
	context->impl->iseek(context, SizeOfData, IL_SEEK_CUR);

	//@TODO: Make this a helper function that set channels, bpc and format.
	switch (Head.HighResImageFormat)
	{
		case IMAGE_FORMAT_DXT1:  //@TODO: Should we make DXT1 channels = 3?
		case IMAGE_FORMAT_DXT1_ONEBITALPHA:
		case IMAGE_FORMAT_DXT3:
		case IMAGE_FORMAT_DXT5:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGR888:
		case IMAGE_FORMAT_BGR888_BLUESCREEN:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA8888:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRX8888:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGB888:
		case IMAGE_FORMAT_RGB888_BLUESCREEN:
			Channels = 3;
			Format = IL_RGB;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGBA8888:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGBA16161616:  // 16-bit shorts
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_SHORT;
			break;
		case IMAGE_FORMAT_RGBA16161616F:  // 16-bit floats
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_FLOAT;
			break;
		case IMAGE_FORMAT_I8:  // 8-bit luminance data
			Channels = 1;
			Format = IL_LUMINANCE;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_IA88:  // 8-bit luminance and alpha data
			Channels = 2;
			Format = IL_LUMINANCE_ALPHA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_A8:  // 8-bit alpha data
			Channels = 1;
			Format = IL_ALPHA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_ARGB8888:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_ABGR8888:
			Channels = 4;
			Format = IL_RGBA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_RGB565:
			Channels = 3;
			Format = IL_RGB;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGR565:
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA5551:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRX5551:  // Unused alpha channel
			Channels = 3;
			Format = IL_BGR;
			Type = IL_UNSIGNED_BYTE;
			break;
		case IMAGE_FORMAT_BGRA4444:
			Channels = 4;
			Format = IL_BGRA;
			Type = IL_UNSIGNED_BYTE;
			break;

		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!ilTexImage(context, Head.Width, Head.Height, Head.Depth, Channels, Format, Type, NULL))
		return IL_FALSE;
	// The origin should be in the upper left.
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	// Create any mipmaps.
	VtfInitFacesMipmaps(context, context->impl->iCurImage, NumFaces, &Head);

	// Create our animation chain
	BaseImage = Image = context->impl->iCurImage;  // Top-level image
	for (Frame = 1; Frame < Head.Frames; Frame++) {
		Image->Next = ilNewImageFull(context, Head.Width, Head.Height, Head.Depth, Channels, Format, Type, NULL);
		if (Image->Next == NULL)
			return IL_FALSE;
		Image = Image->Next;
		// The origin should be in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;

		// Create our faces and mipmaps for each frame.
		VtfInitFacesMipmaps(context, Image, NumFaces, &Head);
	}

	// We want to put the smallest mipmap at the end, but it is first in the file, so we count backwards.
	for (Mipmap = Head.MipmapCount - 1; Mipmap >= 0; Mipmap--) {
		// Frames are in the normal order.
		for (Frame = 0; Frame < Head.Frames; Frame++) {
			// @TODO: Cubemap faces are always in the same order?
			for (Face = 0; Face < NumFaces; Face++) {
				//@TODO: Would probably be quicker to do the linked list traversal manually here.
				ilBindImage(context, CurName);
				ilActiveImage(context, Frame);
				ilActiveFace(context, Face);
				ilActiveMipmap(context, Mipmap);
				Image = context->impl->iCurImage;

				switch (Head.HighResImageFormat)
				{
					// DXT1 compression
					case IMAGE_FORMAT_DXT1:
					case IMAGE_FORMAT_DXT1_ONEBITALPHA:
						// The block size is 8.
						SizeOfData = IL_MAX(Image->Width * Image->Height * Image->Depth / 2, 8);
						CompData = (ILubyte*)ialloc(context, SizeOfData);  // Gives a 6:1 compression ratio (or 8:1 for DXT1 with alpha)
						if (CompData == NULL)
							return IL_FALSE;
						context->impl->iread(context, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (ilGetInteger(context, IL_KEEP_DXTC_DATA) == IL_TRUE) {
							Image->DxtcSize = SizeOfData;
							Image->DxtcData = CompData;
							Image->DxtcFormat = IL_DXT5;
							CompData = NULL;
						}
						bVtf = DecompressDXT1(Image, CompData);
						break;

					// DXT3 compression
					case IMAGE_FORMAT_DXT3:
						// The block size is 16.
						SizeOfData = IL_MAX(Image->Width * Image->Height * Image->Depth, 16);
						CompData = (ILubyte*)ialloc(context, SizeOfData);  // Gives a 4:1 compression ratio
						if (CompData == NULL)
							return IL_FALSE;
						context->impl->iread(context, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (ilGetInteger(context, IL_KEEP_DXTC_DATA) == IL_TRUE) {
							Image->DxtcSize = SizeOfData;
							Image->DxtcData = CompData;
							Image->DxtcFormat = IL_DXT3;
							CompData = NULL;
						}
						bVtf = DecompressDXT3(Image, CompData);
						break;

					// DXT5 compression
					case IMAGE_FORMAT_DXT5:
						// The block size is 16.
						SizeOfData = IL_MAX(Image->Width * Image->Height * Image->Depth, 16);
						CompData = (ILubyte*)ialloc(context, SizeOfData);  // Gives a 4:1 compression ratio
						if (CompData == NULL)
							return IL_FALSE;
						context->impl->iread(context, CompData, 1, SizeOfData);
						// Keep a copy of the DXTC data if the user wants it.
						if (ilGetInteger(context, IL_KEEP_DXTC_DATA) == IL_TRUE) {
							Image->DxtcSize = SizeOfData;
							Image->DxtcData = CompData;
							Image->DxtcFormat = IL_DXT5;
							CompData = NULL;
						}
						bVtf = DecompressDXT5(Image, CompData);
						break;

					// Uncompressed BGR(A) data (24-bit and 32-bit)
					case IMAGE_FORMAT_BGR888:
					case IMAGE_FORMAT_BGRA8888:
					// Uncompressed RGB(A) data (24-bit and 32-bit)
					case IMAGE_FORMAT_RGB888:
					case IMAGE_FORMAT_RGBA8888:
					// Uncompressed 16-bit shorts
					case IMAGE_FORMAT_RGBA16161616:
					// Luminance data only
					case IMAGE_FORMAT_I8:
					// Luminance and alpha data
					case IMAGE_FORMAT_IA88:
					// Alpha data only
					case IMAGE_FORMAT_A8:
					// We will ignore the part about the bluescreen right now.
					//   I could not find any information about it.
					case IMAGE_FORMAT_RGB888_BLUESCREEN:
					case IMAGE_FORMAT_BGR888_BLUESCREEN:
						// Just copy the data over - no compression.
						if (context->impl->iread(context, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
							bVtf = IL_FALSE;
						else
							bVtf = IL_TRUE;
						break;

					// Uncompressed 24-bit data with an unused alpha channel (we discard it)
					case IMAGE_FORMAT_BGRX8888:
						SizeOfData = Image->Width * Image->Height * Image->Depth * 3;
						Temp = CompData = (ILubyte*)ialloc(context, SizeOfData / 3 * 4);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (context->impl->iread(context, CompData, 1, SizeOfData / 3 * 4) != SizeOfData / 3 * 4) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < SizeOfData; k += 3) {
							Image->Data[k]   = Temp[0];
							Image->Data[k+1] = Temp[1];
							Image->Data[k+2] = Temp[2];
							Temp += 4;
						}

						break;

					// Uncompressed 16-bit floats (must be converted to 32-bit)
					case IMAGE_FORMAT_RGBA16161616F:
						SizeOfData = Image->Width * Image->Height * Image->Depth * Image->Bpp * 2;
						CompData = (ILubyte*)ialloc(context, SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (context->impl->iread(context, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						bVtf = iConvFloat16ToFloat32((ILuint*)Image->Data, (ILushort*)CompData, SizeOfData / 2);
						break;

					// Uncompressed 32-bit ARGB and ABGR data.  DevIL does not handle this
					//   internally, so we have to swap values.
					case IMAGE_FORMAT_ARGB8888:
					case IMAGE_FORMAT_ABGR8888:
						if (context->impl->iread(context, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						else {
							bVtf = IL_TRUE;
						}
						// Swap the data
						for (k = 0; k < Image->SizeOfData; k += 4) {
							SwapVal = Image->Data[k];
							Image->Data[k]   = Image->Data[k+3];
							Image->Data[k+3] = SwapVal;
							SwapVal = Image->Data[k+1];
							Image->Data[k+1] = Image->Data[k+2];
							Image->Data[k+2] = SwapVal;
						}
						break;

					// Uncompressed 16-bit RGB and BGR data.  We have to expand this to 24-bit, since
					//   DevIL does not handle this internally.
					//   The data is in the file as: gggbbbbb rrrrrrggg
					case IMAGE_FORMAT_RGB565:
					case IMAGE_FORMAT_BGR565:
						SizeOfData = Image->Width * Image->Height * Image->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(context, SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (context->impl->iread(context, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < Image->SizeOfData; k += 3) {
							Image->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							Image->Data[k+1] = ((Data16Bit[1] & 0x07) << 5) | ((Data16Bit[0] & 0xE0) >> 3);
							Image->Data[k+2] =   Data16Bit[1] & 0xF8;
							Data16Bit += 2;
						}
						break;

					// Uncompressed 16-bit BGRA data (1-bit alpha).  We have to expand this to 32-bit,
					//   since DevIL does not handle this internally.
					//   Something seems strange with this one, but this is how VTFEdit outputs.
					//   The data is in the file as: gggbbbbb arrrrrgg
					case IMAGE_FORMAT_BGRA5551:
						SizeOfData = Image->Width * Image->Height * Image->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(context, SizeOfData);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (context->impl->iread(context, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < Image->SizeOfData; k += 4) {
							Image->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							Image->Data[k+1] = ((Data16Bit[0] & 0xE0) >> 2) | ((Data16Bit[1] & 0x03) << 6);
							Image->Data[k+2] =  (Data16Bit[1] & 0x7C) << 1;
							// 1-bit alpha is either off or on.
							Image->Data[k+3] = ((Data16Bit[0] & 0x80) == 0x80) ? 0xFF : 0x00;
							Data16Bit += 2;
						}
						break;

					// Same as above, but the alpha channel is unused.
					case IMAGE_FORMAT_BGRX5551:
						SizeOfData = Image->Width * Image->Height * Image->Depth * 2;
						Data16Bit = CompData = (ILubyte*)ialloc(context, SizeOfData);  // Not compressed data
						if (context->impl->iread(context, CompData, 1, SizeOfData) != SizeOfData) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < Image->SizeOfData; k += 3) {
							Image->Data[k]   =  (Data16Bit[0] & 0x1F) << 3;
							Image->Data[k+1] = ((Data16Bit[0] & 0xE0) >> 2) | ((Data16Bit[1] & 0x03) << 6);
							Image->Data[k+2] =  (Data16Bit[1] & 0x7C) << 1;
							Data16Bit += 2;
						}
						break;

					// Data is reduced to a 4-bits per channel format.
					case IMAGE_FORMAT_BGRA4444:
						SizeOfData = Image->Width * Image->Height * Image->Depth * 4;
						Temp = CompData = (ILubyte*)ialloc(context, SizeOfData / 2);  // Not compressed data
						if (CompData == NULL)
							return IL_FALSE;
						if (context->impl->iread(context, CompData, 1, SizeOfData / 2) != SizeOfData / 2) {
							bVtf = IL_FALSE;
							break;
						}
						for (k = 0; k < SizeOfData; k += 4) {
							// We double the data here.
							Image->Data[k]   = (Temp[0] & 0x0F) << 4 | (Temp[0] & 0x0F);
							Image->Data[k+1] = (Temp[0] & 0xF0) >> 4 | (Temp[0] & 0xF0);
							Image->Data[k+2] = (Temp[1] & 0x0F) << 4 | (Temp[1] & 0x0F);
							Image->Data[k+3] = (Temp[1] & 0xF0) >> 4 | (Temp[1] & 0xF0);
							Temp += 2;
						}
						break;
				}

				ifree(CompData);
				CompData = NULL;
				if (bVtf == IL_FALSE)  //@TODO: Do we need to do any cleanup here?
					return IL_FALSE;
			}
		}
	}

	ilBindImage(context, CurName);  // Set to parent image first.
	return ilFixImage(context);
}

ILuint GetFaceFlag(ILuint FaceNum)
{
	switch (FaceNum)
	{
		case 0:
			return IL_CUBEMAP_POSITIVEX;
		case 1:
			return IL_CUBEMAP_NEGATIVEX;
		case 2:
			return IL_CUBEMAP_POSITIVEY;
		case 3:
			return IL_CUBEMAP_NEGATIVEY;
		case 4:
			return IL_CUBEMAP_POSITIVEZ;
		case 5:
			return IL_CUBEMAP_NEGATIVEZ;
		case 6:
			return IL_SPHEREMAP;
	}

	return IL_SPHEREMAP;  // Should never reach here!
}

ILboolean VtfInitFacesMipmaps(ILcontext* context, ILimage *BaseImage, ILuint NumFaces, VTFHEAD *Header)
{
	ILimage	*Image;
	ILuint	Face;

	// Initialize mipmaps under the base image.
	VtfInitMipmaps(context, BaseImage, Header);
	Image = BaseImage;

	// We have an environment map.
	if (NumFaces != 1) {
		Image->CubeFlags = IL_CUBEMAP_POSITIVEX;
	}

	for (Face = 1; Face < NumFaces; Face++) {
		Image->Faces = ilNewImageFull(context, Image->Width, Image->Height, Image->Depth, Image->Bpp, Image->Format, Image->Type, NULL);
		if (Image->Faces == NULL)
			return IL_FALSE;
		Image = Image->Faces;

		// The origin should be in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
		// Set the flags that tell which face this is.
		Image->CubeFlags = GetFaceFlag(Face);

		// Now we can initialize the mipmaps under each face.
		VtfInitMipmaps(context, Image, Header);
	}

	return IL_TRUE;
}

ILboolean VtfInitMipmaps(ILcontext* context, ILimage *BaseImage, VTFHEAD *Header)
{
	ILimage	*Image;
	ILuint	Width, Height, Depth, Mipmap;

	Image = BaseImage;
	Width = BaseImage->Width;  Height = BaseImage->Height;  Depth = BaseImage->Depth;

	for (Mipmap = 1; Mipmap < Header->MipmapCount; Mipmap++) {
		// 1 is the smallest dimension possible.
		Width = (Width >> 1) == 0 ? 1 : (Width >> 1);
		Height = (Height >> 1) == 0 ? 1 : (Height >> 1);
		Depth = (Depth >> 1) == 0 ? 1 : (Depth >> 1);

		Image->Mipmaps = ilNewImageFull(context, Width, Height, Depth, BaseImage->Bpp, BaseImage->Format, BaseImage->Type, NULL);
		if (Image->Mipmaps == NULL)
			return IL_FALSE;
		Image = Image->Mipmaps;

		// ilNewImage does not set these.
		Image->Format = BaseImage->Format;
		Image->Type = BaseImage->Type;
		// The origin should be in the upper left.
		Image->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	return IL_TRUE;
}

ILboolean CheckDimensions(ILcontext* context)
{
	if ((ilNextPower2(context->impl->iCurImage->Width) != context->impl->iCurImage->Width) || (ilNextPower2(context->impl->iCurImage->Height) != context->impl->iCurImage->Height)) {
		ilSetError(context, IL_BAD_DIMENSIONS);
		return IL_FALSE;
	}
	return IL_TRUE;
}

//! Writes a Vtf file
ILboolean VtfHandler::save(const ILstring FileName)
{
	ILHANDLE	VtfFile;
	ILuint		VtfSize;

	if (!CheckDimensions(context))
		return IL_FALSE;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	VtfFile = context->impl->iopenw(FileName);
	if (VtfFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	VtfSize = saveF(VtfFile);
	context->impl->iclosew(VtfFile);

	if (VtfSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a .vtf to an already-opened file
ILuint VtfHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	if (!CheckDimensions(context))
		return 0;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a .vtf to a memory "lump"
ILuint VtfHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos;
	if (!CheckDimensions(context))
		return 0;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Internal function used to save the Vtf.
ILboolean VtfHandler::saveInternal()
{
	ILimage	*TempImage = context->impl->iCurImage;
	ILubyte	*TempData, *CompData;
	ILuint	Format, i, CompSize;
	ILenum	Compression;

	// Find out if the user has specified to use DXT compression.
	Compression = ilGetInteger(context, IL_VTF_COMP);

	//@TODO: Other formats
	if (Compression == IL_DXT_NO_COMP) {
		switch (TempImage->Format)
		{
			case IL_RGB:
				Format = IMAGE_FORMAT_RGB888;
				break;
			case IL_RGBA:
				Format = IMAGE_FORMAT_RGBA8888;
				break;
			case IL_BGR:
				Format = IMAGE_FORMAT_BGR888;
				break;
			case IL_BGRA:
				Format = IMAGE_FORMAT_BGRA8888;
				break;
			case IL_LUMINANCE:
				Format = IMAGE_FORMAT_I8;
				break;
			case IL_ALPHA:
				Format = IMAGE_FORMAT_A8;
				break;
			case IL_LUMINANCE_ALPHA:
				Format = IMAGE_FORMAT_IA88;
				break;
			//case IL_COLOUR_INDEX:
			default:
				Format = IMAGE_FORMAT_BGRA8888;
				TempImage = iConvertImage(context, context->impl->iCurImage, IL_BGRA, IL_UNSIGNED_BYTE);
				if (TempImage == NULL)
					return IL_FALSE;
		}

		//@TODO: When we have the half format available internally, also use IMAGE_FORMAT_RGBA16161616F.
		if (TempImage->Format == IL_RGBA && TempImage->Type == IL_UNSIGNED_SHORT) {
			Format = IMAGE_FORMAT_RGBA16161616;
		}
		else if (TempImage->Type != IL_UNSIGNED_BYTE) {  //@TODO: Any possibility for shorts, etc. to be used?
			TempImage = iConvertImage(context, context->impl->iCurImage, Format, IL_UNSIGNED_BYTE);
			if (TempImage == NULL)
				return IL_FALSE;
		}
	}
	else {  // We are using DXT compression.
		switch (Compression)
		{
			case IL_DXT1:
				Format = IMAGE_FORMAT_DXT1_ONEBITALPHA;//IMAGE_FORMAT_DXT1;
				break;
			case IL_DXT3:
				Format = IMAGE_FORMAT_DXT3;
				break;
			case IL_DXT5:
				Format = IMAGE_FORMAT_DXT5;
				break;
			default:  // Should never reach this point.
				ilSetError(context, IL_INTERNAL_ERROR);
				Format = IMAGE_FORMAT_DXT5;
		}
	}

	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	} else {
		TempData = TempImage->Data;
	}

	// Write the file signature.
	context->impl->iwrite(context, "VTF", 1, 4);
	// Write the file version - currently using 7.2 specs.
	SaveLittleUInt(context, 7);
	SaveLittleUInt(context, 2);
	// Write the header size.
	SaveLittleUInt(context, 80);
	// Now we write the width and height of the image.
	SaveLittleUShort(context, TempImage->Width);
	SaveLittleUShort(context, TempImage->Height);
	//@TODO: This is supposed to be the flags used.  What should we use here?  Let users specify?
	SaveLittleUInt(context, 0);
	// Number of frames in the animation. - @TODO: Change to use animations.
	SaveLittleUShort(context, 1);
	// First frame in the animation
	SaveLittleUShort(context, 0);
	// Padding
	SaveLittleUInt(context, 0);
	// Reflectivity (3 floats) - @TODO: Use what values?  User specified?
	SaveLittleFloat(context, 0.0f);
	SaveLittleFloat(context, 0.0f);
	SaveLittleFloat(context, 0.0f);
	// Padding
	SaveLittleUInt(context, 0);
	// Bumpmap scale
	SaveLittleFloat(context, 0.0f);
	// Image format
	SaveLittleUInt(context, Format);
	// Mipmap count - @TODO: Use mipmaps
	context->impl->iputc(context, 1);
	// Low resolution image format - @TODO: Create low resolution image.
	SaveLittleUInt(context, 0xFFFFFFFF);
	// Low resolution image width and height
	context->impl->iputc(context, 0);
	context->impl->iputc(context, 0);
	// Depth of the image - @TODO: Support for volumetric images.
	SaveLittleUShort(context, 1);

	// Write final padding for the header (out to 80 bytes).
	for (i = 0; i < 15; i++) {
		context->impl->iputc(context, 0);
	}

	if (Compression == IL_DXT_NO_COMP) {
		// We just write the image data directly.
		if (context->impl->iwrite(context, TempImage->Data, TempImage->SizeOfData, 1) != 1)
			return IL_FALSE;
	}
	else {  // Do DXT compression here and write.
		// We have to find out how much we are writing first.
		CompSize = ilGetDXTCData(context, NULL, 0, Compression);
		if (CompSize == 0) {
			ilSetError(context, IL_INTERNAL_ERROR);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
		CompData = (ILubyte*)ialloc(context, CompSize);
		if (CompData == NULL) {
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}

		// DXT compress the data.
		CompSize = ilGetDXTCData(context, CompData, CompSize, Compression);
		if (CompSize == 0) {
			ilSetError(context, IL_INTERNAL_ERROR);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
		// Finally write the data.
		if (context->impl->iwrite(context, CompData, CompSize, 1) != 1) {
			ifree(CompData);
			if (TempData != TempImage->Data)
				ifree(TempData);
			return IL_FALSE;
		}
	}

	if (TempData != TempImage->Data)
		ifree(TempData);
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}

#endif//IL_NO_VTF