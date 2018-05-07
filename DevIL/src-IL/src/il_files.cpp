//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_files.cpp
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------


#define __FILES_C
#include "il_internal.h"
#include <stdarg.h>


// All specific to the next set of functions
ILboolean	ILAPIENTRY iEofFile(ILcontext* context);
ILboolean	ILAPIENTRY iEofLump(ILcontext* context);
ILint		ILAPIENTRY iGetcFile(ILcontext* context);
ILint		ILAPIENTRY iGetcLump(ILcontext* context);
ILuint		ILAPIENTRY iReadFile(ILcontext* context, void *Buffer, ILuint Size, ILuint Number);
ILuint		ILAPIENTRY iReadLump(ILcontext* context, void *Buffer, const ILuint Size, const ILuint Number);
ILint		ILAPIENTRY iSeekRFile(ILcontext* context, ILint Offset, ILuint Mode);
ILint		ILAPIENTRY iSeekRLump(ILcontext* context, ILint Offset, ILuint Mode);
ILint		ILAPIENTRY iSeekWFile(ILcontext* context, ILint Offset, ILuint Mode);
ILint		ILAPIENTRY iSeekWLump(ILcontext* context, ILint Offset, ILuint Mode);
ILuint		ILAPIENTRY iTellRFile(ILcontext* context);
ILuint		ILAPIENTRY iTellRLump(ILcontext* context);
ILuint		ILAPIENTRY iTellWFile(ILcontext* context);
ILuint		ILAPIENTRY iTellWLump(ILcontext* context);
ILint		ILAPIENTRY iPutcFile(ILcontext* context, ILubyte Char);
ILint		ILAPIENTRY iPutcLump(ILcontext* context, ILubyte Char);
ILint		ILAPIENTRY iWriteFile(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number);
ILint		ILAPIENTRY iWriteLump(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number);

// "Fake" size functions
//  Definitions are in il_size.c.
ILint		ILAPIENTRY iSizeSeek(ILcontext* context, ILint Offset, ILuint Mode);
ILuint		ILAPIENTRY iSizeTell(ILcontext* context);
ILint		ILAPIENTRY iSizePutc(ILcontext* context, ILubyte Char);
ILint		ILAPIENTRY iSizeWrite(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number);

// Just preserves the current read functions and replaces
//	the current read functions with the default read funcs.
void ILAPIENTRY iPreserveReadFuncs(ILcontext* context)
{
	// Create backups
	context->impl->GetcProcCopy = context->impl->GetcProc;
	context->impl->ReadProcCopy = context->impl->ReadProc;
	context->impl->SeekProcCopy = context->impl->SeekRProc;
	context->impl->TellProcCopy = context->impl->TellRProc;
	context->impl->iopenCopy = context->impl->iopenr;
	context->impl->icloseCopy = context->impl->icloser;

	// Set the standard procs to read
	ilResetRead(context);

	return;
}


// Restores the read functions - must be used after iPreserveReadFuncs().
void ILAPIENTRY iRestoreReadFuncs(ILcontext* context)
{
	context->impl->GetcProc = context->impl->GetcProcCopy;
	context->impl->ReadProc = context->impl->ReadProcCopy;
	context->impl->SeekRProc = context->impl->SeekProcCopy;
	context->impl->TellRProc = context->impl->TellProcCopy;
	context->impl->iopenr = context->impl->iopenCopy;
	context->impl->icloser = context->impl->icloseCopy;

	return;
}


// Just preserves the current read functions and replaces
//	the current read functions with the default read funcs.
void ILAPIENTRY iPreserveWriteFuncs(ILcontext* context)
{
	// Create backups
	context->impl->PutcProcCopy = context->impl->PutcProc;
	context->impl->SeekWProcCopy = context->impl->SeekWProc;
	context->impl->TellWProcCopy = context->impl->TellWProc;
	context->impl->WriteProcCopy = context->impl->WriteProc;
	context->impl->iopenwCopy = context->impl->iopenw;
	context->impl->iclosewCopy = context->impl->iclosew;

	// Set the standard procs to write
	ilResetWrite(context);

	return;
}


