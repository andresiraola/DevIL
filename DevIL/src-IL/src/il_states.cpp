//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_states.cpp
//
// Description: State machine
//
//
// 20040223 XIX : now has a ilPngAlphaIndex member, so we can spit out png files with a transparent index, set to -1 for none
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_states.h"
//#include <malloc.h>
#include <stdlib.h>

ILconst_string _ilVendor		= IL_TEXT("Abysmal Software");
ILconst_string _ilVersion		= IL_TEXT("Developer's Image Library (DevIL) 1.8.0");


char* _ilLoadExt		= (char*)"" IL_BLP_EXT IL_BMP_EXT IL_CUT_EXT IL_DCX_EXT IL_DDS_EXT
									IL_DCM_EXT IL_DPX_EXT IL_EXR_EXT IL_FITS_EXT IL_FTX_EXT
									IL_GIF_EXT IL_HDR_EXT IL_ICNS_EXT IL_ICO_EXT IL_IFF_EXT
									IL_IWI_EXT IL_JPG_EXT IL_JP2_EXT IL_LIF_EXT IL_MDL_EXT
									IL_MNG_EXT IL_MP3_EXT IL_PCD_EXT IL_PCX_EXT IL_PIC_EXT
									IL_PIX_EXT IL_PNG_EXT IL_PNM_EXT IL_PSD_EXT IL_PSP_EXT
									IL_PXR_EXT IL_RAW_EXT IL_ROT_EXT IL_SGI_EXT IL_SUN_EXT
									IL_TEX_EXT IL_TGA_EXT IL_TIF_EXT IL_TPL_EXT IL_UTX_EXT
									IL_VTF_EXT IL_WAL_EXT IL_WDP_EXT IL_XPM_EXT;

char* _ilSaveExt		= (char*)"" IL_BMP_EXT IL_CHEAD_EXT IL_DDS_EXT IL_EXR_EXT
									IL_HDR_EXT IL_JP2_EXT IL_JPG_EXT IL_PCX_EXT
									IL_PNG_EXT IL_PNM_EXT IL_PSD_EXT IL_RAW_EXT
									IL_SGI_EXT IL_TGA_EXT IL_TIF_EXT IL_VTF_EXT
									IL_WBMP_EXT;


//! Set all states to their defaults.
void ilDefaultStates(ILcontext* context)
{
	context->impl->ilStates[context->impl->ilCurrentPos].ilOriginSet = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilOriginMode = IL_ORIGIN_LOWER_LEFT;
	context->impl->ilStates[context->impl->ilCurrentPos].ilFormatSet = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilFormatMode = IL_BGRA;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTypeSet = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTypeMode = IL_UNSIGNED_BYTE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilOverWriteFiles = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilAutoConvPal = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilDefaultOnFail = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilUseKeyColour = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilBlitBlend = IL_TRUE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilCompression = IL_COMPRESS_ZLIB;
	context->impl->ilStates[context->impl->ilCurrentPos].ilInterlace = IL_FALSE;

	context->impl->ilStates[context->impl->ilCurrentPos].ilTgaCreateStamp = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilJpgQuality = 99;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPngInterlace = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTgaRle = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilBmpRle = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilSgiRle = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilJpgFormat = IL_JFIF;
	context->impl->ilStates[context->impl->ilCurrentPos].ilJpgProgressive = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilDxtcFormat = IL_DXT1;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPcdPicNum = 2;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPngAlphaIndex = -1;
	context->impl->ilStates[context->impl->ilCurrentPos].ilVtfCompression = IL_DXT_NO_COMP;

	context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription = NULL;

	//2003-09-01: added tiff strings
	context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName = NULL;
	context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader = NULL;

	context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMode = IL_WU_QUANT;
	context->impl->ilStates[context->impl->ilCurrentPos].ilNeuSample = 15;
	context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMaxIndexs = 256;

	context->impl->ilStates[context->impl->ilCurrentPos].ilKeepDxtcData = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilUseNVidiaDXT = IL_FALSE;
	context->impl->ilStates[context->impl->ilCurrentPos].ilUseSquishDXT = IL_FALSE;

	context->impl->ilHints.MemVsSpeedHint = IL_FASTEST;
	context->impl->ilHints.CompressHint = IL_USE_COMPRESSION;

	while (ilGetError(context) != IL_NO_ERROR);

	return;
}


