//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/26/2009
//
// Filename: src-IL/src/il_dpx.c
//
// Description: Reads from a Digital Picture Exchange (.dpx) file.
//				Specifications for this format were	found at
//				http://www.cineon.com/ff_draft.php and
//				http://www.fileformat.info/format/dpx/.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_DPX

#include "il_bits.h"
#include "il_dpx.h"

#ifdef _WIN32
#pragma pack(push, packed_struct, 1)
#endif

typedef struct R32
{
	ILubyte	r, g, b, a;
} IL_PACKSTRUCT R32;

#ifdef _WIN32
#pragma pack(pop, packed_struct)
#endif

typedef struct DPX_FILE_INFO
{
    ILuint	MagicNum;        /* magic number 0x53445058 (SDPX) or 0x58504453 (XPDS) */
    ILuint	Offset;           /* offset to image data in bytes */
    ILbyte	Vers[8];          /* which header format version is being used (v1.0)*/
    ILuint	FileSize;        /* file size in bytes */
    ILuint	DittoKey;        /* read time short cut - 0 = same, 1 = new */
    ILuint	GenHdrSize;     /* generic header length in bytes */
    ILuint	IndHdrSize;     /* industry header length in bytes */
    ILuint	UserDataSize;   /* user-defined data length in bytes */
    ILbyte	FileName[100];   /* image file name */
    ILbyte	CreateTime[24];  /* file creation date "yyyy:mm:dd:hh:mm:ss:LTZ" */
    ILbyte	Creator[100];     /* file creator's name */
    ILbyte	Project[200];     /* project name */
    ILbyte	Copyright[200];   /* right to use or copyright info */
    ILuint	Key;              /* encryption ( FFFFFFFF = unencrypted ) */
    ILbyte	Reserved[104];    /* reserved field TBD (need to pad) */
} DPX_FILE_INFO;

typedef struct DPX_IMAGE_ELEMENT
{
    ILuint    DataSign;			/* data sign (0 = unsigned, 1 = signed ) */
								/* "Core set images are unsigned" */
    ILuint		RefLowData;		/* reference low data code value */
    R32			RefLowQuantity;	/* reference low quantity represented */
    ILuint		RefHighData;	/* reference high data code value */
    R32			RefHighQuantity;/* reference high quantity represented */
    ILubyte		Descriptor;		/* descriptor for image element */
    ILubyte		Transfer;		/* transfer characteristics for element */
    ILubyte		Colorimetric;	/* colormetric specification for element */
    ILubyte		BitSize;		/* bit size for element */
	ILushort	Packing;		/* packing for element */
    ILushort	Encoding;		/* encoding for element */
    ILuint		DataOffset;		/* offset to data of element */
    ILuint		EolPadding;		/* end of line padding used in element */
    ILuint		EoImagePadding;	/* end of image padding used in element */
    ILbyte		Description[32];/* description of element */
} DPX_IMAGE_ELEMENT;  /* NOTE THERE ARE EIGHT OF THESE */

typedef struct DPX_IMAGE_INFO
{
    ILushort	Orientation;          /* image orientation */
    ILushort	NumElements;       /* number of image elements */
    ILuint		Width;      /* or x value */
    ILuint		Height;  /* or y value, per element */
	DPX_IMAGE_ELEMENT	ImageElement[8];
    ILubyte		reserved[52];             /* reserved for future use (padding) */
} DPX_IMAGE_INFO;

typedef struct DPX_IMAGE_ORIENT
{
    ILuint		XOffset;               /* X offset */
    ILuint		YOffset;               /* Y offset */
    R32			XCenter;               /* X center */
    R32			YCenter;               /* Y center */
    ILuint		XOrigSize;            /* X original size */
    ILuint		YOrigSize;            /* Y original size */
    ILbyte		FileName[100];         /* source image file name */
    ILbyte		CreationTime[24];      /* source image creation date and time */
    ILbyte		InputDev[32];          /* input device name */
    ILbyte		InputSerial[32];       /* input device serial number */
    ILushort	Border[4];              /* border validity (XL, XR, YT, YB) */
    ILuint		PixelAspect[2];        /* pixel aspect ratio (H:V) */
    ILubyte		Reserved[28];           /* reserved for future use (padding) */
} DPX_IMAGE_ORIENT;