// Restores the read functions - must be used after iPreserveReadFuncs().
void ILAPIENTRY iRestoreWriteFuncs(ILcontext* context)
{
	context->impl->PutcProc = context->impl->PutcProcCopy;
	context->impl->SeekWProc = context->impl->SeekWProcCopy;
	context->impl->TellWProc = context->impl->TellWProcCopy;
	context->impl->WriteProc = context->impl->WriteProcCopy;
	context->impl->iopenw = context->impl->iopenwCopy;
	context->impl->iclosew = context->impl->iclosewCopy;

	return;
}


// Next 7 functions are the default read functions

ILHANDLE ILAPIENTRY iDefaultOpenR(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "rb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen for its UTF8 files.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"rb");
	#else
		return (ILHANDLE)fopen((char*)FileName, "rb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseR(ILHANDLE Handle)
{
	fclose((FILE*)Handle);
	return;
}


ILboolean ILAPIENTRY iDefaultEof(ILcontext* context, ILHANDLE Handle)
{
	ILuint OrigPos, FileSize;

	// Find out the filesize for checking for the end of file
	OrigPos = context->impl->itell(context);
	context->impl->iseek(context, 0, IL_SEEK_END);
	FileSize = context->impl->itell(context);
	context->impl->iseek(context, OrigPos, IL_SEEK_SET);

	if (context->impl->itell(context) >= FileSize)
		return IL_TRUE;
	return IL_FALSE;
}


ILint ILAPIENTRY iDefaultGetc(ILcontext* context, ILHANDLE Handle)
{
	ILint Val;

	if (!context->impl->UseCache) {
		Val = fgetc((FILE*)Handle);
		if (Val == IL_EOF)
			ilSetError(context, IL_FILE_READ_ERROR);
	}
	else {
		Val = 0;
		if (context->impl->iread(context, &Val, 1, 1) != 1)
			return IL_EOF;
	}
	return Val;
}


ILint ILAPIENTRY iDefaultRead(void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle)
{
	return (ILint)fread(Buffer, Size, Number, (FILE*)Handle);
}


ILint ILAPIENTRY iDefaultRSeek(ILHANDLE Handle, ILint Offset, ILint Mode)
{
	return fseek((FILE*)Handle, Offset, Mode);
}


ILint ILAPIENTRY iDefaultWSeek(ILHANDLE Handle, ILint Offset, ILint Mode)
{
	return fseek((FILE*)Handle, Offset, Mode);
}


ILint ILAPIENTRY iDefaultRTell(ILHANDLE Handle)
{
	return ftell((FILE*)Handle);
}


ILint ILAPIENTRY iDefaultWTell(ILHANDLE Handle)
{
	return ftell((FILE*)Handle);
}


ILHANDLE ILAPIENTRY iDefaultOpenW(ILconst_string FileName)
{
#ifndef _UNICODE
	return (ILHANDLE)fopen((char*)FileName, "wb");
#else
	// Windows has a different function, _wfopen, to open UTF16 files,
	//  whereas Linux just uses fopen.
	#ifdef _WIN32
		return (ILHANDLE)_wfopen(FileName, L"wb");
	#else
		return (ILHANDLE)fopen((char*)FileName, "wb");
	#endif
#endif//UNICODE
}


void ILAPIENTRY iDefaultCloseW(ILHANDLE Handle)
{
	fclose((FILE*)Handle);
	return;
}


ILint ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle)
{
	return fputc(Char, (FILE*)Handle);
}


ILint ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle)
{
	return (ILint)fwrite(Buffer, Size, Number, (FILE*)Handle);
}


void ILAPIENTRY ilResetRead(ILcontext* context)
{
	ilSetRead(context, iDefaultOpenR, iDefaultCloseR, iDefaultEof, iDefaultGetc, 
				iDefaultRead, iDefaultRSeek, iDefaultRTell);
	return;
}


