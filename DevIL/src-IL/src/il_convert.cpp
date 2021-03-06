//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_convert.cpp
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include <limits.h>


ILimage *iConvertPalette(ILcontext* context, ILimage *Image, ILenum DestFormat)
{
	static const ILfloat LumFactor[3] = { 0.212671f, 0.715160f, 0.072169f };  // http://www.inforamp.net/~poynton/ and libpng's libpng.txt - Used for conversion to luminance.
	ILimage		*NewImage = NULL, *CurImage = NULL;
	ILuint		i, j, k, c, Size, LumBpp = 1;
	ILfloat		Resultf;
	ILubyte		*Temp = NULL;
	ILboolean	Converted;
	ILboolean	HasAlpha;

	NewImage = (ILimage*)icalloc(context, 1, sizeof(ILimage));  // Much better to have it all set to 0.
	if (NewImage == NULL) {
		return IL_FALSE;
	}

	ilCopyImageAttr(context, NewImage, Image);

	if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE || Image->Bpp != 1) {
		ilCloseImage(NewImage);
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return NULL;
	}

	if (DestFormat == IL_LUMINANCE || DestFormat == IL_LUMINANCE_ALPHA) {
		if (NewImage->Pal.Palette)
			ifree(NewImage->Pal.Palette);
		if (DestFormat == IL_LUMINANCE_ALPHA)
			LumBpp = 2;

		switch (context->impl->iCurImage->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				Temp = (ILubyte*)ialloc(context, LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;
					for (c = 0; c < Size; c++) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[c];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (context->impl->iCurImage->Pal.PalType == IL_PAL_RGBA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;

			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				Temp = (ILubyte*)ialloc(context, LumBpp * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += LumBpp) {
					Resultf = 0.0f;  j = 2;
					for (c = 0; c < Size; c++, j--) {
						Resultf += Image->Pal.Palette[i + c] * LumFactor[j];
					}
					Temp[k] = (ILubyte)Resultf;
					if (LumBpp == 2) {
						if (context->impl->iCurImage->Pal.PalType == IL_PAL_RGBA32)
							Temp[k+1] = Image->Pal.Palette[i + 3];
						else
							Temp[k+1] = 0xff;
					}
				}

				break;
		}

		NewImage->Pal.Palette = NULL;
		NewImage->Pal.PalSize = 0;
		NewImage->Pal.PalType = IL_PAL_NONE;
		NewImage->Format = DestFormat;
		NewImage->Bpp = LumBpp;
		NewImage->Bps = NewImage->Width * LumBpp;
		NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->Data = (ILubyte*)ialloc(context, NewImage->SizeOfData);
		if (NewImage->Data == NULL)
			goto alloc_error;

		if (LumBpp == 2) {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
				NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
			}
		}
		else {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i] = Temp[Image->Data[i]];
			}
		}

		ifree(Temp);

		return NewImage;
	}
	else if (DestFormat == IL_ALPHA) {
		if (NewImage->Pal.Palette)
			ifree(NewImage->Pal.Palette);

		switch (context->impl->iCurImage->Pal.PalType)
		{
			// Opaque, so all the values are 0xFF.
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
				HasAlpha = IL_FALSE;
				break;

			case IL_PAL_BGRA32:
			case IL_PAL_RGBA32:
				HasAlpha = IL_TRUE;
				Temp = (ILubyte*)ialloc(context, 1 * Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType));
				if (Temp == NULL)
					goto alloc_error;

				Size = ilGetBppPal(Image->Pal.PalType);
				for (i = 0, k = 0; i < Image->Pal.PalSize; i += Size, k += 1) {
					Temp[k] = Image->Pal.Palette[i + 3];
				}

				break;
		}

		NewImage->Pal.Palette = NULL;
		NewImage->Pal.PalSize = 0;
		NewImage->Pal.PalType = IL_PAL_NONE;
		NewImage->Format = DestFormat;
		NewImage->Bpp = LumBpp;
		NewImage->Bps = NewImage->Width * 1;  // Alpha is only one byte.
		NewImage->SizeOfData = NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->Data = (ILubyte*)ialloc(context, NewImage->SizeOfData);
		if (NewImage->Data == NULL)
			goto alloc_error;

		if (HasAlpha) {
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i*2] = Temp[Image->Data[i] * 2];
				NewImage->Data[i*2+1] = Temp[Image->Data[i] * 2 + 1];
			}
		}
		else {  // No alpha, opaque.
			for (i = 0; i < Image->SizeOfData; i++) {
				NewImage->Data[i] = 0xFF;
			}
		}

		ifree(Temp);

		return NewImage;
	}

	//NewImage->Format = ilGetPalBaseType(context->impl->iCurImage->Pal.PalType);
	NewImage->Format = DestFormat;

	if (ilGetBppFormat(NewImage->Format) == 0) {
		ilCloseImage(NewImage);
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return NULL;
	}

	CurImage = context->impl->iCurImage;
	ilSetCurImage(context, NewImage);

	switch (DestFormat)
	{
		case IL_RGB:
			Converted = ilConvertPal(context, IL_PAL_RGB24);
			break;

		case IL_BGR:
			Converted = ilConvertPal(context, IL_PAL_BGR24);
			break;

		case IL_RGBA:
			Converted = ilConvertPal(context, IL_PAL_RGB32);
			break;

		case IL_BGRA:
			Converted = ilConvertPal(context, IL_PAL_BGR32);
			break;

		case IL_COLOUR_INDEX:
			// Just copy the original image over.
			NewImage->Data = (ILubyte*)ialloc(context, CurImage->SizeOfData);
			if (NewImage->Data == NULL)
				goto alloc_error;
			NewImage->Pal.Palette = (ILubyte*)ialloc(context, context->impl->iCurImage->Pal.PalSize);
			if (NewImage->Pal.Palette == NULL)
				goto alloc_error;
			memcpy(NewImage->Data, CurImage->Data, CurImage->SizeOfData);
			memcpy(NewImage->Pal.Palette, Image->Pal.Palette, Image->Pal.PalSize);
			NewImage->Pal.PalSize = Image->Pal.PalSize;
			NewImage->Pal.PalType = Image->Pal.PalType;
			ilSetCurImage(context, CurImage);
			return NewImage;

		default:
			ilCloseImage(NewImage);
			ilSetError(context, IL_INVALID_CONVERSION);
			return NULL;
	}

	// Resize to new bpp
	ilResizeImage(context, NewImage, NewImage->Width, NewImage->Height, NewImage->Depth, ilGetBppFormat(DestFormat), /*ilGetBpcType(DestType)*/1);

	// ilConvertPal already sets the error message - no need to confuse the user.
	if (!Converted) {
		ilSetCurImage(context, CurImage);
		ilCloseImage(NewImage);
		return NULL;
	}

	Size = ilGetBppPal(NewImage->Pal.PalType);
	for (i = 0; i < Image->SizeOfData; i++) {
		for (c = 0; c < NewImage->Bpp; c++) {
			NewImage->Data[i * NewImage->Bpp + c] = NewImage->Pal.Palette[Image->Data[i] * Size + c];
		}
	}

	ifree(NewImage->Pal.Palette);

	NewImage->Pal.Palette = NULL;
	NewImage->Pal.PalSize = 0;
	NewImage->Pal.PalType = IL_PAL_NONE;
	ilSetCurImage(context, CurImage);

	return NewImage;