//! Returns a constant string detailing aspects about this library.
ILconst_string ILAPIENTRY ilGetString(ILcontext* context, ILenum StringName)
{
	switch (StringName)
	{
		case IL_VENDOR:
			return (ILconst_string)_ilVendor;
		case IL_VERSION_NUM: //changed 2003-08-30: IL_VERSION changes									//switch define ;-)
			return (ILconst_string)_ilVersion;
		case IL_LOAD_EXT:
			return (ILconst_string)_ilLoadExt;
		case IL_SAVE_EXT:
			return (ILconst_string)_ilSaveExt;
		case IL_TGA_ID_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId;
		case IL_TGA_AUTHNAME_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName;
		case IL_TGA_AUTHCOMMENT_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment;
		case IL_PNG_AUTHNAME_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName;
		case IL_PNG_TITLE_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle;
		case IL_PNG_DESCRIPTION_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription;
		//2003-08-31: added tif strings
		case IL_TIF_DESCRIPTION_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription;
		case IL_TIF_HOSTCOMPUTER_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer;
		case IL_TIF_DOCUMENTNAME_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName;
		case IL_TIF_AUTHNAME_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName;
		case IL_CHEAD_HEADER_STRING:
			return (ILconst_string)context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader;
		default:
			ilSetError(context, IL_INVALID_ENUM);
			break;
	}
	return NULL;
}


// Clips a string to a certain length and returns a new string.
char *iClipString(ILcontext* context, char *String, ILuint MaxLen)
{
	char	*Clipped;
	ILuint	Length;

	if (String == NULL)
		return NULL;

	Length = ilCharStrLen(String);  //ilStrLen(String);

	Clipped = (char*)ialloc(context, (MaxLen + 1) * sizeof(char) /*sizeof(ILchar)*/);  // Terminating NULL makes it +1.
	if (Clipped == NULL) {
		return NULL;
	}

	memcpy(Clipped, String, MaxLen * sizeof(char) /*sizeof(ILchar)*/);
	Clipped[Length] = 0;

	return Clipped;
}


// Returns format-specific strings, truncated to MaxLen (not counting the terminating NULL).
char *iGetString(ILcontext* context, ILenum StringName)
{
	switch (StringName)
	{
		case IL_TGA_ID_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId, 254);
		case IL_TGA_AUTHNAME_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName, 40);
		case IL_TGA_AUTHCOMMENT_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment, 80);
		case IL_PNG_AUTHNAME_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName, 255);
		case IL_PNG_TITLE_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle, 255);
		case IL_PNG_DESCRIPTION_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription, 255);

		//changed 2003-08-31...here was a serious copy and paste bug ;-)
		case IL_TIF_DESCRIPTION_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription, 255);
		case IL_TIF_HOSTCOMPUTER_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer, 255);
		case IL_TIF_DOCUMENTNAME_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName, 255);
		case IL_TIF_AUTHNAME_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName, 255);
		case IL_CHEAD_HEADER_STRING:
			return iClipString(context, context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader, 32);
		default:
			ilSetError(context, IL_INVALID_ENUM);
	}
	return NULL;
}


//! Enables a mode
ILboolean ILAPIENTRY ilEnable(ILcontext* context, ILenum Mode)
{
	return ilAble(context, Mode, IL_TRUE);
}


//! Disables a mode
ILboolean ILAPIENTRY ilDisable(ILcontext* context, ILenum Mode)
{
	return ilAble(context, Mode, IL_FALSE);
}


// Internal function that sets the Mode equal to Flag
ILboolean ilAble(ILcontext* context, ILenum Mode, ILboolean Flag)
{
	switch (Mode)
	{
		case IL_ORIGIN_SET:
			context->impl->ilStates[context->impl->ilCurrentPos].ilOriginSet = Flag;
			break;
		case IL_FORMAT_SET:
			context->impl->ilStates[context->impl->ilCurrentPos].ilFormatSet = Flag;
			break;
		case IL_TYPE_SET:
			context->impl->ilStates[context->impl->ilCurrentPos].ilTypeSet = Flag;
			break;
		case IL_FILE_OVERWRITE:
			context->impl->ilStates[context->impl->ilCurrentPos].ilOverWriteFiles = Flag;
			break;
		case IL_CONV_PAL:
			context->impl->ilStates[context->impl->ilCurrentPos].ilAutoConvPal = Flag;
			break;
		case IL_DEFAULT_ON_FAIL:
			context->impl->ilStates[context->impl->ilCurrentPos].ilDefaultOnFail = Flag;
			break;
		case IL_USE_KEY_COLOUR:
			context->impl->ilStates[context->impl->ilCurrentPos].ilUseKeyColour = Flag;
			break;
		case IL_BLIT_BLEND:
			context->impl->ilStates[context->impl->ilCurrentPos].ilBlitBlend = Flag;
			break;
		case IL_SAVE_INTERLACED:
			context->impl->ilStates[context->impl->ilCurrentPos].ilInterlace = Flag;
			break;
		case IL_JPG_PROGRESSIVE:
			context->impl->ilStates[context->impl->ilCurrentPos].ilJpgProgressive = Flag;
			break;
		case IL_NVIDIA_COMPRESS:
			context->impl->ilStates[context->impl->ilCurrentPos].ilUseNVidiaDXT = Flag;
			break;
		case IL_SQUISH_COMPRESS:
			context->impl->ilStates[context->impl->ilCurrentPos].ilUseSquishDXT = Flag;
			break;

		default:
			ilSetError(context, IL_INVALID_ENUM);
			return IL_FALSE;
	}

	return IL_TRUE;
}


