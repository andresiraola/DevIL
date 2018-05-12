//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_raw.cpp
//
// Description: "Raw" file functions
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_RAW

#include "il_raw.h"

RawHandler::RawHandler(ILcontext* context) :
	context(context)
{

}

//! Reads a raw file
ILboolean RawHandler::load(ILconst_string FileName)
{
	ILHANDLE	RawFile;
	ILboolean	bRaw = IL_FALSE;

	// No need to check for raw
	/*if (!iCheckExtension(FileName, "raw")) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bRaw;
	}*/

	RawFile = context->impl->iopenr(FileName);
	if (RawFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bRaw;
	}

	bRaw = loadF(RawFile);
	context->impl->icloser(RawFile);

	return bRaw;
}

//! Reads an already-opened raw file
ILboolean RawHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a raw memory "lump"
ILboolean RawHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function to load a raw image
ILboolean RawHandler::loadInternal()
{
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}


	context->impl->iCurImage->Width = GetLittleUInt(context);

	context->impl->iCurImage->Height = GetLittleUInt(context);

	context->impl->iCurImage->Depth = GetLittleUInt(context);

	context->impl->iCurImage->Bpp = (ILubyte)context->impl->igetc(context);

	if (context->impl->iread(context, &context->impl->iCurImage->Bpc, 1, 1) != 1)
		return IL_FALSE;

	if (!ilTexImage(context, context->impl->iCurImage->Width, context->impl->iCurImage->Height, context->impl->iCurImage->Depth, context->impl->iCurImage->Bpp, 0, ilGetTypeBpc(context->impl->iCurImage->Bpc), NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	// Tries to read the correct amount of data
	if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) < context->impl->iCurImage->SizeOfData)
		return IL_FALSE;

	if (ilIsEnabled(context, IL_ORIGIN_SET)) {
		context->impl->iCurImage->Origin = ilGetInteger(context, IL_ORIGIN_MODE);
	}
	else {
		context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	}

	if (context->impl->iCurImage->Bpp == 1)
		context->impl->iCurImage->Format = IL_LUMINANCE;
	else if (context->impl->iCurImage->Bpp == 3)
		context->impl->iCurImage->Format = IL_RGB;
	else  // 4
		context->impl->iCurImage->Format = IL_RGBA;

	return ilFixImage(context);
}

//! Writes a Raw file
ILboolean RawHandler::save(const ILstring FileName)
{
	ILHANDLE	RawFile;
	ILuint		RawSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	RawFile = context->impl->iopenw(FileName);
	if (RawFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	RawSize = saveF(RawFile);
	context->impl->iclosew(RawFile);

	if (RawSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes Raw to an already-opened file
ILuint RawHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes Raw to a memory "lump"
ILuint RawHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Internal function used to load the raw data.
ILboolean RawHandler::saveInternal()
{
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	SaveLittleUInt(context, context->impl->iCurImage->Width);
	SaveLittleUInt(context, context->impl->iCurImage->Height);
	SaveLittleUInt(context, context->impl->iCurImage->Depth);
	context->impl->iputc(context, context->impl->iCurImage->Bpp);
	context->impl->iputc(context, context->impl->iCurImage->Bpc);
	context->impl->iwrite(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData);

	return IL_TRUE;
}

#endif // IL_NO_RAW