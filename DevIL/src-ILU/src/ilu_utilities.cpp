//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001
//
// Filename: src-ILU/src/ilu_utilities.cpp
//
// Description: Utility functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"


void ILAPIENTRY iluDeleteImage(ILcontext* context, ILuint Id)
{
	ilDeleteImages(context, 1, &Id);
	return;
}


ILuint ILAPIENTRY iluGenImage(ILcontext* context)
{
	ILuint Id;
	ilGenImages(context, 1, &Id);
	ilBindImage(context, Id);
	return Id;
}


//! Retrieves information about the current bound image.
void ILAPIENTRY iluGetImageInfo(ILcontext* context, ILinfo *Info)
{
	iluCurImage = ilGetCurImage(context);
	if (iluCurImage == NULL || Info == NULL) {
		ilSetError(context, ILU_ILLEGAL_OPERATION);
		return;
	}

	Info->Id			= ilGetCurName(context);
	Info->Data			= ilGetData(context);
	Info->Width			= iluCurImage->Width;
	Info->Height		= iluCurImage->Height;
	Info->Depth			= iluCurImage->Depth;
	Info->Bpp			= iluCurImage->Bpp;
	Info->SizeOfData	= iluCurImage->SizeOfData;
	Info->Format		= iluCurImage->Format;
	Info->Type			= iluCurImage->Type;
	Info->Origin		= iluCurImage->Origin;
	Info->Palette		= iluCurImage->Pal.Palette;
	Info->PalType		= iluCurImage->Pal.PalType;
	Info->PalSize		= iluCurImage->Pal.PalSize;
	iGetIntegervImage(context, iluCurImage, IL_NUM_IMAGES,             
	                        (ILint*)&Info->NumNext);
	iGetIntegervImage(context, iluCurImage, IL_NUM_MIPMAPS, 
	                        (ILint*)&Info->NumMips);
	iGetIntegervImage(context, iluCurImage, IL_NUM_LAYERS, 
	                        (ILint*)&Info->NumLayers);
	
	return;
}