//! Checks whether the mode is enabled.
ILboolean ILAPIENTRY ilIsEnabled(ILcontext* context, ILenum Mode)
{
	switch (Mode)
	{
		case IL_ORIGIN_SET:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilOriginSet;
		case IL_FORMAT_SET:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilFormatSet;
		case IL_TYPE_SET:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilTypeSet;
		case IL_FILE_OVERWRITE:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilOverWriteFiles;
		case IL_CONV_PAL:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilAutoConvPal;
		case IL_DEFAULT_ON_FAIL:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilDefaultOnFail;
		case IL_USE_KEY_COLOUR:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilUseKeyColour;
		case IL_BLIT_BLEND:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilBlitBlend;
		case IL_SAVE_INTERLACED:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilInterlace;
		case IL_JPG_PROGRESSIVE:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilJpgProgressive;
		case IL_NVIDIA_COMPRESS:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilUseNVidiaDXT;
		case IL_SQUISH_COMPRESS:
			return context->impl->ilStates[context->impl->ilCurrentPos].ilUseSquishDXT;

		default:
			ilSetError(context, IL_INVALID_ENUM);
	}

	return IL_FALSE;
}


//! Checks whether the mode is disabled.
ILboolean ILAPIENTRY ilIsDisabled(ILcontext* context, ILenum Mode)
{
	return !ilIsEnabled(context, Mode);
}


//! Sets Param equal to the current value of the Mode
void ILAPIENTRY ilGetBooleanv(ILcontext* context, ILenum Mode, ILboolean *Param)
{
	if (Param == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return;
	}

	*Param = ilGetInteger(context, Mode);

	return;
}


//! Returns the current value of the Mode
ILboolean ILAPIENTRY ilGetBoolean(ILcontext* context, ILenum Mode)
{
	ILboolean Temp;
	Temp = IL_FALSE;
	ilGetBooleanv(context, Mode, &Temp);
	return Temp;
}


ILimage *iGetBaseImage(ILcontext* context);

//! Internal function to figure out where we are in an image chain.
//@TODO: This may get much more complex with mipmaps under faces, etc.
ILuint iGetActiveNum(ILcontext* context, ILenum Type)
{
	ILimage *BaseImage;
	ILuint Num = 0;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return 0;
	}

	BaseImage = iGetBaseImage(context);
	if (BaseImage == context->impl->iCurImage)
		return 0;

	switch (Type)
	{
		case IL_ACTIVE_IMAGE:
			BaseImage = BaseImage->Next;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == context->impl->iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Next));
			break;
		case IL_ACTIVE_MIPMAP:
			BaseImage = BaseImage->Mipmaps;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == context->impl->iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Mipmaps));
			break;
		case IL_ACTIVE_LAYER:
			BaseImage = BaseImage->Layers;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == context->impl->iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Layers));
			break;
		case IL_ACTIVE_FACE:
			BaseImage = BaseImage->Faces;
			do {
				if (BaseImage == NULL)
					return 0;
				Num++;
				if (BaseImage == context->impl->iCurImage)
					return Num;
			} while ((BaseImage = BaseImage->Faces));
			break;
	}

	//@TODO: Any error needed here?

	return 0;
}