void ILAPIENTRY ilResetWrite(ILcontext* context)
{
	ilSetWrite(context, iDefaultOpenW, iDefaultCloseW, iDefaultPutc,
				iDefaultWSeek, iDefaultWTell, iDefaultWrite);
	return;
}


//! Allows you to override the default file-reading functions.
void ILAPIENTRY ilSetRead(ILcontext* context, fOpenRProc Open, fCloseRProc Close, fEofProc Eof, fGetcProc Getc, fReadProc Read, fSeekRProc Seek, fTellRProc Tell)
{
	context->impl->iopenr    = Open;
	context->impl->icloser   = Close;
	context->impl->EofProc   = Eof;
	context->impl->GetcProc  = Getc;
	context->impl->ReadProc  = Read;
	context->impl->SeekRProc = Seek;
	context->impl->TellRProc = Tell;

	return;
}


//! Allows you to override the default file-writing functions.
void ILAPIENTRY ilSetWrite(ILcontext* context, fOpenRProc Open, fCloseRProc Close, fPutcProc Putc, fSeekWProc Seek, fTellWProc Tell, fWriteProc Write)
{
	context->impl->iopenw    = Open;
	context->impl->iclosew   = Close;
	context->impl->PutcProc  = Putc;
	context->impl->WriteProc = Write;
	context->impl->SeekWProc = Seek;
	context->impl->TellWProc = Tell;

	return;
}


// Tells DevIL that we're reading from a file, not a lump
void iSetInputFile(ILcontext* context, ILHANDLE File)
{
	context->impl->ieof  = iEofFile;
	context->impl->igetc = iGetcFile;
	context->impl->iread = iReadFile;
	context->impl->iseek = iSeekRFile;
	context->impl->itell = iTellRFile;
	context->impl->FileRead = File;
	context->impl->ReadFileStart = context->impl->itell(context);
}


// Tells DevIL that we're reading from a lump, not a file
void iSetInputLump(ILcontext* context, const void *Lump, ILuint Size)
{
	context->impl->ieof  = iEofLump;
	context->impl->igetc = iGetcLump;
	context->impl->iread = iReadLump;
	context->impl->iseek = iSeekRLump;
	context->impl->itell = iTellRLump;
	context->impl->ReadLump = Lump;
	context->impl->ReadLumpPos = 0;
	context->impl->ReadLumpSize = Size;
}


// Tells DevIL that we're writing to a file, not a lump
void iSetOutputFile(ILcontext* context, ILHANDLE File)
{
	// Helps with ilGetLumpPos().
	context->impl->WriteLump = NULL;
	context->impl->WriteLumpPos = 0;
	context->impl->WriteLumpSize = 0;

	context->impl->iputc  = iPutcFile;
	context->impl->iseekw = iSeekWFile;
	context->impl->itellw = iTellWFile;
	context->impl->iwrite = iWriteFile;
	
	context->impl->FileWrite = File;
}


// This is only called by ilDetermineSize.  Associates iputc, etc. with
//  "fake" writing functions in il_size.c.
void iSetOutputFake(ILcontext* context)
{
	context->impl->iputc  = iSizePutc;
	context->impl->iseekw = iSizeSeek;
	context->impl->itellw = iSizeTell;
	context->impl->iwrite = iSizeWrite;
	return;
}


// Tells DevIL that we're writing to a lump, not a file
void iSetOutputLump(ILcontext* context, void *Lump, ILuint Size)
{
	// In this case, ilDetermineSize is currently trying to determine the
	//  output buffer size.  It already has the write functions it needs.
	if (Lump == NULL)
		return;

	context->impl->iputc  = iPutcLump;
	context->impl->iseekw = iSeekWLump;
	context->impl->itellw = iTellWLump;
	context->impl->iwrite = iWriteLump;
	context->impl->WriteLump = Lump;
	context->impl->WriteLumpPos = 0;
	context->impl->WriteLumpSize = Size;
}


ILuint ILAPIENTRY ilGetLumpPos(ILcontext* context)
{
	if (context->impl->WriteLump)
		return context->impl->WriteLumpPos;
	return 0;
}