typedef struct DPX_MOTION_PICTURE_HEAD
{
    ILbyte film_mfg_id[2];    /* film manufacturer ID code (2 digits from film edge code) */
    ILbyte film_type[2];      /* file type (2 digits from film edge code) */
    ILbyte offset[2];         /* offset in perfs (2 digits from film edge code)*/
    ILbyte prefix[6];         /* prefix (6 digits from film edge code) */
    ILbyte count[4];          /* count (4 digits from film edge code)*/
    ILbyte format[32];        /* format (i.e. academy) */
    ILuint   frame_position;    /* frame position in sequence */
    ILuint   sequence_len;      /* sequence length in frames */
    ILuint   held_count;        /* held count (1 = default) */
    R32   frame_rate;        /* frame rate of original in frames/sec */
    R32   shutter_angle;     /* shutter angle of camera in degrees */
    ILbyte frame_id[32];      /* frame identification (i.e. keyframe) */
    ILbyte slate_info[100];   /* slate information */
    ILubyte    reserved[56];      /* reserved for future use (padding) */
} DPX_MOTION_PICTURE_HEAD;

typedef struct DPX_TELEVISION_HEAD
{
    ILuint tim_code;            /* SMPTE time code */
    ILuint userBits;            /* SMPTE user bits */
    ILubyte  interlace;           /* interlace ( 0 = noninterlaced, 1 = 2:1 interlace*/
    ILubyte  field_num;           /* field number */
    ILubyte  video_signal;        /* video signal standard (table 4)*/
    ILubyte  unused;              /* used for byte alignment only */
    R32 hor_sample_rate;     /* horizontal sampling rate in Hz */
    R32 ver_sample_rate;     /* vertical sampling rate in Hz */
    R32 frame_rate;          /* temporal sampling rate or frame rate in Hz */
    R32 time_offset;         /* time offset from sync to first pixel */
    R32 gamma;               /* gamma value */
    R32 black_level;         /* black level code value */
    R32 black_gain;          /* black gain */
    R32 break_point;         /* breakpoint */
    R32 white_level;         /* reference white level code value */
    R32 integration_times;   /* integration time(s) */
    ILubyte  reserved[76];        /* reserved for future use (padding) */
} DPX_TELEVISION_HEAD;

DpxHandler::DpxHandler(ILcontext* context) :
	context(context)
{

}

//! Reads a DPX file
ILboolean DpxHandler::load(ILconst_string FileName)
{
	ILHANDLE	DpxFile;
	ILboolean	bDpx = IL_FALSE;

	DpxFile = context->impl->iopenr(FileName);
	if (DpxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bDpx;
	}

	bDpx = loadF(DpxFile);
	context->impl->icloser(DpxFile);

	return bDpx;
}

//! Reads an already-opened DPX file
ILboolean DpxHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Reads from a memory "lump" that contains a DPX
ILboolean DpxHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

ILboolean DpxGetFileInfo(ILcontext* context, DPX_FILE_INFO *FileInfo)
{
	//if (context->impl->iread(context, FileInfo, 768, 1) != 1)
	//	return IL_FALSE;

	FileInfo->MagicNum = GetBigUInt(context);
	FileInfo->Offset = GetBigUInt(context);
	context->impl->iread(context, FileInfo->Vers, 8, 1);
	FileInfo->FileSize = GetBigUInt(context);
	FileInfo->DittoKey = GetBigUInt(context);
	FileInfo->GenHdrSize = GetBigUInt(context);
	FileInfo->IndHdrSize = GetBigUInt(context);
	FileInfo->UserDataSize = GetBigUInt(context);
	context->impl->iread(context, FileInfo->FileName, 100, 1);
	context->impl->iread(context, FileInfo->CreateTime, 24, 1);
	context->impl->iread(context, FileInfo->Creator, 100, 1);
	context->impl->iread(context, FileInfo->Project, 200, 1);
	if (context->impl->iread(context, FileInfo->Copyright, 200, 1) != 1)
		return IL_FALSE;
	FileInfo->Key = GetBigUInt(context);
	context->impl->iseek(context, 104, IL_SEEK_CUR);  // Skip reserved section.

	return IL_TRUE;
}