//! Sets Param equal to the current value of the Mode
void ILAPIENTRY ilGetIntegerv(ILcontext* context, ILenum Mode, ILint *Param)
{
	if (Param == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return;
	}

	*Param = 0;

	switch (Mode)
	{
		// Integer values
		case IL_COMPRESS_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilCompression;
			break;
		case IL_CUR_IMAGE:
			if (context->impl->iCurImage == NULL) {
				ilSetError(context, IL_ILLEGAL_OPERATION);
				break;
			}
			*Param = ilGetCurName(context);
			break;
		case IL_FORMAT_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilFormatMode;
			break;
		case IL_INTERLACE_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilInterlace;
			break;
		case IL_KEEP_DXTC_DATA:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilKeepDxtcData;
			break;
		case IL_ORIGIN_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilOriginMode;
			break;
		case IL_MAX_QUANT_INDICES:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMaxIndexs;
			break;
		case IL_NEU_QUANT_SAMPLE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilNeuSample;
			break;
		case IL_QUANTIZATION_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMode;
			break;
		case IL_TYPE_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilTypeMode;
			break;
		case IL_VERSION_NUM:
			*Param = IL_VERSION;
			break;

		// Image specific values
		case IL_ACTIVE_IMAGE:
		case IL_ACTIVE_MIPMAP:
		case IL_ACTIVE_LAYER:
			*Param = iGetActiveNum(context, Mode);
			break;

		// Format-specific values
		case IL_BMP_RLE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilBmpRle;
			break;
		case IL_DXTC_FORMAT:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilDxtcFormat;
			break;
		case IL_JPG_QUALITY:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilJpgQuality;
			break;
		case IL_JPG_SAVE_FORMAT:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilJpgFormat;
			break;
		case IL_PCD_PICNUM:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilPcdPicNum;
			break;
		case IL_PNG_ALPHA_INDEX:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilPngAlphaIndex;
			break;
		case IL_PNG_INTERLACE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilPngInterlace;
			break;
		case IL_SGI_RLE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilSgiRle;
			break;
		case IL_TGA_CREATE_STAMP:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilTgaCreateStamp;
			break;
		case IL_TGA_RLE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilTgaRle;
			break;
		case IL_VTF_COMP:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilVtfCompression;
			break;

		// Boolean values
		case IL_CONV_PAL:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilAutoConvPal;
			break;
		case IL_DEFAULT_ON_FAIL:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilDefaultOnFail;
			break;
		case IL_FILE_MODE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilOverWriteFiles;
			break;
		case IL_FORMAT_SET:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilFormatSet;
			break;
		case IL_ORIGIN_SET:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilOriginSet;
			break;
		case IL_TYPE_SET:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilTypeSet;
			break;
		case IL_USE_KEY_COLOUR:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilUseKeyColour;
			break;
		case IL_BLIT_BLEND:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilBlitBlend;
			break;
		case IL_JPG_PROGRESSIVE:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilJpgProgressive;
			break;
		case IL_NVIDIA_COMPRESS:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilUseNVidiaDXT;
			break;
		case IL_SQUISH_COMPRESS:
			*Param = context->impl->ilStates[context->impl->ilCurrentPos].ilUseSquishDXT;
			break;

		default:
            iGetIntegervImage(context, context->impl->iCurImage, Mode, Param);
	}

	return;
}

