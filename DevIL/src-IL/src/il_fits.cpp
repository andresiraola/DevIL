//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_fits.cpp
//
// Description: Reads from a Flexible Image Transport System (.fits) file.
//                Specifications were found at 
//                http://www.fileformat.info/format/fits.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_FITS

typedef struct FITSHEAD
{
	ILboolean	IsSimple;
	ILint		BitsPixel;
	ILint		NumAxes;  // Number of dimensions / axes
	ILint		Width;
	ILint		Height;
	ILint		Depth;
	ILint		NumChans;

	// Not in the header, but it keeps everything together.
	ILenum		Type;
	ILenum		Format;

} FITSHEAD;

enum {
	CARD_READ_FAIL = -1,
	CARD_END = 1,
	CARD_SIMPLE,
	CARD_NOT_SIMPLE,
	CARD_BITPIX,
	CARD_NUMAXES,
	CARD_AXIS,
	CARD_SKIP
};

ILboolean	iIsValidFits(ILcontext* context);
ILboolean	iCheckFits(ILcontext* context, FITSHEAD *Header);
ILboolean	iLoadFitsInternal(ILcontext* context);
ILenum		GetCardImage(ILcontext* context, FITSHEAD *Header);
ILboolean	GetCardInt(char *Buffer, ILint *Val);

//! Checks if the file specified in FileName is a valid FITS file.
ILboolean ilIsValidFits(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	FitsFile;
	ILboolean	bFits = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("fits")) && !iCheckExtension(FileName, IL_TEXT("fit"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bFits;
	}
	
	FitsFile = context->impl->iopenr(FileName);
	if (FitsFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bFits;
	}
	
	bFits = ilIsValidFitsF(context, FitsFile);
	context->impl->icloser(FitsFile);
	
	return bFits;
}


//! Checks if the ILHANDLE contains a valid FITS file at the current position.
ILboolean ilIsValidFitsF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidFits(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}


//! Checks if Lump is a valid FITS lump.
ILboolean ilIsValidFitsL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidFits(context);
}


// Internal function used to get the FITS header from the current file.
ILboolean iGetFitsHead(ILcontext* context, FITSHEAD *Header)
{
	ILenum	CardKey;

//@TODO: Use something other than memset?
	memset(Header, 0, sizeof(Header));  // Clear the header to all 0s first.

	do {
		CardKey = GetCardImage(context, Header);
		if (CardKey == CARD_END)  // End of the header
			break;
		if (CardKey == CARD_READ_FAIL)
			return IL_FALSE;
		if (CardKey == CARD_NOT_SIMPLE)
			return IL_FALSE;
	} while (!context->impl->ieof(context));

	// The header should never reach the end of the file.
	if (context->impl->ieof(context))
		return IL_FALSE;  // Error needed?

	// The header must always be a multiple of 2880, so we skip the padding bytes (spaces).
	context->impl->iseek(context, (2880 - (context->impl->itell(context) % 2880)) % 2880, IL_SEEK_CUR);

	switch (Header->BitsPixel)
	{
		case 8:
			Header->Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Header->Type = IL_SHORT;
			break;
		case 32:
			Header->Type = IL_INT;
			break;
		case -32:
			Header->Type = IL_FLOAT;
			break;
		case -64:
			Header->Type = IL_DOUBLE;
			break;
		default:
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	switch (Header->NumAxes)
	{
		case 1:  // Just a 1D image
			Header->Format = IL_LUMINANCE;
			Header->Height = 1;
			Header->Depth = 1;
			Header->NumChans = 1;
			break;

		case 2:  // Assuming it is a 2D image (width+height)
			Header->Format = IL_LUMINANCE;
			Header->Depth = 1;
			Header->NumChans = 1;
			break;

		case 3:
			// We cannot deal with more than 3 channels in an image.
			Header->Format = IL_LUMINANCE;
			Header->NumChans = 1;
			break;

		default:
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return IL_FALSE;
	}

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidFits(ILcontext* context)
{
	FITSHEAD	Header;
	ILuint		Pos = context->impl->itell(context);

	if (!iGetFitsHead(context, &Header))
		return IL_FALSE;
	// The length of the header varies, so we just go back to the original position.
	context->impl->iseek(context, Pos, IL_SEEK_CUR);

	return iCheckFits(context, &Header);
}


// Internal function used to check if the HEADER is a valid FITS header.
ILboolean iCheckFits(ILcontext* context, FITSHEAD *Header)
{
	switch (Header->BitsPixel)
	{
		case 8:  // These are the only values accepted.
		case 16:
		case 32:
		case -32:
		case -64:
			break;
		default:
			return IL_FALSE;
	}

	switch (Header->NumAxes)
	{
		case 1:  // Just a 1D image
		case 2:  // Assuming it is a 2D image (width+height)
		case 3:  // 3D image (with depth)
			break;
		default:
			return IL_FALSE;
	}

	// Possibility that one of these values is returned as <= 0 by atoi, which we cannot use.
	if (Header->Width <= 0 || Header->Height <= 0 || Header->Depth <= 0) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	return IL_TRUE;
}


//! Reads a FITS file
ILboolean ilLoadFits(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	FitsFile;
	ILboolean	bFits = IL_FALSE;
	
	FitsFile = context->impl->iopenr(FileName);
	if (FitsFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bFits;
	}

	bFits = ilLoadFitsF(context, FitsFile);
	context->impl->icloser(FitsFile);

	return bFits;
}


//! Reads an already-opened FITS file
ILboolean ilLoadFitsF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadFitsInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}


//! Reads from a memory "lump" that contains a FITS
ILboolean ilLoadFitsL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadFitsInternal(context);
}


