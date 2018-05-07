//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_pic.cpp
//
// Description: Softimage Pic (.pic) functions
//	Lots of this code is taken from Paul Bourke's Softimage Pic code at
//	http://local.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_PIC
#include "il_pic.h"
#include <string.h>


//! Checks if the file specified in FileName is a valid .pic file.
ILboolean ilIsValidPic(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PicFile;
	ILboolean	bPic = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("pic"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bPic;
	}

	PicFile = context->impl->iopenr(FileName);
	if (PicFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPic;
	}

	bPic = ilIsValidPicF(context, PicFile);
	context->impl->icloser(PicFile);

	return bPic;
}


//! Checks if the ILHANDLE contains a valid .pic file at the current position.
ILboolean ilIsValidPicF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iIsValidPic(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid .pic lump.
ILboolean ilIsValidPicL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iIsValidPic(context);
}


// Internal function used to get the .pic header from the current file.
ILboolean iGetPicHead(ILcontext* context, PIC_HEAD *Header)
{
	Header->Magic = GetBigInt(context);
	Header->Version = GetBigFloat(context);
	context->impl->iread(context, Header->Comment, 1, 80);
	context->impl->iread(context, Header->Id, 1, 4);
	Header->Width = GetBigShort(context);
	Header->Height = GetBigShort(context);
	Header->Ratio = GetBigFloat(context);
	Header->Fields = GetBigShort(context);
	Header->Padding = GetBigShort(context);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPic(ILcontext* context)
{
	PIC_HEAD	Head;

	if (!iGetPicHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -(ILint)sizeof(PIC_HEAD), IL_SEEK_CUR);  // Go ahead and restore to previous state

	return iCheckPic(&Head);
}


// Internal function used to check if the header is a valid .pic header.
ILboolean iCheckPic(PIC_HEAD *Header)
{
	if (Header->Magic != 0x5380F634)
		return IL_FALSE;
	if (strncmp((const char*)Header->Id, "PICT", 4))
		return IL_FALSE;
	if (Header->Width == 0)
		return IL_FALSE;
	if (Header->Height == 0)
		return IL_FALSE;

	return IL_TRUE;
}


//! Reads a .pic file
ILboolean ilLoadPic(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	PicFile;
	ILboolean	bPic = IL_FALSE;

	PicFile = context->impl->iopenr(FileName);
	if (PicFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bPic;
	}

	bPic = ilLoadPicF(context, PicFile);
	context->impl->icloser(PicFile);

	return bPic;
}


//! Reads an already-opened .pic file
ILboolean ilLoadPicF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadPicInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .pic
ILboolean ilLoadPicL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadPicInternal(context);
}