ILuint ILAPIENTRY ilprintf(ILcontext* context, const char *Line, ...)
{
	char	Buffer[2048];  // Hope this is large enough
	va_list	VaLine;
	ILuint	i;

	va_start(VaLine, Line);
	vsprintf(Buffer, Line, VaLine);
	va_end(VaLine);

	i = ilCharStrLen(Buffer);
	context->impl->iwrite(context, Buffer, 1, i);

	return i;
}


// To pad zeros where needed...
void ipad(ILcontext* context, ILuint NumZeros)
{
	ILuint i = 0;
	for (; i < NumZeros; i++)
		context->impl->iputc(context, 0);
	return;
}


//
// The rest of the functions following in this file are quite
//	self-explanatory, except where commented.
//

// Next 12 functions are the default write functions

ILboolean ILAPIENTRY iEofFile(ILcontext* context)
{
	return context->impl->EofProc(context, (FILE*)context->impl->FileRead);
}


ILboolean ILAPIENTRY iEofLump(ILcontext* context)
{
	if (context->impl->ReadLumpSize)
		return (context->impl->ReadLumpPos >= context->impl->ReadLumpSize);
	return IL_FALSE;
}


ILint ILAPIENTRY iGetcFile(ILcontext* context)
{
	if (!context->impl->UseCache) {
		return context->impl->GetcProc(context, context->impl->FileRead);
	}
	if (context->impl->CachePos >= context->impl->CacheSize) {
		iPreCache(context, context->impl->CacheSize);
	}

	context->impl->CacheBytesRead++;
	return context->impl->Cache[context->impl->CachePos++];
}


ILint ILAPIENTRY iGetcLump(ILcontext* context)
{
	// If ReadLumpSize is 0, don't even check to see if we've gone past the bounds.
	if (context->impl->ReadLumpSize > 0) {
		if (context->impl->ReadLumpPos + 1 > context->impl->ReadLumpSize) {
			context->impl->ReadLumpPos--;
			ilSetError(context, IL_FILE_READ_ERROR);
			return IL_EOF;
		}
	}

	return *((ILubyte*)context->impl->ReadLump + context->impl->ReadLumpPos++);
}


ILuint ILAPIENTRY iReadFile(ILcontext* context, void *Buffer, ILuint Size, ILuint Number)
{
	ILuint	TotalBytes = 0, BytesCopied;
	ILuint	BuffSize = Size * Number;
	ILuint	NumRead;

	if (!context->impl->UseCache) {
		NumRead = context->impl->ReadProc(Buffer, Size, Number, context->impl->FileRead);
		if (NumRead != Number)
			ilSetError(context, IL_FILE_READ_ERROR);
		return NumRead;
	}

	/*if (Cache == NULL || CacheSize == 0) {  // Shouldn't happen, but we check anyway.
		return ReadProc(Buffer, Size, Number, FileRead);
	}*/

	if (BuffSize < context->impl->CacheSize - context->impl->CachePos) {
		memcpy(Buffer, context->impl->Cache + context->impl->CachePos, BuffSize);
		context->impl->CachePos += BuffSize;
		context->impl->CacheBytesRead += BuffSize;
		if (Size != 0)
			BuffSize /= Size;
		return BuffSize;
	}
	else {
		while (TotalBytes < BuffSize) {
			// If loop through more than once, after first, CachePos is 0.
			if (TotalBytes + context->impl->CacheSize - context->impl->CachePos > BuffSize)
				BytesCopied = BuffSize - TotalBytes;
			else
				BytesCopied = context->impl->CacheSize - context->impl->CachePos;

			memcpy((ILubyte*)Buffer + TotalBytes, context->impl->Cache + context->impl->CachePos, BytesCopied);
			TotalBytes += BytesCopied;
			context->impl->CachePos += BytesCopied;
			if (TotalBytes < BuffSize) {
				iPreCache(context, context->impl->CacheSize);
			}
		}
	}

	// DW: Changed on 12-27-2008.  Was causing the position to go too far if the
	//     cache was smaller than the buffer.
	//CacheBytesRead += TotalBytes;
	context->impl->CacheBytesRead = context->impl->CachePos;
	if (Size != 0)
		TotalBytes /= Size;
	if (TotalBytes != Number)
		ilSetError(context, IL_FILE_READ_ERROR);
	return TotalBytes;
}


