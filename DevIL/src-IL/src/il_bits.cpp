//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 08/14/2004
//
// Filename: src-IL/src/il_bits.cpp
//
// Description: Implements a file class that reads/writes bits directly.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_bits.h"


// Opens a BITFILE just like fopen opens a FILE.
/*BITFILE *bopen(const char *FileName, const char *Mode)
{
	BITFILE *ToReturn = NULL;

	if (FileName != NULL) {
		ToReturn = (BITFILE*)ialloc(context, sizeof(BITFILE));
		if (ToReturn != NULL) {
			iopenr((char*)FileName);
			ToReturn->File = iGetFile();
			ToReturn->BitPos = 0;
			ToReturn->ByteBitOff = 8;
			ToReturn->Buff = 0;
		}
	}

	return ToReturn;
}*/


// Converts a FILE to a BITFILE.
BITFILE *bfile(ILcontext* context, ILHANDLE File)
{
	BITFILE *ToReturn = NULL;

	if (File != NULL) {
		ToReturn = (BITFILE*)ialloc(context, sizeof(BITFILE));
		if (ToReturn != NULL) {
			ToReturn->File = File;
			ToReturn->BitPos = context->impl->itell(context) << 3;
			ToReturn->ByteBitOff = 8;
			ToReturn->Buff = 0;
		}
	}

	return ToReturn;
}


// Closes an open BITFILE and frees memory for it.
ILint bclose(BITFILE *BitFile)
{
	if (BitFile == NULL || BitFile->File == NULL)
		return IL_EOF;

	// Removed 01-26-2008.  The file will get closed later by
	//  the calling function.
	//context->impl->icloser(BitFile->File);
	ifree(BitFile);

	return 0;
}


// Returns the current bit position of a BITFILE.
ILint btell(BITFILE *BitFile)
{
	return BitFile->BitPos;
}


// Seeks in a BITFILE just like fseek for FILE.
ILint bseek(ILcontext* context, BITFILE *BitFile, ILuint Offset, ILuint Mode)
{
	ILint KeepPos, Len;

	if (BitFile == NULL || BitFile->File == NULL)
		return 1;

	switch (Mode)
	{
		case IL_SEEK_SET:
			if (!context->impl->iseek(context, Offset >> 3, Mode)) {
				BitFile->BitPos = Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_CUR:
			if (!context->impl->iseek(context, Offset >> 3, Mode)) {
				BitFile->BitPos += Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}
			break;
		case IL_SEEK_END:
			KeepPos = context->impl->itell(context);
			context->impl->iseek(context, 0, IL_SEEK_END);
			Len = context->impl->itell(context);
			context->impl->iseek(context, 0, IL_SEEK_SET);

			if (!context->impl->iseek(context, Offset >> 3, Mode)) {
				BitFile->BitPos = (Len << 3) + Offset;
				BitFile->ByteBitOff = BitFile->BitPos % 8;
			}

			break;

		default:
			return 1;
	}

	return 0;
}


// hehe, "bread".  It reads data into Buffer from the BITFILE, just like fread for FILE.
ILint bread(ILcontext* context, void *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile)
{
	// Note that this function is somewhat useless: In binary image
	// formats, there are some pad bits after each scanline. This
	// function does not take that into account, so you must use bseek to
	// skip the calculated value of bits.

	ILuint	BuffPos = 0, Count = Size * Number;

	while (BuffPos < Count) {
		if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
			BitFile->ByteBitOff = 7;
			if (context->impl->iread(context, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
				return BuffPos;
		}

		*((ILubyte*)(Buffer) + BuffPos) = (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

		BuffPos++;
		BitFile->ByteBitOff--;
	}

	return BuffPos;
}


// Reads bits and puts the first bit in the file as the highest in the return value.
ILuint breadVal(ILcontext* context, ILuint NumBits, BITFILE *BitFile)
{
	ILuint	BuffPos = 0;
	ILuint	Buffer = 0;

	// Only returning up to 32 bits at one time
	if (NumBits > 32) {
		ilSetError(context, IL_INTERNAL_ERROR);
		return 0;
	}

	while (BuffPos < NumBits) {
		Buffer <<= 1;
		if (BitFile->ByteBitOff < 0 || BitFile->ByteBitOff > 7) {
			BitFile->ByteBitOff = 7;
			if (context->impl->iread(context, &BitFile->Buff, 1, 1) != 1)  // Reached eof or error...
				return BuffPos;
		}

		Buffer = Buffer + (ILubyte)!!(BitFile->Buff & (1 << BitFile->ByteBitOff));

		BuffPos++;
		BitFile->ByteBitOff--;
	}

	return BuffPos;
}


// Not implemented yet.
/*ILint bwrite(void *Buffer, ILuint Size, ILuint Number, BITFILE *BitFile)
{


	return 0;
}*/