//@TODO rename to ilGetImageIntegerv for 1.6.9 and make it public
//! Sets Param equal to the current value of the Mode
void ILAPIENTRY iGetIntegervImage(ILcontext* context, ILimage *Image, ILenum Mode, ILint *Param)
{
    ILimage *SubImage;
    if (Image == NULL) {
        ilSetError(context, IL_ILLEGAL_OPERATION);
        return;
    }
    if (Param == NULL) {
        ilSetError(context, IL_INVALID_PARAM);
        return;
    }
    *Param = 0;

    switch (Mode)
    {
        case IL_DXTC_DATA_FORMAT:
            if (Image->DxtcData == NULL || Image->DxtcSize == 0) {
                 *Param = IL_DXT_NO_COMP;
                 break;
            }
            *Param = Image->DxtcFormat;
            break;
            ////
        case IL_IMAGE_BITS_PER_PIXEL:
            //changed 20040610 to channel count (Bpp) times Bytes per channel
            *Param = (Image->Bpp << 3)*Image->Bpc;
            break;
        case IL_IMAGE_BYTES_PER_PIXEL:
            //changed 20040610 to channel count (Bpp) times Bytes per channel
            *Param = Image->Bpp*Image->Bpc;
            break;
        case IL_IMAGE_BPC:
            *Param = Image->Bpc;
            break;
        case IL_IMAGE_CHANNELS:
            *Param = Image->Bpp;
            break;
        case IL_IMAGE_CUBEFLAGS:
            *Param = Image->CubeFlags;
            break;
        case IL_IMAGE_DEPTH:
            *Param = Image->Depth;
            break;
        case IL_IMAGE_DURATION:
            *Param = Image->Duration;
            break;
        case IL_IMAGE_FORMAT:
            *Param = Image->Format;
            break;
        case IL_IMAGE_HEIGHT:
            *Param = Image->Height;
            break;
        case IL_IMAGE_SIZE_OF_DATA:
            *Param = Image->SizeOfData;

            break;
        case IL_IMAGE_OFFX:
            *Param = Image->OffX;
            break;
        case IL_IMAGE_OFFY:
            *Param = Image->OffY;
            break;
        case IL_IMAGE_ORIGIN:
            *Param = Image->Origin;
            break;
        case IL_IMAGE_PLANESIZE:
            *Param = Image->SizeOfPlane;
            break;
        case IL_IMAGE_TYPE:
            *Param = Image->Type;
            break;
        case IL_IMAGE_WIDTH:
            *Param = Image->Width;
            break;
        case IL_NUM_FACES:
            for (SubImage = Image->Faces; SubImage; SubImage = SubImage->Faces)
                (*Param)++;
            break;
        case IL_NUM_IMAGES:
            for (SubImage = Image->Next; SubImage; SubImage = SubImage->Next)
                (*Param)++;
            break;
        case IL_NUM_LAYERS:
            for (SubImage = Image->Layers; SubImage; SubImage = SubImage->Layers)
                (*Param)++;
            break;
        case IL_NUM_MIPMAPS:
			for (SubImage = Image->Mipmaps; SubImage; SubImage = SubImage->Mipmaps)
                (*Param)++;
            break;

        case IL_PALETTE_TYPE:
             *Param = Image->Pal.PalType;
             break;
        case IL_PALETTE_BPP:
             *Param = ilGetBppPal(Image->Pal.PalType);
             break;
        case IL_PALETTE_NUM_COLS:
             if (!Image->Pal.Palette || !Image->Pal.PalSize || Image->Pal.PalType == IL_PAL_NONE)
                  *Param = 0;
             else
                  *Param = Image->Pal.PalSize / ilGetBppPal(Image->Pal.PalType);
             break;
        case IL_PALETTE_BASE_TYPE:
             switch (Image->Pal.PalType)
             {
                  case IL_PAL_RGB24:
                      *Param = IL_RGB;
                  case IL_PAL_RGB32:
                      *Param = IL_RGBA; // Not sure
                  case IL_PAL_RGBA32:
                      *Param = IL_RGBA;
                  case IL_PAL_BGR24:
                      *Param = IL_BGR;
                  case IL_PAL_BGR32:
                      *Param = IL_BGRA; // Not sure
                  case IL_PAL_BGRA32:
                      *Param = IL_BGRA;
             }
             break;
        default:
             ilSetError(context, IL_INVALID_ENUM);
    }
}



//! Returns the current value of the Mode
ILint ILAPIENTRY ilGetInteger(ILcontext* context, ILenum Mode)
{
	ILint Temp;
	Temp = 0;
	ilGetIntegerv(context, Mode, &Temp);
	return Temp;
}