ILuint ILAPIENTRY iReadLump(ILcontext* context, void *Buffer, const ILuint Size, const ILuint Number)
{
	ILuint i, ByteSize = IL_MIN( Size*Number, context->impl->ReadLumpSize - context->impl->ReadLumpPos);

	for (i = 0; i < ByteSize; i++) {
		*((ILubyte*)Buffer + i) = *((ILubyte*)context->impl->ReadLump + context->impl->ReadLumpPos + i);
		if (context->impl->ReadLumpSize > 0) {  // ReadLumpSize is too large to care about apparently
			if (context->impl->ReadLumpPos + i > context->impl->ReadLumpSize) {
				context->impl->ReadLumpPos += i;
				if (i != Number)
					ilSetError(context, IL_FILE_READ_ERROR);
				return i;
			}
		}
	}

	context->impl->ReadLumpPos += i;
	if (Size != 0)
		i /= Size;
	if (i != Number)
		ilSetError(context, IL_FILE_READ_ERROR);
	return i;
}


ILboolean iPreCache(ILcontext* context, ILuint Size)
{
	// Reading from a memory lump, so don't cache.
	if (context->impl->iread == iReadLump) {
		//iUnCache(context);  // DW: Removed 06-10-2002.
		return IL_TRUE;
	}

	if (context->impl->Cache) {
		ifree(context->impl->Cache);
	}

	if (Size == 0) {
		Size = 1;
	}

	context->impl->Cache = (ILubyte*)ialloc(context, Size);
	if (context->impl->Cache == NULL) {
		return IL_FALSE;
	}

	context->impl->UseCache = IL_FALSE;
	context->impl->CacheStartPos = context->impl->itell(context);
	context->impl->CacheSize = context->impl->iread(context, context->impl->Cache, 1, Size);
	if (context->impl->CacheSize != Size)
		ilGetError(context);  // Get rid of the IL_FILE_READ_ERROR.

	//2003-09-09: uncommented the following line to prevent
	//an infinite loop in ilPreCache()
	context->impl->CacheSize = Size;
	context->impl->CachePos = 0;
	context->impl->UseCache = IL_TRUE;
	context->impl->CacheBytesRead = 0;

	return IL_TRUE;
}


void iUnCache(ILcontext* context)
{
	//changed 2003-09-01:
	//make iUnCache smart enough to return if
	//no cache is used
	if (!context->impl->UseCache)
		return;

	if (context->impl->iread == iReadLump)
		return;

	context->impl->CacheSize = 0;
	context->impl->CachePos = 0;
	if (context->impl->Cache) {
		ifree(context->impl->Cache);
		context->impl->Cache = NULL;
	}
	context->impl->UseCache = IL_FALSE;

	context->impl->iseek(context, context->impl->CacheStartPos + context->impl->CacheBytesRead, IL_SEEK_SET);

	return;
}


ILint ILAPIENTRY iSeekRFile(ILcontext* context, ILint Offset, ILuint Mode)
{
	if (Mode == IL_SEEK_SET)
		Offset += context->impl->ReadFileStart;  // This allows us to use IL_SEEK_SET in the middle of a file.
	return context->impl->SeekRProc(context->impl->FileRead, Offset, Mode);
}


// Returns 1 on error, 0 on success
ILint ILAPIENTRY iSeekRLump(ILcontext* context, ILint Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			if (Offset > (ILint)context->impl->ReadLumpSize)
				return 1;
			context->impl->ReadLumpPos = Offset;
			break;

		case IL_SEEK_CUR:
			if (context->impl->ReadLumpPos + Offset > context->impl->ReadLumpSize)
				return 1;
			context->impl->ReadLumpPos += Offset;
			break;

		case IL_SEEK_END:
			if (Offset > 0)
				return 1;
			// Should we use >= instead?
			if (abs(Offset) > (ILint)context->impl->ReadLumpSize)  // If ReadLumpSize == 0, too bad
				return 1;
			context->impl->ReadLumpPos = context->impl->ReadLumpSize + Offset;
			break;

		default:
			return 1;
	}

	return 0;
}