alloc_error:
	ifree(Temp);
	if (NewImage)
		ilCloseImage(NewImage);
	if (CurImage != context->impl->iCurImage)
		ilSetCurImage(context, CurImage);
	return NULL;
}


// In il_quantizer.c
ILimage *iQuantizeImage(ILcontext* context, ILimage *Image, ILuint NumCols);
// In il_neuquant.c
ILimage *iNeuQuant(ILcontext* context, ILimage *Image, ILuint NumCols);

// Converts an image from one format to another
ILAPI ILimage* ILAPIENTRY iConvertImage(ILcontext* context, ILimage *Image, ILenum DestFormat, ILenum DestType)
{
	ILimage	*NewImage, *CurImage;
	ILuint	i;
	ILubyte	*NewData;

	CurImage = Image;
	if (Image == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	// We don't support 16-bit color indices (or higher).
	if (DestFormat == IL_COLOUR_INDEX && DestType >= IL_SHORT) {
		ilSetError(context, IL_INVALID_CONVERSION);
		return NULL;
	}

	if (Image->Format == IL_COLOUR_INDEX) {
		NewImage = iConvertPalette(context, Image, DestFormat);

		//added test 2003-09-01
		if (NewImage == NULL)
			return NULL;

		if (DestType == NewImage->Type)
			return NewImage;

		NewData = (ILubyte*)ilConvertBuffer(context, NewImage->SizeOfData, NewImage->Format, DestFormat, NewImage->Type, DestType, NULL, NewImage->Data);
		if (NewData == NULL) {
			ifree(NewImage);  // ilCloseImage not needed.
			return NULL;
		}
		ifree(NewImage->Data);
		NewImage->Data = NewData;

		ilCopyImageAttr(context, NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBpcType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;
	}
	else if (DestFormat == IL_COLOUR_INDEX && Image->Format != IL_LUMINANCE) {
		if (iGetInt(context, IL_QUANTIZATION_MODE) == IL_NEU_QUANT)
			return iNeuQuant(context, Image, iGetInt(context, IL_MAX_QUANT_INDICES));
		else // Assume IL_WU_QUANT otherwise.
			return iQuantizeImage(context, Image, iGetInt(context, IL_MAX_QUANT_INDICES));
	}
	else {
		NewImage = (ILimage*)icalloc(context, 1, sizeof(ILimage));  // Much better to have it all set to 0.
		if (NewImage == NULL) {
			return NULL;
		}

		if (ilGetBppFormat(DestFormat) == 0) {
			ilSetError(context, IL_INVALID_PARAM);
			ifree(NewImage);
			return NULL;
		}

		ilCopyImageAttr(context, NewImage, Image);
		NewImage->Format = DestFormat;
		NewImage->Type = DestType;
		NewImage->Bpc = ilGetBpcType(DestType);
		NewImage->Bpp = ilGetBppFormat(DestFormat);
		NewImage->Bps = NewImage->Bpp * NewImage->Bpc * NewImage->Width;
		NewImage->SizeOfPlane = NewImage->Bps * NewImage->Height;
		NewImage->SizeOfData = NewImage->SizeOfPlane * NewImage->Depth;

		if (DestFormat == IL_COLOUR_INDEX && Image->Format == IL_LUMINANCE) {
			NewImage->Pal.PalSize = 768;
			NewImage->Pal.PalType = IL_PAL_RGB24;
			NewImage->Pal.Palette = (ILubyte*)ialloc(context, 768);
			for (i = 0; i < 256; i++) {
				NewImage->Pal.Palette[i * 3] = i;
				NewImage->Pal.Palette[i * 3 + 1] = i;
				NewImage->Pal.Palette[i * 3 + 2] = i;
			}
			NewImage->Data = (ILubyte*)ialloc(context, Image->SizeOfData);
			if (NewImage->Data == NULL) {
				ilCloseImage(NewImage);
				return NULL;
			}
			memcpy(NewImage->Data, Image->Data, Image->SizeOfData);
		}
		else {
			NewImage->Data = (ILubyte*)ilConvertBuffer(context, Image->SizeOfData, Image->Format, DestFormat, Image->Type, DestType, NULL, Image->Data);
			if (NewImage->Data == NULL) {
				ifree(NewImage);  // ilCloseImage not needed.
				return NULL;
			}
		}
	}

	return NewImage;
}


//! Converts the current image to the DestFormat format.
/*! \param DestFormat An enum of the desired output format.  Any format values are accepted.
    \param DestType An enum of the desired output type.  Any type values are accepted.
	\exception IL_ILLEGAL_OPERATION No currently bound image
	\exception IL_INVALID_CONVERSION DestFormat or DestType was an invalid identifier.
	\exception IL_OUT_OF_MEMORY Could not allocate enough memory.
	\return Boolean value of failure or success*/
ILboolean ILAPIENTRY ilConvertImage(ILcontext* context, ILenum DestFormat, ILenum DestType)
{
	ILimage *Image, *pCurImage;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (DestFormat == context->impl->iCurImage->Format && DestType == context->impl->iCurImage->Type)
		return IL_TRUE;  // No conversion needed.

	if (DestType == context->impl->iCurImage->Type) {
		if (iFastConvert(context, DestFormat)) {
			context->impl->iCurImage->Format = DestFormat;
			return IL_TRUE;
		}
	}

	if (ilIsEnabled(context, IL_USE_KEY_COLOUR)) {
		ilAddAlphaKey(context, context->impl->iCurImage);
	}

	pCurImage = context->impl->iCurImage;
	while (pCurImage != NULL)
	{
		Image = iConvertImage(context, pCurImage, DestFormat, DestType);
		if (Image == NULL)
			return IL_FALSE;

		//ilCopyImageAttr(pCurImage, Image);  // Destroys subimages.

		// We don't copy the colour profile here, since it stays the same.
		//	Same with the DXTC data.
		pCurImage->Format = DestFormat;
		pCurImage->Type = DestType;
		pCurImage->Bpc = ilGetBpcType(DestType);
		pCurImage->Bpp = ilGetBppFormat(DestFormat);
		pCurImage->Bps = pCurImage->Width * pCurImage->Bpc * pCurImage->Bpp;
		pCurImage->SizeOfPlane = pCurImage->Bps * pCurImage->Height;
		pCurImage->SizeOfData = pCurImage->Depth * pCurImage->SizeOfPlane;
		if (pCurImage->Pal.Palette && pCurImage->Pal.PalSize && pCurImage->Pal.PalType != IL_PAL_NONE)
			ifree(pCurImage->Pal.Palette);
		pCurImage->Pal.Palette = Image->Pal.Palette;
		pCurImage->Pal.PalSize = Image->Pal.PalSize;
		pCurImage->Pal.PalType = Image->Pal.PalType;
		Image->Pal.Palette = NULL;
		ifree(pCurImage->Data);
		pCurImage->Data = Image->Data;
		Image->Data = NULL;
		ilCloseImage(Image);

		pCurImage = pCurImage->Next;
	}

	return IL_TRUE;
}


// Swaps the colour order of the current image (rgb(a)->bgr(a) or vice-versa).
//	Must be either an 8, 24 or 32-bit (coloured) image (or palette).
ILboolean ilSwapColours(ILcontext* context)
{
	ILuint		i = 0, Size = context->impl->iCurImage->Bpp * context->impl->iCurImage->Width * context->impl->iCurImage->Height;
	ILbyte		PalBpp = ilGetBppPal(context->impl->iCurImage->Pal.PalType);
	ILushort	*ShortPtr;
	ILuint		*IntPtr, Temp;
	ILdouble	*DoublePtr, DoubleTemp;

	if ((context->impl->iCurImage->Bpp != 1 && context->impl->iCurImage->Bpp != 3 && context->impl->iCurImage->Bpp != 4)) {
		ilSetError(context, IL_INVALID_VALUE);
		return IL_FALSE;
	}

	// Just check before we change the format.
	if (context->impl->iCurImage->Format == IL_COLOUR_INDEX) {
		if (PalBpp == 0 || context->impl->iCurImage->Format != IL_COLOUR_INDEX) {
			ilSetError(context, IL_ILLEGAL_OPERATION);
			return IL_FALSE;
		}
	}

	switch (context->impl->iCurImage->Format)
	{
		case IL_RGB:
			context->impl->iCurImage->Format = IL_BGR;
			break;
		case IL_RGBA:
			context->impl->iCurImage->Format = IL_BGRA;
			break;
		case IL_BGR:
			context->impl->iCurImage->Format = IL_RGB;
			break;
		case IL_BGRA:
			context->impl->iCurImage->Format = IL_RGBA;
			break;
		case IL_ALPHA:
		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			return IL_TRUE;  // No need to do anything to luminance or alpha images.
		case IL_COLOUR_INDEX:
			switch (context->impl->iCurImage->Pal.PalType)
			{
				case IL_PAL_RGB24:
					context->impl->iCurImage->Pal.PalType = IL_PAL_BGR24;
					break;
				case IL_PAL_RGB32:
					context->impl->iCurImage->Pal.PalType = IL_PAL_BGR32;
					break;
				case IL_PAL_RGBA32:
					context->impl->iCurImage->Pal.PalType = IL_PAL_BGRA32;
					break;
				case IL_PAL_BGR24:
					context->impl->iCurImage->Pal.PalType = IL_PAL_RGB24;
					break;
				case IL_PAL_BGR32:
					context->impl->iCurImage->Pal.PalType = IL_PAL_RGB32;
					break;
				case IL_PAL_BGRA32:
					context->impl->iCurImage->Pal.PalType = IL_PAL_RGBA32;
					break;
				default:
					ilSetError(context, IL_ILLEGAL_OPERATION);
					return IL_FALSE;
			}
			break;
		default:
			ilSetError(context, IL_ILLEGAL_OPERATION);
			return IL_FALSE;
	}

	if (context->impl->iCurImage->Format == IL_COLOUR_INDEX) {
		for (; i < context->impl->iCurImage->Pal.PalSize; i += PalBpp) {
				Temp = context->impl->iCurImage->Pal.Palette[i];
				context->impl->iCurImage->Pal.Palette[i] = context->impl->iCurImage->Pal.Palette[i+2];
				context->impl->iCurImage->Pal.Palette[i+2] = Temp;
		}
	}
	else {
		ShortPtr = (ILushort*)context->impl->iCurImage->Data;
		IntPtr = (ILuint*)context->impl->iCurImage->Data;
		DoublePtr = (ILdouble*)context->impl->iCurImage->Data;
		switch (context->impl->iCurImage->Bpc)
		{
			case 1:
				for (; i < Size; i += context->impl->iCurImage->Bpp) {
					Temp = context->impl->iCurImage->Data[i];
					context->impl->iCurImage->Data[i] = context->impl->iCurImage->Data[i+2];
					context->impl->iCurImage->Data[i+2] = Temp;
				}
				break;
			case 2:
				for (; i < Size; i += context->impl->iCurImage->Bpp) {
					Temp = ShortPtr[i];
					ShortPtr[i] = ShortPtr[i+2];
					ShortPtr[i+2] = Temp;
				}
				break;
			case 4:  // Works fine with ILint, ILuint and ILfloat.
				for (; i < Size; i += context->impl->iCurImage->Bpp) {
					Temp = IntPtr[i];
					IntPtr[i] = IntPtr[i+2];
					IntPtr[i+2] = Temp;
				}
				break;
			case 8:
				for (; i < Size; i += context->impl->iCurImage->Bpp) {
					DoubleTemp = DoublePtr[i];
					DoublePtr[i] = DoublePtr[i+2];
					DoublePtr[i+2] = DoubleTemp;
				}
				break;
		}
	}

	return IL_TRUE;
}


// Adds an opaque alpha channel to a 24-bit image
ILboolean ilAddAlpha(ILcontext* context)
{
	ILubyte		*NewData, NewBpp;
	ILuint		i = 0, j = 0, Size;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Bpp != 3) {
		ilSetError(context, IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = context->impl->iCurImage->Bps * context->impl->iCurImage->Height / context->impl->iCurImage->Bpc;
	NewBpp = (ILubyte)(context->impl->iCurImage->Bpp + 1);
	
	NewData = (ILubyte*)ialloc(context, NewBpp * context->impl->iCurImage->Bpc * context->impl->iCurImage->Width * context->impl->iCurImage->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (context->impl->iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				NewData[j]   = context->impl->iCurImage->Data[i];
				NewData[j+1] = context->impl->iCurImage->Data[i+1];
				NewData[j+2] = context->impl->iCurImage->Data[i+2];
				NewData[j+3] = UCHAR_MAX;  // Max opaqueness
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)context->impl->iCurImage->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)context->impl->iCurImage->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)context->impl->iCurImage->Data)[i+2];
				((ILushort*)NewData)[j+3] = USHRT_MAX;
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)context->impl->iCurImage->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)context->impl->iCurImage->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)context->impl->iCurImage->Data)[i+2];
				((ILuint*)NewData)[j+3] = UINT_MAX;
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)context->impl->iCurImage->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)context->impl->iCurImage->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)context->impl->iCurImage->Data)[i+2];
				((ILfloat*)NewData)[j+3] = 1.0f;
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)context->impl->iCurImage->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)context->impl->iCurImage->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)context->impl->iCurImage->Data)[i+2];
				((ILdouble*)NewData)[j+3] = 1.0;
			}
			break;

		default:
			ifree(NewData);
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	context->impl->iCurImage->Bpp = NewBpp;
	context->impl->iCurImage->Bps = context->impl->iCurImage->Width * context->impl->iCurImage->Bpc * NewBpp;
	context->impl->iCurImage->SizeOfPlane = context->impl->iCurImage->Bps * context->impl->iCurImage->Height;
	context->impl->iCurImage->SizeOfData = context->impl->iCurImage->SizeOfPlane * context->impl->iCurImage->Depth;
	ifree(context->impl->iCurImage->Data);
	context->impl->iCurImage->Data = NewData;

	switch (context->impl->iCurImage->Format)
	{
		case IL_RGB:
			context->impl->iCurImage->Format = IL_RGBA;
			break;
		case IL_BGR:
			context->impl->iCurImage->Format = IL_BGRA;
			break;
	}

	return IL_TRUE;
}


//ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;

void ILAPIENTRY ilKeyColour(ILclampf Red, ILclampf Green, ILclampf Blue, ILclampf Alpha)
{
	ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;
        KeyRed		= Red;
	KeyGreen	= Green;
	KeyBlue		= Blue;
	KeyAlpha	= Alpha;
	return;
}


// Adds an alpha channel to an 8 or 24-bit image,
//	making the image transparent where Key is equal to the pixel.
ILboolean ilAddAlphaKey(ILcontext* context, ILimage *Image)
{
	ILfloat KeyRed = 0, KeyGreen = 0, KeyBlue = 0, KeyAlpha = 0;
        ILubyte		*NewData, NewBpp;
	ILfloat		KeyColour[3];
	ILuint		i = 0, j = 0, c, Size;
	ILboolean	Same;

	if (Image == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Image->Format != IL_COLOUR_INDEX) {
		if (Image->Bpp != 3) {
			ilSetError(context, IL_INVALID_VALUE);
			return IL_FALSE;
		}

		if (Image->Format == IL_BGR || Image->Format == IL_BGRA) {
			KeyColour[0] = KeyBlue;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyRed;
		}
		else {
			KeyColour[0] = KeyRed;
			KeyColour[1] = KeyGreen;
			KeyColour[2] = KeyBlue;
		}

		Size = Image->Bps * Image->Height / Image->Bpc;

		NewBpp = (ILubyte)(Image->Bpp + 1);
		
		NewData = (ILubyte*)ialloc(context, NewBpp * Image->Bpc * Image->Width * Image->Height);
		if (NewData == NULL) {
			return IL_FALSE;
		}

		switch (Image->Type)
		{
			case IL_BYTE:
			case IL_UNSIGNED_BYTE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					NewData[j]   = Image->Data[i];
					NewData[j+1] = Image->Data[i+1];
					NewData[j+2] = Image->Data[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (Image->Data[i+c] != KeyColour[c] * UCHAR_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						NewData[j+3] = 0;  // Transparent - matches key colour
					else
						NewData[j+3] = UCHAR_MAX;
				}
				break;

			case IL_SHORT:
			case IL_UNSIGNED_SHORT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILushort*)NewData)[j]   = ((ILushort*)Image->Data)[i];
					((ILushort*)NewData)[j+1] = ((ILushort*)Image->Data)[i+1];
					((ILushort*)NewData)[j+2] = ((ILushort*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILushort*)Image->Data)[i+c] != KeyColour[c] * USHRT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILushort*)NewData)[j+3] = 0;
					else
						((ILushort*)NewData)[j+3] = USHRT_MAX;
				}
				break;

			case IL_INT:
			case IL_UNSIGNED_INT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILuint*)NewData)[j]   = ((ILuint*)Image->Data)[i];
					((ILuint*)NewData)[j+1] = ((ILuint*)Image->Data)[i+1];
					((ILuint*)NewData)[j+2] = ((ILuint*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILuint*)Image->Data)[i+c] != KeyColour[c] * UINT_MAX)
							Same = IL_FALSE;
					}

					if (Same)
						((ILuint*)NewData)[j+3] = 0;
					else
						((ILuint*)NewData)[j+3] = UINT_MAX;
				}
				break;

			case IL_FLOAT:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILfloat*)NewData)[j]   = ((ILfloat*)Image->Data)[i];
					((ILfloat*)NewData)[j+1] = ((ILfloat*)Image->Data)[i+1];
					((ILfloat*)NewData)[j+2] = ((ILfloat*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILfloat*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILfloat*)NewData)[j+3] = 0.0f;
					else
						((ILfloat*)NewData)[j+3] = 1.0f;
				}
				break;

			case IL_DOUBLE:
				for (; i < Size; i += Image->Bpp, j += NewBpp) {
					((ILdouble*)NewData)[j]   = ((ILdouble*)Image->Data)[i];
					((ILdouble*)NewData)[j+1] = ((ILdouble*)Image->Data)[i+1];
					((ILdouble*)NewData)[j+2] = ((ILdouble*)Image->Data)[i+2];
					Same = IL_TRUE;
					for (c = 0; c < Image->Bpp; c++) {
						if (((ILdouble*)Image->Data)[i+c] != KeyColour[c])
							Same = IL_FALSE;
					}

					if (Same)
						((ILdouble*)NewData)[j+3] = 0.0;
					else
						((ILdouble*)NewData)[j+3] = 1.0;
				}
				break;

			default:
				ifree(NewData);
				ilSetError(context, IL_INTERNAL_ERROR);
				return IL_FALSE;
		}


		Image->Bpp = NewBpp;
		Image->Bps = Image->Width * Image->Bpc * NewBpp;
		Image->SizeOfPlane = Image->Bps * Image->Height;
		Image->SizeOfData = Image->SizeOfPlane * Image->Depth;
		ifree(Image->Data);
		Image->Data = NewData;

		switch (Image->Format)
		{
			case IL_RGB:
				Image->Format = IL_RGBA;
				break;
			case IL_BGR:
				Image->Format = IL_BGRA;
				break;
		}
	}
	else {  // IL_COLOUR_INDEX
		if (Image->Bpp != 1) {
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		Size = ilGetInteger(context, IL_PALETTE_NUM_COLS);
		if (Size == 0) {
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
		}

		if ((ILuint)(KeyAlpha * UCHAR_MAX) > Size) {
			ilSetError(context, IL_INVALID_VALUE);
			return IL_FALSE;
		}

		switch (Image->Pal.PalType)
		{
			case IL_PAL_RGB24:
			case IL_PAL_RGB32:
			case IL_PAL_RGBA32:
				if (!ilConvertPal(context, IL_PAL_RGBA32))
					return IL_FALSE;
				break;
			case IL_PAL_BGR24:
			case IL_PAL_BGR32:
			case IL_PAL_BGRA32:
				if (!ilConvertPal(context, IL_PAL_BGRA32))
					return IL_FALSE;
				break;
			default:
				ilSetError(context, IL_INTERNAL_ERROR);
				return IL_FALSE;
		}

		// Set the colour index to be transparent.
		Image->Pal.Palette[(ILuint)(KeyAlpha * UCHAR_MAX) * 4 + 3] = 0;

		// @TODO: Check if this is the required behaviour.

		if (Image->Pal.PalType == IL_PAL_RGBA32)
			ilConvertImage(context, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			ilConvertImage(context, IL_BGRA, IL_UNSIGNED_BYTE);
	}

	return IL_TRUE;
}


// Removes alpha from a 32-bit image
//	Should we maybe add an option that changes the image based on the alpha?
ILboolean ilRemoveAlpha(ILcontext* context)
{
        ILubyte *NewData, NewBpp;
	ILuint i = 0, j = 0, Size;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (context->impl->iCurImage->Bpp != 4) {
		ilSetError(context, IL_INVALID_VALUE);
		return IL_FALSE;
	}

	Size = context->impl->iCurImage->Bps * context->impl->iCurImage->Height;
	NewBpp = (ILubyte)(context->impl->iCurImage->Bpp - 1);
	
	NewData = (ILubyte*)ialloc(context, NewBpp * context->impl->iCurImage->Bpc * context->impl->iCurImage->Width * context->impl->iCurImage->Height);
	if (NewData == NULL) {
		return IL_FALSE;
	}

	switch (context->impl->iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				NewData[j]   = context->impl->iCurImage->Data[i];
				NewData[j+1] = context->impl->iCurImage->Data[i+1];
				NewData[j+2] = context->impl->iCurImage->Data[i+2];
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILushort*)NewData)[j]   = ((ILushort*)context->impl->iCurImage->Data)[i];
				((ILushort*)NewData)[j+1] = ((ILushort*)context->impl->iCurImage->Data)[i+1];
				((ILushort*)NewData)[j+2] = ((ILushort*)context->impl->iCurImage->Data)[i+2];
			}
			break;

		case IL_INT:
		case IL_UNSIGNED_INT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILuint*)NewData)[j]   = ((ILuint*)context->impl->iCurImage->Data)[i];
				((ILuint*)NewData)[j+1] = ((ILuint*)context->impl->iCurImage->Data)[i+1];
				((ILuint*)NewData)[j+2] = ((ILuint*)context->impl->iCurImage->Data)[i+2];
			}
			break;

		case IL_FLOAT:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILfloat*)NewData)[j]   = ((ILfloat*)context->impl->iCurImage->Data)[i];
				((ILfloat*)NewData)[j+1] = ((ILfloat*)context->impl->iCurImage->Data)[i+1];
				((ILfloat*)NewData)[j+2] = ((ILfloat*)context->impl->iCurImage->Data)[i+2];
			}
			break;

		case IL_DOUBLE:
			for (; i < Size; i += context->impl->iCurImage->Bpp, j += NewBpp) {
				((ILdouble*)NewData)[j]   = ((ILdouble*)context->impl->iCurImage->Data)[i];
				((ILdouble*)NewData)[j+1] = ((ILdouble*)context->impl->iCurImage->Data)[i+1];
				((ILdouble*)NewData)[j+2] = ((ILdouble*)context->impl->iCurImage->Data)[i+2];
			}
			break;

		default:
			ifree(NewData);
			ilSetError(context, IL_INTERNAL_ERROR);
			return IL_FALSE;
	}


	context->impl->iCurImage->Bpp = NewBpp;
	context->impl->iCurImage->Bps = context->impl->iCurImage->Width * context->impl->iCurImage->Bpc * NewBpp;
	context->impl->iCurImage->SizeOfPlane = context->impl->iCurImage->Bps * context->impl->iCurImage->Height;
	context->impl->iCurImage->SizeOfData = context->impl->iCurImage->SizeOfPlane * context->impl->iCurImage->Depth;
	ifree(context->impl->iCurImage->Data);
	context->impl->iCurImage->Data = NewData;

	switch (context->impl->iCurImage->Format)
	{
		case IL_RGBA:
			context->impl->iCurImage->Format = IL_RGB;
			break;
		case IL_BGRA:
			context->impl->iCurImage->Format = IL_BGR;
			break;
	}

	return IL_TRUE;
}


