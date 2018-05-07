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
#include "il_dpx.h"
#include "il_bits.h"

ILboolean iLoadDpxInternal(ILcontext* context);


//! Reads a DPX file
ILboolean ilLoadDpx(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	DpxFile;
	ILboolean	bDpx = IL_FALSE;

	DpxFile = context->impl->iopenr(FileName);
	if (DpxFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bDpx;
	}

	bDpx = ilLoadDpxF(context, DpxFile);
	context->impl->icloser(DpxFile);

	return bDpx;
}


//! Reads an already-opened DPX file
ILboolean ilLoadDpxF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadDpxInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}


//! Reads from a memory "lump" that contains a DPX
ILboolean ilLoadDpxL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadDpxInternal(context);
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
ILboolean iLoadDpxInternal(ILcontext* context)
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

