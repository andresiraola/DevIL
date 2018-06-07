//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2016 by Denton Woods
// Last modified: 05/15/2016
//
// Filename: src-IL/src/il_ktx.cpp
//
// Description: Reads a Khronos Texture .ktx file
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_KTX

#include "il_ktx.h"

#include "il_bits.h"
#include "rg_etc1.h"

ILboolean	iKtxReadMipmaps(ILcontext* context, ILboolean Compressed, ILuint NumMips, ILenum Origin);
ILboolean	iKtxKeyValueData(ILcontext* context, ILuint bytesOfKeyValueData, ILenum &Origin);

#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif
typedef struct KTX_HEAD
{
	ILubyte	identifier[12];
	ILuint	endianness;
	ILuint	glType;
	ILuint	glTypeSize;
	ILuint	glFormat;
	ILuint	glInternalFormat;
	ILuint	glBaseInternalFormat;
	ILuint	pixelWidth;
	ILuint	pixelHeight;
	ILuint	pixelDepth;
	ILuint	numberOfArrayElements;
	ILuint	numberOfFaces;
	ILuint	numberOfMipmapLevels;
	ILuint	bytesOfKeyValueData;
} IL_PACKSTRUCT KTX_HEAD;
#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

// From GL/GL.h
#define I_GL_BYTE                           0x1400
#define I_GL_UNSIGNED_BYTE                  0x1401
#define I_GL_SHORT                          0x1402
#define I_GL_UNSIGNED_SHORT                 0x1403
#define I_GL_INT                            0x1404
#define I_GL_UNSIGNED_INT                   0x1405
#define I_GL_FLOAT                          0x1406
//#define I_GL_2_BYTES                        0x1407
//#define I_GL_3_BYTES                        0x1408
//#define I_GL_4_BYTES                        0x1409
#define I_GL_DOUBLE                         0x140A
#define I_GL_HALF                           0x140B

#define I_GL_ALPHA                          0x1906
#define I_GL_RGB                            0x1907
#define I_GL_RGBA                           0x1908
#define I_GL_LUMINANCE                      0x1909
#define I_GL_LUMINANCE_ALPHA                0x190A
// From gl2ext.h
#define I_GL_ETC1_RGB8_OES                  0x8D64
// From https://www.opengl.org/registry/specs/ARB/ES3_compatibility.txt
#define I_GL_COMPRESSED_RGB8_ETC2                             0x9274
#define I_GL_COMPRESSED_SRGB8_ETC2                            0x9275
#define I_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2         0x9276
#define I_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2        0x9277
#define I_GL_COMPRESSED_RGBA8_ETC2_EAC                        0x9278
#define I_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC                 0x9279
#define I_GL_COMPRESSED_R11_EAC                               0x9270
#define I_GL_COMPRESSED_SIGNED_R11_EAC                        0x9271
#define I_GL_COMPRESSED_RG11_EAC                              0x9272
#define I_GL_COMPRESSED_SIGNED_RG11_EAC                       0x9273

KtxHandler::KtxHandler(ILcontext* context) :
	context(context)
{

}

ILboolean KtxHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	KtxFile;
	ILboolean	bKtx = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("ktx"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bKtx;
	}

	KtxFile = context->impl->iopenr(FileName);
	if (KtxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bKtx;
	}

	bKtx = isValidF(KtxFile);
	context->impl->icloser(KtxFile);

	return bKtx;
}

ILboolean KtxHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

ILboolean KtxHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

ILboolean KtxHandler::isValidInternal()
{
	ILubyte 	Signature[8];
	ILint		Read;

	/*Read = context->impl->iread(context, Signature, 1, 8);
	context->impl->iseek(context, -Read, IL_SEEK_CUR);*/

	return IL_FALSE;
}

//! Reads a .ktx file
ILboolean KtxHandler::load(ILconst_string FileName)
{
	ILHANDLE	KtxFile;
	ILboolean	bKtx = IL_FALSE;

	KtxFile = context->impl->iopenr(FileName);
	if (KtxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bKtx;
	}

	bKtx = loadF(KtxFile);
	context->impl->icloser(KtxFile);

	return bKtx;
}