ILboolean ilFixCur(ILcontext* context)
{
	if (ilIsEnabled(context, IL_ORIGIN_SET)) {
		if ((ILenum)ilGetInteger(context, IL_ORIGIN_MODE) != context->impl->iCurImage->Origin) {
			if (!ilFlipImage(context)) {
				return IL_FALSE;
			}
		}
	}

	if (ilIsEnabled(context, IL_TYPE_SET)) {
		if ((ILenum)ilGetInteger(context, IL_TYPE_MODE) != context->impl->iCurImage->Type) {
			if (!ilConvertImage(context, context->impl->iCurImage->Format, ilGetInteger(context, IL_TYPE_MODE))) {
				return IL_FALSE;
			}
		}
	}
	if (ilIsEnabled(context, IL_FORMAT_SET)) {
		if ((ILenum)ilGetInteger(context, IL_FORMAT_MODE) != context->impl->iCurImage->Format) {
			if (!ilConvertImage(context, ilGetInteger(context, IL_FORMAT_MODE), context->impl->iCurImage->Type)) {
				return IL_FALSE;
			}
		}
	}

	if (context->impl->iCurImage->Format == IL_COLOUR_INDEX) {
		if (ilGetBoolean(context, IL_CONV_PAL) == IL_TRUE) {
			if (!ilConvertImage(context, IL_BGR, IL_UNSIGNED_BYTE)) {
				return IL_FALSE;
			}
		}
	}
/*	Swap Colors on Big Endian !!!!!
#ifdef __BIG_ENDIAN__
	// Swap endian
	EndianSwapData(context->impl->iCurImage);
#endif 
*/
	return IL_TRUE;
}