ILboolean GetImageElement(ILcontext* context, DPX_IMAGE_ELEMENT *ImageElement)
{
	ImageElement->DataSign = GetBigUInt(context);
	ImageElement->RefLowData = GetBigUInt(context);
	context->impl->iread(context, &ImageElement->RefLowQuantity, 1, 4);
	ImageElement->RefHighData = GetBigUInt(context);
	context->impl->iread(context, &ImageElement->RefHighQuantity, 1, 4);
	ImageElement->Descriptor = context->impl->igetc(context);
	ImageElement->Transfer = context->impl->igetc(context);
	ImageElement->Colorimetric = context->impl->igetc(context);
	ImageElement->BitSize = context->impl->igetc(context);
	ImageElement->Packing = GetBigUShort(context);
	ImageElement->Encoding = GetBigUShort(context);
	ImageElement->DataOffset = GetBigUInt(context);
	ImageElement->EolPadding = GetBigUInt(context);
	ImageElement->EoImagePadding = GetBigUInt(context);
	if (context->impl->iread(context, ImageElement->Description, 32, 1) != 1)
		return IL_FALSE;

	return IL_TRUE;
}

ILboolean DpxGetImageInfo(ILcontext* context, DPX_IMAGE_INFO *ImageInfo)
{
	ILuint i;

	//if (context->impl->iread(context, ImageInfo, sizeof(DPX_IMAGE_INFO), 1) != 1)
	//	return IL_FALSE;
	ImageInfo->Orientation = GetBigUShort(context);
	ImageInfo->NumElements = GetBigUShort(context);
	ImageInfo->Width = GetBigUInt(context);
	ImageInfo->Height = GetBigUInt(context);

	for (i = 0; i < 8; i++) {
		GetImageElement(context, &ImageInfo->ImageElement[i]);
	}

	context->impl->iseek(context, 52, IL_SEEK_CUR);  // Skip padding bytes.

	return IL_TRUE;
}

ILboolean DpxGetImageOrient(ILcontext* context, DPX_IMAGE_ORIENT *ImageOrient)
{
	ImageOrient->XOffset = GetBigUInt(context);
	ImageOrient->YOffset = GetBigUInt(context);
	context->impl->iread(context, &ImageOrient->XCenter, 4, 1);
	context->impl->iread(context, &ImageOrient->YCenter, 4, 1);
	ImageOrient->XOrigSize = GetBigUInt(context);
	ImageOrient->YOrigSize = GetBigUInt(context);
	context->impl->iread(context, ImageOrient->FileName, 100, 1);
	context->impl->iread(context, ImageOrient->CreationTime, 24, 1);
	context->impl->iread(context, ImageOrient->InputDev, 32, 1);
	if (context->impl->iread(context, ImageOrient->InputSerial, 32, 1) != 1)
		return IL_FALSE;
	ImageOrient->Border[0] = GetBigUShort(context);
	ImageOrient->Border[1] = GetBigUShort(context);
	ImageOrient->Border[2] = GetBigUShort(context);
	ImageOrient->Border[3] = GetBigUShort(context);
	ImageOrient->PixelAspect[0] = GetBigUInt(context);
	ImageOrient->PixelAspect[1] = GetBigUInt(context);
	context->impl->iseek(context, 28, IL_SEEK_CUR);  // Skip reserved bytes.

	return IL_TRUE;
}

