//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/21/2017
//
// Filename: src-IL/src/il_scitex.cpp
//
// Description: Reads from a Scitex Continuous Tone (.ct) file.
//                Specifications were found at http://fileformats.archiveteam.org/wiki/Scitex_CT
//				  and http://web.archive.org/web/20130506073138/http://electricmessiah.org/press/the-scitex-ct-file-format/
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_SCITEX
//#include "il_scitex.h"


typedef struct SCITEXHEAD
{
	ILubyte	Comment[80];
	ILubyte	Sig[2];			// Signature, should be "CT"
	ILubyte	Units;			// 0 for mm, 1 for inches
	ILubyte	NumChans;		// Number of channels
	ILushort ColorModel;	// Color model (7=RGB, 8=GREYSCALE, 15=CMYK)
	ILfloat	HeightUnits;	// Physical height
	ILfloat	WidthUnits;		// Physical width
	ILuint	HeightPixels;
	ILuint	WidthPixels;
} SCITEXHEAD;

ILboolean iIsValidScitex(ILcontext* context);
ILboolean iLoadScitexInternal(ILcontext* context);
ILboolean iCheckScitex(SCITEXHEAD *Header);


// Internal function used to get the Scitex header from the current file.
ILboolean iGetScitexHead(ILcontext* context, SCITEXHEAD *Header)
{
	char HeightUnits[15], WidthUnits[15], HeightPixels[13], WidthPixels[13];

	if (context->impl->iread(context, Header->Comment, 1, 80) != 80)
		return IL_FALSE;
	if (context->impl->iread(context, Header->Sig, 1, 2) != 2)
		return IL_FALSE;
	if (context->impl->iseek(context, 942, IL_SEEK_CUR) == -1)
		return IL_FALSE;
	Header->Units = context->impl->igetc(context);
	Header->NumChans = context->impl->igetc(context);
	Header->ColorModel = GetBigUShort(context);
	if (context->impl->iread(context, &HeightUnits, 1, 14) != 14)
		return IL_FALSE;
	if (context->impl->iread(context, &WidthUnits, 1, 14) != 14)
		return IL_FALSE;
	if (context->impl->iread(context, &HeightPixels, 1, 12) != 12)
		return IL_FALSE;
	if (context->impl->iread(context, &WidthPixels, 1, 12) != 12)
		return IL_FALSE;

	HeightUnits[14] = NULL;
	WidthUnits[14] = NULL;
	HeightPixels[12] = NULL;
	WidthPixels[12] = NULL;

	Header->HeightUnits = (ILfloat)atof(HeightUnits);
	Header->WidthUnits = (ILfloat)atof(WidthUnits);
	Header->HeightPixels = (ILint)atoi(HeightPixels);
	Header->WidthPixels = (ILint)atof(WidthPixels);

	switch (Header->ColorModel)
	{
		case 7:
			if (Header->NumChans != 3)
				return IL_FALSE;
			break;
		case 8:
			if (Header->NumChans != 1)
				return IL_FALSE;
			break;
		//case 15:
		//default:
	}

	return IL_TRUE;
}


//! Checks if the file specified in FileName is a valid BLP file.
ILboolean ilIsValidScitex(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	ScitexFile;
	ILboolean	bScitex = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("ct"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bScitex;
	}

	ScitexFile = context->impl->iopenr(FileName);
	if (ScitexFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bScitex;
	}

	bScitex = ilIsValidScitexF(context, ScitexFile);
	context->impl->icloser(ScitexFile);

	return bScitex;
}


//! Checks if the ILHANDLE contains a valid Scitex file at the current position.
ILboolean ilIsValidScitexF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidScitex(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid BLP lump.
ILboolean ilIsValidScitexL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidScitex(context);
}


// Internal function to get the header and check it.
ILboolean iIsValidScitex(ILcontext* context)
{
	SCITEXHEAD Header;

	if (!iGetScitexHead(context, &Header))
		return IL_FALSE;
	context->impl->iseek(context, -2048, IL_SEEK_CUR);

	return iCheckScitex(&Header);
}