/*
ILboolean ilFixImage()
{
	ILuint NumImages, i;

	NumImages = ilGetInteger(context, IL_NUM_IMAGES);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName(context));  // Set to parent image first.
		if (!ilActiveImage(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(context, IL_NUM_MIPMAPS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName(context));  // Set to parent image first.
		if (!ilActiveMipmap(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	NumImages = ilGetInteger(context, IL_NUM_LAYERS);
	for (i = 0; i < NumImages; i++) {
		ilBindImage(ilGetCurName(context));  // Set to parent image first.
		if (!ilActiveLayer(i+1))
			return IL_FALSE;
		if (!ilFixCur())
			return IL_FALSE;
	}

	ilBindImage(ilGetCurName(context));
	ilFixCur();

	return IL_TRUE;
}
*/

/*
This function was replaced 20050304, because the previous version
didn't fix the mipmaps of the subimages etc. This version is not
completely correct either, because the subimages of the subimages
etc. are not fixed, but at the moment no images of this type can
be loaded anyway. Thanks to Chris Lux for pointing this out.
*/

ILboolean ilFixImage(ILcontext* context)
{
	ILuint NumFaces,  f;
	ILuint NumImages, i;
	ILuint NumMipmaps,j;
	ILuint NumLayers, k;

	NumImages = ilGetInteger(context, IL_NUM_IMAGES);
	for (i = 0; i <= NumImages; i++) {
		ilBindImage(context, ilGetCurName(context));  // Set to parent image first.
		if (!ilActiveImage(context, i))
			return IL_FALSE;

		NumFaces = ilGetInteger(context, IL_NUM_FACES);
		for (f = 0; f <= NumFaces; f++) {
			ilBindImage(context, ilGetCurName(context));  // Set to parent image first.
			if (!ilActiveImage(context, i))
				return IL_FALSE;
			if (!ilActiveFace(context, f))
				return IL_FALSE;

			NumLayers = ilGetInteger(context, IL_NUM_LAYERS);
			for (k = 0; k <= NumLayers; k++) {
				ilBindImage(context, ilGetCurName(context));  // Set to parent image first.
				if (!ilActiveImage(context, i))
					return IL_FALSE;
				if (!ilActiveFace(context, f))
					return IL_FALSE;
				if (!ilActiveLayer(context, k))
					return IL_FALSE;

				NumMipmaps = ilGetInteger(context, IL_NUM_MIPMAPS);
				for (j = 0; j <= NumMipmaps; j++) {
					ilBindImage(context, ilGetCurName(context));	// Set to parent image first.
					if (!ilActiveImage(context, i))
						return IL_FALSE;
					if (!ilActiveFace(context, f))
						return IL_FALSE;
					if (!ilActiveLayer(context, k))
						return IL_FALSE;
					if (!ilActiveMipmap(context, j))
						return IL_FALSE;
					if (!ilFixCur(context))
						return IL_FALSE;
				}
			}
		}
	}
	ilBindImage(context, ilGetCurName(context));

	return IL_TRUE;
}