// Internal function used to load the .pic
ILboolean iLoadPicInternal(ILcontext* context)
{
	ILuint		Alpha = IL_FALSE;
	ILubyte		Chained;
	CHANNEL		*Channel = NULL, *Channels = NULL, *Prev;
	PIC_HEAD	Header;
	ILboolean	Read;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetPicHead(context, &Header))
		return IL_FALSE;
	if (!iCheckPic(&Header)) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	// Read channels
	do {
		if (Channel == NULL) {
			Channel = Channels = (CHANNEL*)ialloc(context, sizeof(CHANNEL));
			if (Channels == NULL)
				return IL_FALSE;
		}
		else {
			Channels->Next = (CHANNEL*)ialloc(context, sizeof(CHANNEL));
			if (Channels->Next == NULL) {
				// Clean up the list before erroring out.
				while (Channel) {
					Prev = Channel;
					Channel = (CHANNEL*)Channel->Next;
					ifree(Prev);
				}
				return IL_FALSE;
			}
			Channels = (CHANNEL*)Channels->Next;
		}
		Channels->Next = NULL;

		Chained = context->impl->igetc(context);
		Channels->Size = context->impl->igetc(context);
		Channels->Type = context->impl->igetc(context);
		Channels->Chan = context->impl->igetc(context);
		if (context->impl->ieof(context)) {
			Read = IL_FALSE;
			goto finish;
		}
		
		// See if we have an alpha channel in there
		if (Channels->Chan & PIC_ALPHA_CHANNEL)
			Alpha = IL_TRUE;
		
	} while (Chained);

	if (Alpha) {  // Has an alpha channel
		if (!ilTexImage(context, Header.Width, Header.Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	else {  // No alpha channel
		if (!ilTexImage(context, Header.Width, Header.Height, 1, 3, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
			Read = IL_FALSE;
			goto finish;  // Have to destroy Channels first.
		}
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	Read = readScanlines(context, (ILuint*)context->impl->iCurImage->Data, Header.Width, Header.Height, Channel, Alpha);

finish:
	// Destroy channels
	while (Channel) {
		Prev = Channel;
		Channel = (CHANNEL*)Channel->Next;
		ifree(Prev);
	}

	if (Read == IL_FALSE)
		return IL_FALSE;

	return ilFixImage(context);
}


ILboolean readScanlines(ILcontext* context, ILuint *image, ILint width, ILint height, CHANNEL *channel, ILuint alpha)
{
	ILint	i;
	ILuint	*scan;

	(void)alpha;
	
	for (i = height - 1; i >= 0; i--) {
		scan = image + i * width;

		if (!readScanline(context, (ILubyte *)scan, width, channel, alpha ? 4 : 3)) {
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
		}
	}

	return IL_TRUE;
}


ILuint readScanline(ILcontext* context, ILubyte *scan, ILint width, CHANNEL *channel, ILint bytes)
{
	ILint		noCol;
	ILint		off[4];
	ILuint		status=0;

	while (channel) {
		noCol = 0;
		if(channel->Chan & PIC_RED_CHANNEL) {
			off[noCol] = 0;
			noCol++;
		}
		if(channel->Chan & PIC_GREEN_CHANNEL) {
			off[noCol] = 1;
			noCol++;
		}
		if(channel->Chan & PIC_BLUE_CHANNEL) {
			off[noCol] = 2;
			noCol++;
		}
		if(channel->Chan & PIC_ALPHA_CHANNEL) {
			off[noCol] = 3;
			noCol++;
			//@TODO: Find out if this is possible.
			if (bytes == 3)  // Alpha channel in a 24-bit image.  Do not know what to do with this.
				return 0;
		}

		switch(channel->Type & 0x0F)
		{
			case PIC_UNCOMPRESSED:
				status = channelReadRaw(context, scan, width, noCol, off, bytes);
				break;
			case PIC_PURE_RUN_LENGTH:
				status = channelReadPure(context, scan, width, noCol, off, bytes);
				break;
			case PIC_MIXED_RUN_LENGTH:
				status = channelReadMixed(context, scan, width, noCol, off, bytes);
				break;
		}
		if (!status)
			break;

		channel = (CHANNEL*)channel->Next;
	}
	return status;
}


ILboolean channelReadRaw(ILcontext* context, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint i, j;

	for (i = 0; i < width; i++) {
		if (context->impl->ieof(context))
			return IL_FALSE;
		for (j = 0; j < noCol; j++)
			if (context->impl->iread(context, &scan[off[j]], 1, 1) != 1)
				return IL_FALSE;
		scan += bytes;
	}
	return IL_TRUE;
}


ILboolean channelReadPure(ILcontext* context, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILubyte		col[4];
	ILint		count;
	int			i, j, k;

	for (i = width; i > 0; ) {
		count = context->impl->igetc(context);
		if (count == IL_EOF)
			return IL_FALSE;
		if (count > width)
			count = width;
		i -= count;
		
		if (context->impl->ieof(context))
			return IL_FALSE;
		
		for (j = 0; j < noCol; j++)
			if (context->impl->iread(context, &col[j], 1, 1) != 1)
				return IL_FALSE;
		
		for (k = 0; k < count; k++, scan += bytes) {
			for(j = 0; j < noCol; j++)
				scan[off[j] + k] = col[j];
		}
	}
	return IL_TRUE;
}


ILboolean channelReadMixed(ILcontext* context, ILubyte *scan, ILint width, ILint noCol, ILint *off, ILint bytes)
{
	ILint	count;
	int		i, j, k;
	ILubyte	col[4];

	for(i = 0; i < width; i += count) {
		if (context->impl->ieof(context))
			return IL_FALSE;

		count = context->impl->igetc(context);
		if (count == IL_EOF)
			return IL_FALSE;

		if (count >= 128) {  // Repeated sequence
			if (count == 128) {  // Long run
				count = GetBigUShort(context);
				if (context->impl->ieof(context)) {
					ilSetError(context, IL_FILE_READ_ERROR);
					return IL_FALSE;
				}
			}
			else
				count -= 127;
			
			// We've run past...
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Repeat) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				ilSetError(context, IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}

			for (j = 0; j < noCol; j++)
				if (context->impl->iread(context, &col[j], 1, 1) != 1) {
					ilSetError(context, IL_FILE_READ_ERROR);
					return IL_FALSE;
				}

			for (k = 0; k < count; k++, scan += bytes) {
				for (j = 0; j < noCol; j++)
					scan[off[j]] = col[j];
			}
		} else {				// Raw sequence
			count++;
			if ((i + count) > width) {
				//fprintf(stderr, "ERROR: FF_PIC_load(): Overrun scanline (Raw) [%d + %d > %d] (NC=%d)\n", i, count, width, noCol);
				ilSetError(context, IL_ILLEGAL_FILE_VALUE);
				return IL_FALSE;
			}
			
			for (k = count; k > 0; k--, scan += bytes) {
				for (j = 0; j < noCol; j++)
					if (context->impl->iread(context, &scan[off[j]], 1, 1) != 1) {
						ilSetError(context, IL_FILE_READ_ERROR);
						return IL_FALSE;
					}
			}
		}
	}

	return IL_TRUE;
}


#endif//IL_NO_PIC

