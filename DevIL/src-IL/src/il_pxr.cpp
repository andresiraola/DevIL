//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/26/2002
//
// Filename: src-IL/src/il_pxr.cpp
//
// Description: Reads from a Pxrar (.pxr) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_PXR

#include "il_endian.h"
#include "il_pxr.h"

#ifdef _MSC_VER
#pragma pack(push, pxr_struct, 1)
#endif

typedef struct PIXHEAD
{
	ILushort	Signature;
	ILubyte		Reserved1[413];
	ILushort	Height;
	ILushort	Width;
	ILubyte		Reserved2[4];
	ILubyte		BppInfo;
	ILubyte		Reserved3[598];
} IL_PACKSTRUCT PIXHEAD;

#ifdef _MSC_VER
#pragma pack(pop, pxr_struct)
#endif

PxrHandler::PxrHandler(ILcontext* context) :
	context(context)
{

}

//! Reads a Pxr file
ILboolean PxrHandler::load(ILconst_string FileName)
{
	ILHANDLE	PxrFile;
	ILboolean	bPxr = IL_FALSE;

	PxrFile = context->impl->iopenr(FileName);
	if (PxrFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPxr;
	}

	bPxr = loadF(PxrFile);
	context->impl->icloser(PxrFile);

	return bPxr;
}

//! Reads an already-opened Pxr file
ILboolean PxrHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a Pxr
ILboolean PxrHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the Pxr.
ILboolean PxrHandler::loadInternal()
{
	ILushort	Width, Height;
	ILubyte		Bpp;

	Width = sizeof(PIXHEAD);

	context->impl->iseek(context, 416, IL_SEEK_SET);
	Height = GetLittleUShort(context);
	Width = GetLittleUShort(context);
	context->impl->iseek(context, 424, IL_SEEK_SET);
	Bpp = (ILubyte)context->impl->igetc(context);

	switch (Bpp)
	{
		case 0x08:
			ilTexImage(context, Width, Height, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0E:
			ilTexImage(context, Width, Height, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
			break;
		case 0x0F:
			ilTexImage(context, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			break;
		default:
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	context->impl->iseek(context, 1024, IL_SEEK_SET);
	context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData);
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return IL_TRUE;
}

#endif//IL_NO_PXR