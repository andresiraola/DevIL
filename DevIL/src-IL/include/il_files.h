//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/include/il_files.h
//
// Description: File handling for DevIL
//
//-----------------------------------------------------------------------------

#ifndef FILES_H
#define FILES_H

#if defined (__FILES_C)
#define __FILES_EXTERN
#else
#define __FILES_EXTERN extern
#endif
#include <IL/il.h>


__FILES_EXTERN void ILAPIENTRY iPreserveReadFuncs(void);
__FILES_EXTERN void ILAPIENTRY iRestoreReadFuncs(void);
__FILES_EXTERN void ILAPIENTRY iPreserveWriteFuncs(void);
__FILES_EXTERN void ILAPIENTRY iRestoreWriteFuncs(void);

__FILES_EXTERN ILHANDLE			ILAPIENTRY iDefaultOpen(ILconst_string FileName);
__FILES_EXTERN void		        ILAPIENTRY iDefaultClose(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultGetc(ILcontext* context, ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultRead(void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultSeekR(ILHANDLE Handle, ILint Offset, ILint Mode);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultSeekW(ILHANDLE Handle, ILint Offset, ILint Mode);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultTellR(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultTellW(ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultPutc(ILubyte Char, ILHANDLE Handle);
__FILES_EXTERN ILint			ILAPIENTRY iDefaultWrite(const void *Buffer, ILuint Size, ILuint Number, ILHANDLE Handle);

__FILES_EXTERN void				iSetInputFile(ILcontext* context, ILHANDLE File);
__FILES_EXTERN void				iSetInputLump(ILcontext* context, const void *Lump, ILuint Size);

__FILES_EXTERN void				iSetOutputFile(ILcontext* context, ILHANDLE File);
__FILES_EXTERN void				iSetOutputLump(ILcontext* context, void *Lump, ILuint Size);
__FILES_EXTERN void				iSetOutputFake(ILcontext* context);
 
__FILES_EXTERN ILHANDLE			ILAPIENTRY iGetFile(ILcontext* context);
__FILES_EXTERN const ILubyte*	ILAPIENTRY iGetLump(ILcontext* context);

__FILES_EXTERN ILuint			ILAPIENTRY ilprintf(ILcontext* context, const char *, ...);
__FILES_EXTERN void				ipad(ILcontext* context, ILuint NumZeros);

__FILES_EXTERN ILboolean		iPreCache(ILcontext* context, ILuint Size);
__FILES_EXTERN void				iUnCache(ILcontext* context);

#endif//FILES_H