// Internal function used to load the FITS.
ILboolean iLoadFitsInternal(ILcontext* context)
{
	FITSHEAD	Header;
	ILuint		i, NumPix;
	ILfloat		MaxF = 0.0f;
	ILdouble	MaxD = 0.0f;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetFitsHead(context, &Header))
		return IL_FALSE;
	if (!iCheckFits(context, &Header))
		return IL_FALSE;

	if (!ilTexImage(context, Header.Width, Header.Height, Header.Depth, Header.NumChans, Header.Format, Header.Type, NULL))
		return IL_FALSE;

	/*if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData)
		return IL_FALSE;*/

	NumPix = Header.Width * Header.Height * Header.Depth;
//@TODO: Do some checks while reading to see if we have hit the end of the file.
	switch (Header.Type)
	{
		case IL_UNSIGNED_BYTE:
			if (context->impl->iread(context, context->impl->iCurImage->Data, 1, context->impl->iCurImage->SizeOfData) != context->impl->iCurImage->SizeOfData)
				return IL_FALSE;
			break;
		case IL_SHORT:
			for (i = 0; i < NumPix; i++) {
				((ILshort*)context->impl->iCurImage->Data)[i] = GetBigShort(context);
			}
			break;
		case IL_INT:
			for (i = 0; i < NumPix; i++) {
				((ILint*)context->impl->iCurImage->Data)[i] = GetBigInt(context);
			}
			break;
		case IL_FLOAT:
			for (i = 0; i < NumPix; i++) {
				((ILfloat*)context->impl->iCurImage->Data)[i] = GetBigFloat(context);
				if (((ILfloat*)context->impl->iCurImage->Data)[i] > MaxF)
					MaxF = ((ILfloat*)context->impl->iCurImage->Data)[i];
			}

			// Renormalize to [0..1].
			for (i = 0; i < NumPix; i++) {
				// Change all negative numbers to 0.
				if (((ILfloat*)context->impl->iCurImage->Data)[i] < 0.0f)
					((ILfloat*)context->impl->iCurImage->Data)[i] = 0.0f;
				// Do the renormalization now, dividing by the maximum value.
				((ILfloat*)context->impl->iCurImage->Data)[i] = ((ILfloat*)context->impl->iCurImage->Data)[i] / MaxF;
			}
			break;
		case IL_DOUBLE:
			for (i = 0; i < NumPix; i++) {
				((ILdouble*)context->impl->iCurImage->Data)[i] = GetBigDouble(context);
				if (((ILdouble*)context->impl->iCurImage->Data)[i] > MaxD)
					MaxD = ((ILdouble*)context->impl->iCurImage->Data)[i];
			}

			// Renormalize to [0..1].
			for (i = 0; i < NumPix; i++) {
				// Change all negative numbers to 0.
				if (((ILdouble*)context->impl->iCurImage->Data)[i] < 0.0f)
					((ILdouble*)context->impl->iCurImage->Data)[i] = 0.0f;
				// Do the renormalization now, dividing by the maximum value.
				((ILdouble*)context->impl->iCurImage->Data)[i] = ((ILdouble*)context->impl->iCurImage->Data)[i] / MaxD;
			}			break;
	}

	return ilFixImage(context);
}