//! Reads an already-opened .ktx file
ILboolean KtxHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a .ktx
ILboolean KtxHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Note:  .Ktx support has not been tested yet!
//  https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
ILboolean KtxHandler::loadInternal()
{
	KTX_HEAD	Header;
	ILuint		imageSize;
	ILenum		Format, Type, Origin;
	ILubyte		Bpp, Bpc;
	ILboolean	Compressed;
	char		FileIdentifier[12] = {
		//0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		'�', 'K', 'T', 'X', ' ', '1', '1', '�', '\r', '\n', '\x1A', '\n'
	};

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	/*Header.Width = GetLittleShort(context);
	Header.Height = GetLittleShort(context);
	Header.Dummy = GetLittleInt(context);*/

	if (context->impl->iread(context, Header.identifier, 1, 12) != 12)
		return IL_FALSE;
	Header.endianness = GetLittleUInt(context);
	Header.glType = GetLittleUInt(context);
	Header.glTypeSize = GetLittleUInt(context);
	Header.glFormat = GetLittleUInt(context);
	Header.glInternalFormat = GetLittleUInt(context);
	Header.glBaseInternalFormat = GetLittleUInt(context);
	Header.pixelWidth = GetLittleUInt(context);
	Header.pixelHeight = GetLittleUInt(context);
	Header.pixelDepth = GetLittleUInt(context);
	Header.numberOfArrayElements = GetLittleUInt(context);
	Header.numberOfFaces = GetLittleUInt(context);
	Header.numberOfMipmapLevels = GetLittleUInt(context);
	Header.bytesOfKeyValueData = GetLittleUInt(context);

	if (memcmp(Header.identifier, FileIdentifier, 12) || Header.endianness != 0x04030201) {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	if (Header.glTypeSize != 1)  //@TODO: Cases when this is different?
	{
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}
	//@TODO: Really needed?
	/*if (Header.glInternalFormat != Header.glFormat || Header.glBaseInternalFormat != Header.glFormat)
	{
	ilSetError(context, IL_ILLEGAL_FILE_VALUE);
	return IL_FALSE;
	}*/

	//@TODO: Mipmaps, etc.
	if (Header.numberOfArrayElements != 0 || Header.numberOfFaces != 1 /*|| Header.numberOfMipmapLevels != 1*/)
	{
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}
	if (!iKtxKeyValueData(context, Header.bytesOfKeyValueData, Origin))
		return IL_FALSE;

	//@TODO: Additional formats
	if (Header.glFormat != 0)  // Uncompressed formats
	{
		switch (Header.glFormat)
		{
		case I_GL_LUMINANCE:
			Bpp = 1;
			Format = IL_LUMINANCE;
			Compressed = IL_FALSE;
			break;
		case I_GL_LUMINANCE_ALPHA:
			Bpp = 2;
			Format = IL_LUMINANCE_ALPHA;
			Compressed = IL_FALSE;
			break;
		case I_GL_RGB:
			Bpp = 3;
			Format = IL_RGB;
			Compressed = IL_FALSE;
			break;
		case I_GL_RGBA:
			Bpp = 4;
			Format = IL_RGBA;
			Compressed = IL_FALSE;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}
	else  // Compressed formats - Note that type must be set here!
	{
		switch (Header.glInternalFormat)
		{
		case I_GL_ETC1_RGB8_OES:
			Bpp = 4;
			Format = IL_RGBA;
			Compressed = IL_TRUE;
			Type = IL_UNSIGNED_BYTE;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	if (!Compressed)
	{
		//@TODO: Additional types
		switch (Header.glType)
		{
		case I_GL_UNSIGNED_BYTE:
			Bpc = 1;
			Type = IL_UNSIGNED_BYTE;
			break;
		case I_GL_HALF:
			Bpc = 2;
			Type = IL_HALF;
			break;
			//case I_GL_SHORT:
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		if (!ilTexImage(context, Header.pixelWidth, Header.pixelHeight, 1, Bpp, Format, Type, NULL))
			return IL_FALSE;
		if (!iKtxReadMipmaps(context, Compressed, Header.numberOfMipmapLevels, Origin))
			return IL_FALSE;
	}
	else  // Compressed formats require different handling
	{
		//@TODO: Add support for compressed mipmaps
		if (Header.numberOfMipmapLevels != 1)
		{
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}

		imageSize = GetLittleUInt(context);

		if (!ilTexImage(context, Header.pixelWidth, Header.pixelHeight, 1, Bpp, Format, Type, NULL))
			return IL_FALSE;

		switch (Header.glInternalFormat)
		{
		case I_GL_ETC1_RGB8_OES:
			ILubyte * FullBuffer = (ILubyte*)ialloc(context, Header.pixelWidth * Header.pixelHeight / 2);
			ILuint UnpackBuff[16 * 4];
			ILuint *Data = (ILuint*)context->impl->iCurImage->Data;  // Easier copies

			if (FullBuffer == NULL)
				return IL_FALSE;
			if (imageSize != Header.pixelWidth * Header.pixelHeight / 2) {
				ilSetError(context, IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}
			if (context->impl->iread(context, FullBuffer, 1, imageSize) != imageSize)
				return IL_FALSE;

			ILuint BlockPos = 0;
			for (ILuint y = 0; y < context->impl->iCurImage->Height; y += 4)
			{
				for (ILuint x = 0; x < context->impl->iCurImage->Width; x += 4)
				{
					ILuint ImagePos = y * context->impl->iCurImage->Width + x;
					rg_etc1::unpack_etc1_block(&FullBuffer[BlockPos], (ILuint*)UnpackBuff, false);
					ILuint c = 0;
					for (ILuint y1 = 0; y1 < 4; y1++)
					{
						for (ILuint x1 = 0; x1 < 4; x1++)
						{
							Data[ImagePos + y1 * context->impl->iCurImage->Width + x1] = UnpackBuff[c++];
						}
					}
					BlockPos += 8;
				}
			}

			ifree(FullBuffer);
			break;
		}

		context->impl->iCurImage->Origin = Origin;  //@TODO: Correct for all?
	}

	return ilFixImage(context);
}

// There are multiple items that can be in the key values, but all we care about
//  right now is the orientation/origin data.
ILboolean iKtxKeyValueData(ILcontext* context, ILuint bytesOfKeyValueData, ILenum &Origin)
{
	ILubyte	*KeyValueData;
	ILuint	Pos = 0, Length;

	Origin = IL_ORIGIN_UPPER_LEFT;  // KTX specs declare that this is the default

	if (bytesOfKeyValueData == 0)
		return IL_TRUE;

	KeyValueData = (ILubyte*)ialloc(context, bytesOfKeyValueData);
	if (KeyValueData == NULL)
		return IL_FALSE;

	if (context->impl->iread(context, KeyValueData, 1, bytesOfKeyValueData) != bytesOfKeyValueData)
		return IL_FALSE;

	// Just a very rudimentary check on the origin values
	while (Pos < bytesOfKeyValueData)
	{
		Length = *((ILuint*)&KeyValueData[Pos]);
		if (Length >= 22)  // Size needed for the KTXorientation key
		{
			if (!strncmp((const char*)&KeyValueData[Pos + 4], "KTXorientation", 14))
			{
				// Found the orientation key we need
				ILubyte TopDownOrient = KeyValueData[Pos + 25];
				if (TopDownOrient == 'd')
					Origin = IL_ORIGIN_UPPER_LEFT;
				else if (TopDownOrient == 'u')
					Origin = IL_ORIGIN_LOWER_LEFT;
			}
		}

		Pos += Length;
		if (Pos >= bytesOfKeyValueData)
			break;
	}

	ifree(KeyValueData);

	return IL_TRUE;
}

ILboolean iKtxReadMipmaps(ILcontext* context, ILboolean Compressed, ILuint NumMips, ILenum Origin)
{
	ILimage *Image = context->impl->iCurImage;
	ILuint Width = context->impl->iCurImage->Width, Height = context->impl->iCurImage->Height;
	ILuint imageSize, Padding, Pos;

	for (ILuint Mip = 0; Mip < NumMips; Mip++)
	{
		imageSize = GetLittleUInt(context);
		Padding = 3 - ((Image->Bps + 3) % 4);

		if (imageSize != Image->SizeOfData + Padding * Image->Height)
		{
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			goto mip_fail;
		}

		// Note: The KTX spec at https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/
		//  seems to imply dword-alignment of the entire image, but this is per scanline.
		if (Image->Bps % 4 == 0)  // Required to be dword-aligned
		{
			if (context->impl->iread(context, Image->Data, 1, Image->SizeOfData) != Image->SizeOfData)
				return IL_FALSE;
		}
		else  // Since not dword-aligned, have to read each line separately.
		{
			Pos = 0;
			for (ILuint h = 0; h < Height; h++)
			{
				if (context->impl->iread(context, &Image->Data[Pos], 1, Image->Bps) != Image->Bps)
					return IL_FALSE;
				context->impl->iseek(context, Padding, IL_SEEK_CUR);
				Pos += Image->Bps;
			}
		}

		Image->Origin = Origin;
		Width = Width / 2;
		Height = Height / 2;

		if (Mip < NumMips - 1)
		{
			Image->Mipmaps = ilNewImage(context, Width, Height, 1, Image->Bpp, Image->Bpc);
			if (Image->Mipmaps == NULL)
				goto mip_fail;
			Image->Mipmaps->Format = Image->Format;
			Image->Mipmaps->Type = Image->Type;
			Image = Image->Mipmaps;
		}
	}

	return IL_TRUE;

mip_fail:
	Image = context->impl->iCurImage;
	ILimage *StartImage = Image->Mipmaps, *TempImage;
	while (StartImage) {
		TempImage = StartImage;
		StartImage = StartImage->Mipmaps;
		ifree(TempImage);
	}

	Image->Mipmaps = NULL;
	return IL_FALSE;
}

#endif//IL_NO_KTX