//! Sets the default origin to be used.
ILboolean ILAPIENTRY ilOriginFunc(ILcontext* context, ILenum Mode)
{
	switch (Mode)
	{
		case IL_ORIGIN_LOWER_LEFT:
		case IL_ORIGIN_UPPER_LEFT:
			context->impl->ilStates[context->impl->ilCurrentPos].ilOriginMode = Mode;
			break;
		default:
			ilSetError(context, IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Sets the default format to be used.
ILboolean ILAPIENTRY ilFormatFunc(ILcontext* context, ILenum Mode)
{
	switch (Mode)
	{
		//case IL_COLOUR_INDEX:
		case IL_RGB:
		case IL_RGBA:
		case IL_BGR:
		case IL_BGRA:
		case IL_LUMINANCE:
		case IL_LUMINANCE_ALPHA:
			context->impl->ilStates[context->impl->ilCurrentPos].ilFormatMode = Mode;
			break;
		default:
			ilSetError(context, IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Sets the default type to be used.
ILboolean ILAPIENTRY ilTypeFunc(ILcontext* context, ILenum Mode)
{
	switch (Mode)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
		case IL_DOUBLE:
			context->impl->ilStates[context->impl->ilCurrentPos].ilTypeMode = Mode;
			break;
		default:
			ilSetError(context, IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


ILboolean ILAPIENTRY ilCompressFunc(ILcontext* context, ILenum Mode)
{
	switch (Mode)
	{
		case IL_COMPRESS_NONE:
		case IL_COMPRESS_RLE:
		//case IL_COMPRESS_LZO:
		case IL_COMPRESS_ZLIB:
			context->impl->ilStates[context->impl->ilCurrentPos].ilCompression = Mode;
			break;
		default:
			ilSetError(context, IL_INVALID_PARAM);
			return IL_FALSE;
	}
	return IL_TRUE;
}


//! Pushes the states indicated by Bits onto the state stack
void ILAPIENTRY ilPushAttrib(ILcontext* context, ILuint Bits)
{
	// Should we check here to see if ilCurrentPos is negative?

	if (context->impl->ilCurrentPos >= IL_ATTRIB_STACK_MAX - 1) {
		context->impl->ilCurrentPos = IL_ATTRIB_STACK_MAX - 1;
		ilSetError(context, IL_STACK_OVERFLOW);
		return;
	}

	context->impl->ilCurrentPos++;

	//	memcpy(&context->impl->ilStates[context->impl->ilCurrentPos], &context->impl->ilStates[context->impl->ilCurrentPos - 1], sizeof(IL_STATES));

	ilDefaultStates(context);

	if (Bits & IL_ORIGIN_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilOriginMode = context->impl->ilStates[context->impl->ilCurrentPos-1].ilOriginMode;
		context->impl->ilStates[context->impl->ilCurrentPos].ilOriginSet  = context->impl->ilStates[context->impl->ilCurrentPos-1].ilOriginSet;
	}
	if (Bits & IL_FORMAT_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilFormatMode = context->impl->ilStates[context->impl->ilCurrentPos-1].ilFormatMode;
		context->impl->ilStates[context->impl->ilCurrentPos].ilFormatSet  = context->impl->ilStates[context->impl->ilCurrentPos-1].ilFormatSet;
	}
	if (Bits & IL_TYPE_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilTypeMode = context->impl->ilStates[context->impl->ilCurrentPos-1].ilTypeMode;
		context->impl->ilStates[context->impl->ilCurrentPos].ilTypeSet  = context->impl->ilStates[context->impl->ilCurrentPos-1].ilTypeSet;
	}
	if (Bits & IL_FILE_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilOverWriteFiles = context->impl->ilStates[context->impl->ilCurrentPos-1].ilOverWriteFiles;
	}
	if (Bits & IL_PAL_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilAutoConvPal = context->impl->ilStates[context->impl->ilCurrentPos-1].ilAutoConvPal;
	}
	if (Bits & IL_LOADFAIL_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilDefaultOnFail = context->impl->ilStates[context->impl->ilCurrentPos-1].ilDefaultOnFail;
	}
	if (Bits & IL_COMPRESS_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilCompression = context->impl->ilStates[context->impl->ilCurrentPos-1].ilCompression;
	}
	if (Bits & IL_FORMAT_SPECIFIC_BIT) {
		context->impl->ilStates[context->impl->ilCurrentPos].ilTgaCreateStamp = context->impl->ilStates[context->impl->ilCurrentPos-1].ilTgaCreateStamp;
		context->impl->ilStates[context->impl->ilCurrentPos].ilJpgQuality = context->impl->ilStates[context->impl->ilCurrentPos-1].ilJpgQuality;
		context->impl->ilStates[context->impl->ilCurrentPos].ilPngInterlace = context->impl->ilStates[context->impl->ilCurrentPos-1].ilPngInterlace;
		context->impl->ilStates[context->impl->ilCurrentPos].ilTgaRle = context->impl->ilStates[context->impl->ilCurrentPos-1].ilTgaRle;
		context->impl->ilStates[context->impl->ilCurrentPos].ilBmpRle = context->impl->ilStates[context->impl->ilCurrentPos-1].ilBmpRle;
		context->impl->ilStates[context->impl->ilCurrentPos].ilSgiRle = context->impl->ilStates[context->impl->ilCurrentPos-1].ilSgiRle;
		context->impl->ilStates[context->impl->ilCurrentPos].ilJpgFormat = context->impl->ilStates[context->impl->ilCurrentPos-1].ilJpgFormat;
		context->impl->ilStates[context->impl->ilCurrentPos].ilDxtcFormat = context->impl->ilStates[context->impl->ilCurrentPos-1].ilDxtcFormat;
		context->impl->ilStates[context->impl->ilCurrentPos].ilPcdPicNum = context->impl->ilStates[context->impl->ilCurrentPos-1].ilPcdPicNum;

		context->impl->ilStates[context->impl->ilCurrentPos].ilPngAlphaIndex = context->impl->ilStates[context->impl->ilCurrentPos-1].ilPngAlphaIndex;

		// Strings
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription);

		//2003-09-01: added tif strings
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName);
		if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName);

		if (context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader)
			ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader);

		context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTgaId);
		context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTgaAuthName);
		context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTgaAuthComment);
		context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilPngAuthName);
		context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilPngTitle);
		context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilPngDescription);

		//2003-09-01: added tif strings
		context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTifDescription);
		context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTifHostComputer);
		context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTifDocumentName);
		context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilTifAuthName);

		context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader = strdup(context->impl->ilStates[context->impl->ilCurrentPos-1].ilCHeader);
	}

	return;
}


