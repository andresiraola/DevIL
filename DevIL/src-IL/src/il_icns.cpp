//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_icns.cpp
//
// Description: Reads from a Mac OS X icon (.icns) file.
//		Credit for the format of .icns files goes to
//		http://www.macdisk.com/maciconen.php3 and
//		http://ezix.org/project/wiki/MacOSXIcons
//
//-----------------------------------------------------------------------------

//@TODO: Put ilSetError calls in when errors occur.
//@TODO: Should we clear the alpha channel just in case there isn't one in the file?
//@TODO: Checks on iread

#include "il_internal.h"

#ifndef IL_NO_ICNS

#include "il_icns.h"
#include "il_jp2.h"

#ifndef IL_NO_JP2
	#if defined(_WIN32) && defined(IL_USE_PRAGMA_LIBS)
		#if defined(_MSC_VER) || defined(__BORLANDC__)
			#ifndef _DEBUG
				#pragma comment(lib, "libjasper.lib")
			#else
				#pragma comment(lib, "libjasper.lib")  //libjasper-d.lib
			#endif
		#endif
	#endif
#endif//IL_NO_JP2

#ifdef _WIN32
	#pragma pack(push, icns_struct, 1)
#endif
typedef struct ICNSHEAD
{
	char		Head[4];	// Must be 'ICNS'
	ILint		Size;		// Total size of the file (including header)
} IL_PACKSTRUCT ICNSHEAD;

typedef struct ICNSDATA
{
	char		ID[4];		// Identifier ('it32', 'il32', etc.)
	ILint		Size;		// Total size of the entry (including identifier)
} IL_PACKSTRUCT ICNSDATA;

#ifdef _WIN32
	#pragma pack(pop, icns_struct)
#endif

ILboolean iIcnsReadData(ILcontext* context, ILboolean *BaseCreated, ILboolean IsAlpha, ILint Width, ICNSDATA *Entry, ILimage **Image);

IcnsHandler::IcnsHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid .icns file.
ILboolean IcnsHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	IcnsFile;
	ILboolean	bIcns = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("icns"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bIcns;
	}

	IcnsFile = context->impl->iopenr(FileName);
	if (IcnsFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bIcns;
	}

	bIcns = isValidF(IcnsFile);
	context->impl->icloser(IcnsFile);

	return bIcns;
}

//! Checks if the ILHANDLE contains a valid .icns file at the current position.
ILboolean IcnsHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid .icns lump.
ILboolean IcnsHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function to get the header and check it.
ILboolean IcnsHandler::isValidInternal()
{
	ICNSHEAD	Header;

	context->impl->iread(context, Header.Head, 1, 4);
	context->impl->iseek(context, -4, IL_SEEK_CUR);  // Go ahead and restore to previous state

	if (strncmp(Header.Head, "icns", 4))  // First 4 bytes have to be 'icns'.
		return IL_FALSE;

	return IL_TRUE;
}

//! Reads an icon file.
ILboolean IcnsHandler::load(ILconst_string FileName)
{
	ILHANDLE	IcnsFile;
	ILboolean	bIcns = IL_FALSE;

	IcnsFile = context->impl->iopenr(FileName);
	if (IcnsFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bIcns;
	}

	bIcns = loadF(IcnsFile);
	context->impl->icloser(IcnsFile);

	return bIcns;
}