// Internal function used to load the DPX.
ILboolean DpxHandler::loadInternal()
{
	DPX_FILE_INFO		FileInfo;
	DPX_IMAGE_INFO		ImageInfo;
	DPX_IMAGE_ORIENT	ImageOrient;
//	BITFILE		*File;
	ILuint		i, NumElements, CurElem = 0;
	ILushort	Val, *ShortData;
	ILubyte		Data[8];
	ILenum		Format = 0;
	ILubyte		NumChans = 0;


	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	
	if (!DpxGetFileInfo(context, &FileInfo))
		return IL_FALSE;
	if (!DpxGetImageInfo(context, &ImageInfo))
		return IL_FALSE;
	if (!DpxGetImageOrient(context, &ImageOrient))
		return IL_FALSE;

	context->impl->iseek(context, ImageInfo.ImageElement[CurElem].DataOffset, IL_SEEK_SET);

//@TODO: Deal with different origins!

	switch (ImageInfo.ImageElement[CurElem].Descriptor)
	{
		case 6:  // Luminance data
			Format = IL_LUMINANCE;
			NumChans = 1;
			break;
		case 50:  // RGB data
			Format = IL_RGB;
			NumChans = 3;
			break;
		case 51:  // RGBA data
			Format = IL_RGBA;
			NumChans = 4;
			break;
		default:
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	// These are all on nice word boundaries.
	switch (ImageInfo.ImageElement[CurElem].BitSize)
	{
		case 8:
		case 16:
		case 32:
			if (!ilTexImage(context, ImageInfo.Width, ImageInfo.Height, 1, NumChans, Format, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
			if (context->impl->iread(context, context->impl->iCurImage->Data, context->impl->iCurImage->SizeOfData, 1) != 1)
				return IL_FALSE;
			goto finish;
	}

	// The rest of these do not end on word boundaries.
	if (ImageInfo.ImageElement[CurElem].Packing == 1) {
		// Here we have it padded out to a word boundary, so the extra bits are filler.
		switch (ImageInfo.ImageElement[CurElem].BitSize)
		{
			case 10:
				//@TODO: Support other formats!
				/*if (Format != IL_RGB) {
					ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
					return IL_FALSE;
				}*/
				switch (Format)
				{
					case IL_LUMINANCE:
						if (!ilTexImage(context, ImageInfo.Width, ImageInfo.Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)context->impl->iCurImage->Data;
						NumElements = context->impl->iCurImage->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							context->impl->iread(context, Data, 1, 2);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
						}
						break;

					case IL_RGB:
						if (!ilTexImage(context, ImageInfo.Width, ImageInfo.Height, 1, 3, IL_RGB, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)context->impl->iCurImage->Data;
						NumElements = context->impl->iCurImage->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							context->impl->iread(context, Data, 1, 4);
							Val = ((Data[0] << 2) + ((Data[1] & 0xC0) >> 6)) << 6;  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = (((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4)) << 6;  // Use the next 10 bits.
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
							Val = (((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2)) << 6;  // And finally use the last 10 bits (ignores the last 2 bits).
							ShortData[i++] = Val | ((Val & 0x3F0) >> 4);  // Same fill
						}
						break;

					case IL_RGBA:  // Is this even a possibility?  There is a ton of wasted space here!
						if (!ilTexImage(context, ImageInfo.Width, ImageInfo.Height, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, NULL))
							return IL_FALSE;
						context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
						ShortData = (ILushort*)context->impl->iCurImage->Data;
						NumElements = context->impl->iCurImage->SizeOfData / 2;

						for (i = 0; i < NumElements;) {
							context->impl->iread(context, Data, 1, 8);
							Val = (Data[0] << 2) + ((Data[1] & 0xC0) >> 6);  // Use the first 10 bits of the word-aligned data.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Fill in the lower 6 bits with a copy of the higher bits.
							Val = ((Data[1] & 0x3F) << 4) + ((Data[2] & 0xF0) >> 4);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[2] & 0x0F) << 6) + ((Data[3] & 0xFC) >> 2);  // Use the next 10 bits.
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Same fill
							Val = ((Data[3] & 0x03) << 8) + Data[4];  // And finally use the last 10 relevant bits (skips 3 whole bytes worth of padding!).
							ShortData[i++] = (Val << 6) | ((Val & 0x3F0) >> 4);  // Last fill
						}
						break;
				}
				break;

			//case 1:
			//case 12:
			default:
				ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
				return IL_FALSE;
		}
	}
	else if (ImageInfo.ImageElement[0].Packing == 0) {
		// Here we have the data packed so that it is never aligned on a word boundary.
		/*File = bfile(iGetFile());
		if (File == NULL)
			return IL_FALSE;  //@TODO: Error?
		ShortData = (ILushort*)context->impl->iCurImage->Data;
		NumElements = context->impl->iCurImage->SizeOfData / 2;
		for (i = 0; i < NumElements; i++) {
			//bread(&Val, 1, 10, File);
			Val = breadVal(10, File);
			ShortData[i] = (Val << 6) | (Val >> 4);
		}
		bclose(File);*/

		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	else {
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;  //@TODO: Take care of this in an iCheckDpx* function.
	}

finish:
	return ilFixImage(context);
}

#endif//IL_NO_DPX