ILuint ILAPIENTRY iTellRFile(ILcontext* context)
{
	return context->impl->TellRProc(context->impl->FileRead);
}


ILuint ILAPIENTRY iTellRLump(ILcontext* context)
{
	return context->impl->ReadLumpPos;
}


ILHANDLE ILAPIENTRY iGetFile(ILcontext* context)
{
	return context->impl->FileRead;
}


const ILubyte* ILAPIENTRY iGetLump(ILcontext* context) {
	return (ILubyte*)context->impl->ReadLump;
}



// Next 4 functions are the default write functions

ILint ILAPIENTRY iPutcFile(ILcontext* context, ILubyte Char)
{
	return context->impl->PutcProc(Char, context->impl->FileWrite);
}


ILint ILAPIENTRY iPutcLump(ILcontext* context, ILubyte Char)
{
	if (context->impl->WriteLumpPos >= context->impl->WriteLumpSize)
		return IL_EOF;  // IL_EOF
	*((ILubyte*)(context->impl->WriteLump) +context->impl->WriteLumpPos++) = Char;
	return Char;
}


ILint ILAPIENTRY iWriteFile(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number)
{
	ILuint NumWritten;
	NumWritten = context->impl->WriteProc(Buffer, Size, Number, context->impl->FileWrite);
	if (NumWritten != Number) {
		ilSetError(context, IL_FILE_WRITE_ERROR);
		return 0;
	}
	return NumWritten;
}


ILint ILAPIENTRY iWriteLump(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number)
{
	ILuint SizeBytes = Size * Number;
	ILuint i = 0;

	for (; i < SizeBytes; i++) {
		if (context->impl->WriteLumpSize > 0) {
			if (context->impl->WriteLumpPos + i >= context->impl->WriteLumpSize) {  // Should we use > instead?
				ilSetError(context, IL_FILE_WRITE_ERROR);
				context->impl->WriteLumpPos += i;
				return i;
			}
		}

		*((ILubyte*)context->impl->WriteLump + context->impl->WriteLumpPos + i) = *((ILubyte*)Buffer + i);
	}

	context->impl->WriteLumpPos += SizeBytes;
	
	return SizeBytes;
}


ILint ILAPIENTRY iSeekWFile(ILcontext* context, ILint Offset, ILuint Mode)
{
	if (Mode == IL_SEEK_SET)
		Offset += context->impl->WriteFileStart;  // This allows us to use IL_SEEK_SET in the middle of a file.
	return context->impl->SeekWProc(context->impl->FileWrite, Offset, Mode);
}


// Returns 1 on error, 0 on success
ILint ILAPIENTRY iSeekWLump(ILcontext* context, ILint Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			if (Offset > (ILint)context->impl->WriteLumpSize)
				return 1;
			context->impl->WriteLumpPos = Offset;
			break;

		case IL_SEEK_CUR:
			if (context->impl->WriteLumpPos + Offset > context->impl->WriteLumpSize)
				return 1;
			context->impl->WriteLumpPos += Offset;
			break;

		case IL_SEEK_END:
			if (Offset > 0)
				return 1;
			// Should we use >= instead?
			if (abs(Offset) > (ILint)context->impl->WriteLumpSize)  // If WriteLumpSize == 0, too bad
				return 1;
			context->impl->WriteLumpPos = context->impl->WriteLumpSize + Offset;
			break;

		default:
			return 1;
	}

	return 0;
}


ILuint ILAPIENTRY iTellWFile(ILcontext* context)
{
	return context->impl->TellWProc(context->impl->FileWrite);
}


ILuint ILAPIENTRY iTellWLump(ILcontext* context)
{
	return context->impl->WriteLumpPos;
}