//! Reads an already-opened icon file.
ILboolean IcnsHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains an icon.
ILboolean IcnsHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the icon.
ILboolean IcnsHandler::loadInternal()
{
	ICNSHEAD	Header;
	ICNSDATA	Entry;
	ILimage		*Image = NULL;
	ILboolean	BaseCreated = IL_FALSE;

	if (context->impl->iCurImage == NULL)
	{
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	context->impl->iread(context, Header.Head, 4, 1);
	Header.Size = GetBigInt(context);

	if (strncmp(Header.Head, "icns", 4))  // First 4 bytes have to be 'icns'.
		return IL_FALSE;

	while ((ILint)context->impl->itell(context) < Header.Size && !context->impl->ieof(context))
	{
		context->impl->iread(context, Entry.ID, 4, 1);
		Entry.Size = GetBigInt(context);

		if (!strncmp(Entry.ID, "it32", 4))  // 128x128 24-bit
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 128, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "t8mk", 4))  // 128x128 alpha mask
		{
			if (iIcnsReadData(context, &BaseCreated, IL_TRUE, 128, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "ih32", 4))  // 48x48 24-bit
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 48, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "h8mk", 4))  // 48x48 alpha mask
		{
			if (iIcnsReadData(context, &BaseCreated, IL_TRUE, 48, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "il32", 4))  // 32x32 24-bit
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 32, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "l8mk", 4))  // 32x32 alpha mask
		{
			if (iIcnsReadData(context, &BaseCreated, IL_TRUE, 32, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "is32", 4))  // 16x16 24-bit
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 16, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "s8mk", 4))  // 16x16 alpha mask
		{
			if (iIcnsReadData(context, &BaseCreated, IL_TRUE, 16, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
#ifndef IL_NO_JP2
		else if (!strncmp(Entry.ID, "ic09", 4))  // 512x512 JPEG2000 encoded - Uses JasPer
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 512, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
		else if (!strncmp(Entry.ID, "ic08", 4))  // 256x256 JPEG2000 encoded - Uses JasPer
		{
			if (iIcnsReadData(context, &BaseCreated, IL_FALSE, 256, &Entry, &Image) == IL_FALSE)
				goto icns_error;
		}
#endif//IL_NO_JP2
		else  // Not a valid format or one that we can use
		{
			context->impl->iseek(context, Entry.Size - 8, IL_SEEK_CUR);
		}
	}

	return ilFixImage(context);

icns_error:
	return IL_FALSE;
}

ILboolean iIcnsReadData(ILcontext* context, ILboolean *BaseCreated, ILboolean IsAlpha, ILint Width, ICNSDATA *Entry, ILimage **Image)
{
	ILint		Position = 0, RLEPos = 0, Channel, i;
	ILubyte		RLERead, *Data = NULL;
	ILimage		*TempImage = NULL;
	ILboolean	ImageAlreadyExists = IL_FALSE;

	// The .icns format stores the alpha and RGB as two separate images, so this
	//  checks to see if one exists for that particular size.  Unfortunately,
	//	there is no guarantee that they are in any particular order.
	if (*BaseCreated && context->impl->iCurImage != NULL)
	{
		TempImage = context->impl->iCurImage;
		while (TempImage != NULL)
		{
			if ((ILuint)Width == TempImage->Width)
			{
				ImageAlreadyExists = IL_TRUE;
				break;
			}
			TempImage = TempImage->Next;
		}
	}

	Data = (ILubyte*)ialloc(context, Entry->Size - 8);
	if (Data == NULL)
		return IL_FALSE;

	if (!ImageAlreadyExists)
	{
		if (!*BaseCreated)  // Create base image
		{
			ilTexImage(context, Width, Width, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
			context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
			*Image = context->impl->iCurImage;
			*BaseCreated = IL_TRUE;
		}
		else  // Create next image in list
		{
			(*Image)->Next = ilNewImage(context, Width, Width, 1, 4, 1);
			*Image = (*Image)->Next;
			(*Image)->Format = IL_RGBA;
			(*Image)->Origin = IL_ORIGIN_UPPER_LEFT;
		}

		TempImage = *Image;
	}

	if (IsAlpha)  // Alpha is never compressed.
	{
		context->impl->iread(context, Data, Entry->Size - 8, 1);  // Size includes the header
		if (Entry->Size - 8 != Width * Width)
		{
			ifree(Data);
			return IL_FALSE;
		}

		for (i = 0; i < Width * Width; i++)
		{
			TempImage->Data[(i * 4) + 3] = Data[i];
		}
	}
	else if (Width == 256 || Width == 512)  // JPEG2000 encoded - uses JasPer
	{
#ifndef IL_NO_JP2
		context->impl->iread(context, Data, Entry->Size - 8, 1);  // Size includes the header
		if (Jp2Handler::LoadLToImage(context, Data, Entry->Size - 8, TempImage) == IL_FALSE)
		{
			ifree(Data);
			ilSetError(context, IL_LIB_JP2_ERROR);
			return IL_TRUE;
		}
#else  // Cannot handle this size.
		ilSetError(context, IL_LIB_JP2_ERROR);  //@TODO: Handle this better...just skip the data.
		return IL_FALSE;
#endif//IL_NO_JP2
	}
	else  // RGB data
	{
		context->impl->iread(context, Data, Entry->Size - 8, 1);  // Size includes the header
		if (Width == 128)
			RLEPos += 4;  // There are an extra 4 bytes here of zeros.
						  //@TODO: Should we check to make sure they are all 0?

		if (Entry->Size - 8 == Width * Width * 4) // Uncompressed
		{
			//memcpy(TempImage->Data, Data, Entry->Size);
			for (i = 0; i < Width * Width; i++, Position += 4)
			{
				TempImage->Data[i * 4 + 0] = Data[Position + 1];
				TempImage->Data[i * 4 + 1] = Data[Position + 2];
				TempImage->Data[i * 4 + 2] = Data[Position + 3];
			}
		}
		else  // RLE decoding
		{
			for (Channel = 0; Channel < 3; Channel++)
			{
				Position = 0;
				while (Position < Width * Width)
				{
					RLERead = Data[RLEPos];
					RLEPos++;

					if (RLERead >= 128)
					{
						for (i = 0; i < RLERead - 125 && (Position + i) < Width * Width; i++)
						{
							TempImage->Data[Channel + (Position + i) * 4] = Data[RLEPos];
						}
						RLEPos++;
						Position += RLERead - 125;
					}
					else
					{
						for (i = 0; i < RLERead + 1 && (Position + i) < Width * Width; i++)
						{
							TempImage->Data[Channel + (Position + i) * 4] = Data[RLEPos + i];
						}
						RLEPos += RLERead + 1;
						Position += RLERead + 1;
					}
				}
			}
		}
	}

	ifree(Data);
	return IL_TRUE;
}

#endif//IL_NO_ICNS