// Internal function used to check if the HEADER is a valid BLP header.
ILboolean iCheckScitex(SCITEXHEAD *Header)
{
	// The file signature is 'CT'.
	if (Header->Sig[0] != 'C' || Header->Sig[1] != 'T')
		return IL_FALSE;
	if (Header->Units != 0 && Header->Units != 1)
		return IL_FALSE;
	//@TODO: Handle CMYK version
	if (Header->ColorModel != 7 && Header->ColorModel != 8 /*&& Header->ColorModel != 15*/)
		return IL_FALSE;
	if (Header->HeightUnits <= 0 || Header->WidthUnits <= 0)
		return IL_FALSE;
	if (Header->HeightPixels <= 0 || Header->WidthPixels <= 0)
		return IL_FALSE;

	return IL_TRUE;
}



//! Reads a BLP file
ILboolean ilLoadScitex(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	ScitexFile;
	ILboolean	bScitex = IL_FALSE;

	ScitexFile = context->impl->iopenr(FileName);
	if (ScitexFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bScitex;
	}

	bScitex = ilLoadScitexF(context, ScitexFile);
	context->impl->icloser(ScitexFile);

	return bScitex;
}


//! Reads an already-opened BLP file
ILboolean ilLoadScitexF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadScitexInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a BLP
ILboolean ilLoadScitexL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadScitexInternal(context);
}


// Internal function used to load the BLP.
ILboolean iLoadScitexInternal(ILcontext* context)
{
	SCITEXHEAD Header;
	ILuint Pos = 0;
	ILubyte *RowData;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetScitexHead(context, &Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if (!iCheckScitex(&Header)) {
		return IL_FALSE;
	}

	context->impl->iseek(context, 2048, IL_SEEK_SET);
	RowData = (ILubyte*)ialloc(context, Header.WidthPixels);
	if (RowData == NULL)
		return IL_FALSE;

	switch (Header.ColorModel)
	{
		case 7:
			if (!ilTexImage(context, Header.WidthPixels, Header.HeightPixels, 1, Header.NumChans, 0, IL_UNSIGNED_BYTE, NULL))
			{
				ifree(RowData);
				return IL_FALSE;
			}
			context->impl->iCurImage->Format = IL_RGB;

			for (ILuint h = 0; h < context->impl->iCurImage->Height; h++)
			{
				for (ILuint c = 0; c < context->impl->iCurImage->Bpp; c++)
				{
					if (context->impl->iread(context, RowData, 1, Header.WidthPixels) != Header.WidthPixels)
					{
						ifree(RowData);
						return IL_FALSE;
					}
					for (ILuint w = 0; w < context->impl->iCurImage->Width; w++)
					{
						context->impl->iCurImage->Data[Pos + w * 3 + c] = RowData[w];
					}
					if (context->impl->iCurImage->Width % 2 != 0)  // Pads to even width
						context->impl->igetc(context);
				}
				Pos += Header.WidthPixels * 3;
			}
			break;

		case 8:
			if (!ilTexImage(context, Header.WidthPixels, Header.HeightPixels, 1, Header.NumChans, 0, IL_UNSIGNED_BYTE, NULL))
			{
				ifree(RowData);
				return IL_FALSE;
			}
			context->impl->iCurImage->Format = IL_LUMINANCE;

			for (ILuint h = 0; h < context->impl->iCurImage->Height; h++)
			{
				context->impl->iread(context, context->impl->iCurImage->Data + Pos, 1, context->impl->iCurImage->Width);
				if (context->impl->iCurImage->Width % 2 != 0)  // Pads to even width
					context->impl->igetc(context);
				Pos += Header.WidthPixels;
			}

			break;

		//case 15:
		//default:
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	return ilFixImage(context);
};



#endif//IL_NO_SCITEX