// @TODO:  Find out how this affects strings!!!

//! Pops the last entry off the state stack into the current states
void ILAPIENTRY ilPopAttrib(ILcontext* context)
{
	if (context->impl->ilCurrentPos <= 0) {
		context->impl->ilCurrentPos = 0;
		ilSetError(context, IL_STACK_UNDERFLOW);
		return;
	}

	// Should we check here to see if ilCurrentPos is too large?
	context->impl->ilCurrentPos--;

	return;
}


//! Specifies implementation-dependent performance hints
void ILAPIENTRY ilHint(ILcontext* context, ILenum Target, ILenum Mode)
{
	switch (Target)
	{
		case IL_MEM_SPEED_HINT:
			switch (Mode)
			{
				case IL_FASTEST:
					context->impl->ilHints.MemVsSpeedHint = Mode;
					break;
				case IL_LESS_MEM:
					context->impl->ilHints.MemVsSpeedHint = Mode;
					break;
				case IL_DONT_CARE:
					context->impl->ilHints.MemVsSpeedHint = IL_FASTEST;
					break;
				default:
					ilSetError(context, IL_INVALID_ENUM);
					return;
			}
			break;

		case IL_COMPRESSION_HINT:
			switch (Mode)
			{
				case IL_USE_COMPRESSION:
					context->impl->ilHints.CompressHint = Mode;
					break;
				case IL_NO_COMPRESSION:
					context->impl->ilHints.CompressHint = Mode;
					break;
				case IL_DONT_CARE:
					context->impl->ilHints.CompressHint = IL_NO_COMPRESSION;
					break;
				default:
					ilSetError(context, IL_INVALID_ENUM);
					return;
			}
			break;

			
		default:
			ilSetError(context, IL_INVALID_ENUM);
			return;
	}

	return;
}


ILenum iGetHint(ILcontext* context, ILenum Target)
{
	switch (Target)
	{
		case IL_MEM_SPEED_HINT:
			return context->impl->ilHints.MemVsSpeedHint;
		case IL_COMPRESSION_HINT:
			return context->impl->ilHints.CompressHint;
		default:
			ilSetError(context, IL_INTERNAL_ERROR);
			return 0;
	}
}


void ILAPIENTRY ilSetString(ILcontext* context, ILenum Mode, const char *String)
{
	if (String == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return;
	}

	switch (Mode)
	{
		case IL_TGA_ID_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTgaId = strdup(String);
			break;
		case IL_TGA_AUTHNAME_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthName = strdup(String);
			break;
		case IL_TGA_AUTHCOMMENT_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTgaAuthComment = strdup(String);
			break;
		case IL_PNG_AUTHNAME_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName);
			context->impl->ilStates[context->impl->ilCurrentPos].ilPngAuthName = strdup(String);
			break;
		case IL_PNG_TITLE_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle);
			context->impl->ilStates[context->impl->ilCurrentPos].ilPngTitle = strdup(String);
			break;
		case IL_PNG_DESCRIPTION_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription);
			context->impl->ilStates[context->impl->ilCurrentPos].ilPngDescription = strdup(String);
			break;

		//2003-09-01: added tif strings
		case IL_TIF_DESCRIPTION_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTifDescription = strdup(String);
			break;
		case IL_TIF_HOSTCOMPUTER_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTifHostComputer = strdup(String);
			break;
		case IL_TIF_DOCUMENTNAME_STRING:
						if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTifDocumentName = strdup(String);
			break;
		case IL_TIF_AUTHNAME_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName);
			context->impl->ilStates[context->impl->ilCurrentPos].ilTifAuthName = strdup(String);
			break;

		case IL_CHEAD_HEADER_STRING:
			if (context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader)
				ifree(context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader);
			context->impl->ilStates[context->impl->ilCurrentPos].ilCHeader = strdup(String);
			break;

		default:
			ilSetError(context, IL_INVALID_ENUM);
	}

	return;
}