//@TODO: NAXISx have to come in order.  Check this!
ILenum GetCardImage(ILcontext* context, FITSHEAD *Header)
{
	char	Buffer[80];

	if (context->impl->iread(context, Buffer, 1, 80) != 80)  // Each card image is exactly 80 bytes long.
		return CARD_READ_FAIL;

//@TODO: Use something other than !strncmp?
	if (!strncmp(Buffer, "END ", 4))
		return CARD_END;

	else if (!strncmp(Buffer, "SIMPLE ", 7)) {
		// The true value 'T' is always in the 30th position.
		if (Buffer[29] != 'T') {
			// We cannot support FITS files that do not correspond to the standard.
			Header->IsSimple = IL_FALSE;  //@TODO: Does this even need to be set?  Should exit loading anyway.
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return CARD_NOT_SIMPLE;
		}
		Header->IsSimple = IL_TRUE;
		return CARD_SIMPLE;
	}

	else if (!strncmp(Buffer, "BITPIX ", 7)) {
		// The specs state that BITPIX has to come after SIMPLE.
		if (Header->IsSimple != IL_TRUE) {
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}
		if (GetCardInt(Buffer, &Header->BitsPixel) != IL_TRUE)
			return CARD_READ_FAIL;
//@TODO: Should I do this check from the calling function?  Does it really matter?
		if (Header->BitsPixel == 0) {
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return CARD_READ_FAIL;
		}
		return CARD_BITPIX;
	}

	// Needs the space after NAXIS so that it does not get this confused with NAXIS1, NAXIS2, etc.
	else if (!strncmp(Buffer, "NAXIS ", 6)) {
		if (GetCardInt(Buffer, &Header->NumAxes) != IL_TRUE)
			return CARD_READ_FAIL;
//@TODO: Should I do this check from the calling function?  Does it really matter?
		if (Header->NumAxes < 1 || Header->NumAxes > 3) {
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
			return CARD_READ_FAIL;
		}
		return CARD_NUMAXES;
	}

	else if (!strncmp(Buffer, "NAXIS1 ", 7)) {
		if (Header->NumAxes == 0) {  // Has not been initialized, and it has to come first.
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}
		// First one will always be the width.
		if (GetCardInt(Buffer, &Header->Width) != IL_TRUE)
			return CARD_READ_FAIL;
		return CARD_AXIS;
	}

	else if (!strncmp(Buffer, "NAXIS2 ", 7)) {
		if (Header->NumAxes == 0) {  // Has not been initialized, and it has to come first.
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}
		// Cannot have a 2nd axis for 0 or 1.
		if (Header->NumAxes == 0 || Header->NumAxes == 1) {
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}
		
//@TODO: We are assuming that this is the height right now.  Could it just be a
//  1D image with multiple bytes per pixel?
		if (GetCardInt(Buffer, &Header->Height) != IL_TRUE)
			return CARD_READ_FAIL;
		return CARD_AXIS;
	}

	else if (!strncmp(Buffer, "NAXIS3 ", 7)) {
		if (Header->NumAxes == 0) {  // Has not been initialized, and it has to come first.
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}
		// Cannot have a 3rd axis for 0, 1 and 2.
		if (Header->NumAxes < 3) {
			ilSetError(context, IL_INVALID_FILE_HEADER);
			return CARD_READ_FAIL;
		}

		if (GetCardInt(Buffer, &Header->Depth) != IL_TRUE)
			return CARD_READ_FAIL;
		return CARD_AXIS;
	}

	return CARD_SKIP;  // This is a card that we do not recognize, so skip it.
}


ILboolean GetCardInt(char *Buffer, ILint *Val)
{
	ILuint	i;
	char	ValString[22];

	if (Buffer[7] != '=' && Buffer[8] != '=')
		return IL_FALSE;
	for (i = 9; i < 30; i++) {
		if (Buffer[i] != ' ' && Buffer[i] != 0)  // Right-aligned with ' ' or 0, so skip.
			break;
	}
	if (i == 30)  // Did not find the integer in positions 10 - 30.
		return IL_FALSE;

  //@TODO: Safest way to do this?
	memcpy(ValString, &Buffer[i], 30-i);
	ValString[30-i] = 0;  // Terminate the string.

	//@TODO: Check the return somehow?
	*Val = atoi(ValString);

	return IL_TRUE;
}

#endif//IL_NO_FITS
