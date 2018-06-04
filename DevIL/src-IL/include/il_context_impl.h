#pragma once

#include <setjmp.h>

#include <IL/il_context.h>

#include "il_states.h"

// Just a guess...seems large enough
#define I_STACK_INCREMENT 1024

typedef struct iFree
{
	ILuint	Name;
	void	*Next;
} iFree;

class ILcontext::Impl
{
public:
	ILenum		ilErrorNum[IL_ERROR_STACK_SIZE];
	ILint		ilErrorPlace = (-1);

	IL_HINTS	ilHints;
	IL_STATES	ilStates[IL_ATTRIB_STACK_MAX];
	ILuint		ilCurrentPos = 0;  // Which position on the stack

	ILuint		CurName = 0;

	ILuint		StackSize = 0;
	ILuint		LastUsed = 0;

	// two fixed slots - slot 0 is default (fallback) image, slot 1 is temp image
	ILimage**	ImageStack = NULL;

	iFree*		FreeNames = NULL;
	ILboolean	OnExit = IL_FALSE;
	ILboolean	ParentImage = IL_TRUE;

	const void*	ReadLump = NULL;
	void*		WriteLump = NULL;

	ILHANDLE	FileRead = NULL, FileWrite = NULL;

	ILuint		ReadLumpPos = 0, ReadLumpSize = 0, ReadFileStart = 0, WriteFileStart = 0;
	ILuint		WriteLumpPos = 0, WriteLumpSize = 0;

	ILuint CurPos;  // Fake "file" pointer.
	ILuint MaxPos;

	ILboolean	UseCache = IL_FALSE;
	ILubyte*	Cache = NULL;
	ILuint		CacheSize, CachePos, CacheStartPos, CacheBytesRead;

	void		(ILAPIENTRY *iclosew)(ILHANDLE);
	ILHANDLE	(ILAPIENTRY *iopenw)(ILconst_string);
	ILint		(ILAPIENTRY *iputc)(ILcontext* context, ILubyte Char);
	ILint		(ILAPIENTRY *iseekw)(ILcontext* context, ILint Offset, ILuint Mode);
	ILuint		(ILAPIENTRY *itellw)(ILcontext* context);
	ILint		(ILAPIENTRY *iwrite)(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number);

	ILboolean	(ILAPIENTRY *ieof)(ILcontext* context);
	ILHANDLE	(ILAPIENTRY *iopenr)(ILconst_string);
	void		(ILAPIENTRY *icloser)(ILHANDLE);
	ILint		(ILAPIENTRY *igetc)(ILcontext* context);
	ILuint		(ILAPIENTRY *iread)(ILcontext* context, void *Buffer, ILuint Size, ILuint Number);
	ILint		(ILAPIENTRY *iseek)(ILcontext* context, ILint Offset, ILuint Mode);
	ILuint		(ILAPIENTRY *itell)(ILcontext* context);

	fEofProc	EofProc;
	fGetcProc	GetcProc;
	fReadProc	ReadProc;
	fSeekRProc	SeekRProc;
	fSeekWProc	SeekWProc;
	fTellRProc	TellRProc;
	fTellWProc	TellWProc;
	fPutcProc	PutcProc;
	fWriteProc	WriteProc;

	ILHANDLE	(ILAPIENTRY *iopenCopy)(ILconst_string);
	void		(ILAPIENTRY *icloseCopy)(ILHANDLE);

	ILHANDLE	(ILAPIENTRY *iopenwCopy)(ILconst_string);
	void		(ILAPIENTRY *iclosewCopy)(ILHANDLE);

	fGetcProc	GetcProcCopy;
	fReadProc	ReadProcCopy;
	fSeekRProc	SeekProcCopy;
	fTellRProc	TellProcCopy;

	fPutcProc	PutcProcCopy;
	fSeekWProc	SeekWProcCopy;
	fTellWProc	TellWProcCopy;
	fWriteProc	WriteProcCopy;

	ILimage*	iCurImage;

	jmp_buf		jumpBuffer;
};