void ILAPIENTRY ilSetInteger(ILcontext* context, ILenum Mode, ILint Param)
{
	switch (Mode)
	{
		// Integer values
		case IL_FORMAT_MODE:
			ilFormatFunc(context, Param);
			return;
		case IL_KEEP_DXTC_DATA:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilKeepDxtcData = Param;
				return;
			}
			break;
		case IL_MAX_QUANT_INDICES:
			if (Param >= 2 && Param <= 256) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMaxIndexs = Param;
				return;
			}
			break;
		case IL_NEU_QUANT_SAMPLE:
			if (Param >= 1 && Param <= 30) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilNeuSample = Param;
				return;
			}
			break;
		case IL_ORIGIN_MODE:
			ilOriginFunc(context, Param);
			return;
		case IL_QUANTIZATION_MODE:
			if (Param == IL_WU_QUANT || Param == IL_NEU_QUANT) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilQuantMode = Param;
				return;
			}
			break;
		case IL_TYPE_MODE:
			ilTypeFunc(context, Param);
			return;

		// Image specific values
		case IL_IMAGE_DURATION:
			if (context->impl->iCurImage == NULL) {
				ilSetError(context, IL_ILLEGAL_OPERATION);
				break;
			}
			context->impl->iCurImage->Duration = Param;
			return;
		case IL_IMAGE_OFFX:
			if (context->impl->iCurImage == NULL) {
				ilSetError(context, IL_ILLEGAL_OPERATION);
				break;
			}
			context->impl->iCurImage->OffX = Param;
			return;
		case IL_IMAGE_OFFY:
			if (context->impl->iCurImage == NULL) {
				ilSetError(context, IL_ILLEGAL_OPERATION);
				break;
			}
			context->impl->iCurImage->OffY = Param;
			return;
		case IL_IMAGE_CUBEFLAGS:
			if (context->impl->iCurImage == NULL) {
				ilSetError(context, IL_ILLEGAL_OPERATION);
				break;
			}
			context->impl->iCurImage->CubeFlags = Param;
			break;
 
		// Format specific values
		case IL_BMP_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilBmpRle = Param;
				return;
			}
			break;
		case IL_DXTC_FORMAT:
			if (Param >= IL_DXT1 || Param <= IL_DXT5 || Param == IL_DXT1A) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilDxtcFormat = Param;
				return;
			}
			break;
		case IL_JPG_SAVE_FORMAT:
			if (Param == IL_JFIF || Param == IL_EXIF) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilJpgFormat = Param;
				return;
			}
			break;
		case IL_JPG_QUALITY:
			if (Param >= 0 && Param <= 99) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilJpgQuality = Param;
				return;
			}
			break;
		case IL_PNG_INTERLACE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilPngInterlace = Param;
				return;
			}
			break;
		case IL_PCD_PICNUM:
			if (Param >= 0 || Param <= 2) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilPcdPicNum = Param;
				return;
			}
			break;
		case IL_PNG_ALPHA_INDEX:
			if (Param >= -1 || Param <= 255) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilPngAlphaIndex=Param;
				return;
			}
			break;
		case IL_SGI_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilSgiRle = Param;
				return;
			}
			break;
		case IL_TGA_CREATE_STAMP:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilTgaCreateStamp = Param;
				return;
			}
			break;
		case IL_TGA_RLE:
			if (Param == IL_FALSE || Param == IL_TRUE) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilTgaRle = Param;
				return;
			}
			break;
		case IL_VTF_COMP:
			if (Param == IL_DXT1 || Param == IL_DXT5 || Param == IL_DXT3 || Param == IL_DXT1A || Param == IL_DXT_NO_COMP) {
				context->impl->ilStates[context->impl->ilCurrentPos].ilVtfCompression = Param;
				return;
			}
			break;

		default:
			ilSetError(context, IL_INVALID_ENUM);
			return;
	}

	ilSetError(context, IL_INVALID_PARAM);  // Parameter not in valid bounds.
	return;
}



ILint iGetInt(ILcontext* context, ILenum Mode)
{
	//like ilGetInteger(), but sets another error on failure

	//call ilGetIntegerv() for more robust code
	ILenum err;
	ILint r = -1;

	ilGetIntegerv(context, Mode, &r);

	//check if an error occured, set another error
	err = ilGetError(context);
	if (r == -1 && err == IL_INVALID_ENUM)
		ilSetError(context, IL_INTERNAL_ERROR);
	else
		ilSetError(context, err); //restore error

	return r;
}
