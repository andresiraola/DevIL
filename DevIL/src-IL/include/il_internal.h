//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/01/2009
//
// Filename: src-IL/include/il_internal.h
//
// Description: Internal stuff for DevIL
//
//-----------------------------------------------------------------------------
#ifndef INTERNAL_H
#define INTERNAL_H
#define _IL_BUILD_LIBRARY

// config.h is auto-generated
#include "config.h"

#if defined(__GNUC__) && __STDC_VERSION__ >= 199901L
    // this makes various common-but-not-C99 functions visable in gcc -std-c99
    // most notably, strdup, strcasecmp().
    #define _GNU_SOURCE
#endif

// Standard headers
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <IL/il.h>
#include <IL/devil_internal_exports.h>
#include "il_files.h"
#include "il_endian.h"
#include "il_manip.h"
#include "il_context_impl.h"

// If we do not want support for game image formats, this define removes them all.
#ifdef IL_NO_GAMES
	#define IL_NO_BLP
	#define IL_NO_DOOM
	#define IL_NO_FTX
	#define IL_NO_IWI
	#define IL_NO_LIF
	#define IL_NO_MDL
	#define IL_NO_ROT
	#define IL_NO_TPL
	#define IL_NO_UTX
	#define IL_NO_WAL
#endif//IL_NO_GAMES

// If we want to compile without support for formats supported by external libraries,
//  this define will remove them all.
#ifdef IL_NO_EXTLIBS
	#define IL_NO_EXR
	#define IL_NO_JP2
	#define IL_NO_JPG
	#define IL_NO_LCMS
	#define IL_NO_MNG
	#define IL_NO_PNG
	#define IL_NO_TIF
	#define IL_NO_WDP
	#undef IL_USE_DXTC_NVIDIA
	#undef IL_USE_DXTC_SQUISH
#endif//IL_NO_EXTLIBS

// Windows-specific
#ifdef _WIN32
	#ifdef _MSC_VER
		#if _MSC_VER > 1000
			#pragma once
			#pragma intrinsic(memcpy)
			#pragma intrinsic(memset)
			#pragma intrinsic(strcmp)
			#pragma intrinsic(strlen)
			#pragma intrinsic(strcpy)
			
			#if _MSC_VER >= 1300
				#pragma warning(disable : 4996)  // MSVC++ 8/9 deprecation warnings
			#endif
		#endif // _MSC_VER > 1000
	#endif
	#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
	#include <windows.h>
#endif//_WIN32

#ifdef _UNICODE
	#define IL_TEXT(s) L##s
	#ifndef _WIN32  // At least in Linux, fopen works fine, and wcsicmp is not defined.
		#define wcsicmp wcsncasecmp
		#define _wcsicmp wcsncasecmp
		#define _wfopen fopen
	#endif
	#define iStrCpy wcscpy
#else
	#define IL_TEXT(s) (s)
	#define iStrCpy strcpy
#endif

#ifdef IL_INLINE_ASM
	#if (defined (_MSC_VER) && defined(_WIN32))  // MSVC++ only
		#define USE_WIN32_ASM
	#endif

	#ifdef _WIN64
		#undef USE_WIN32_ASM
	//@TODO: Windows 64 compiler cannot use inline ASM, so we need to
	//  generate some MASM code at some point.
	#endif

	#ifdef _WIN32_WCE  // Cannot use our inline ASM in Windows Mobile.
		#undef USE_WIN32_ASM
	#endif
#endif

#define BIT_0	0x00000001
#define BIT_1	0x00000002
#define BIT_2	0x00000004
#define BIT_3	0x00000008
#define BIT_4	0x00000010
#define BIT_5	0x00000020
#define BIT_6	0x00000040
#define BIT_7	0x00000080
#define BIT_8	0x00000100
#define BIT_9	0x00000200
#define BIT_10	0x00000400
#define BIT_11	0x00000800
#define BIT_12	0x00001000
#define BIT_13	0x00002000
#define BIT_14	0x00004000
#define BIT_15	0x00008000
#define BIT_16	0x00010000
#define BIT_17	0x00020000
#define BIT_18	0x00040000
#define BIT_19	0x00080000
#define BIT_20	0x00100000
#define BIT_21	0x00200000
#define BIT_22	0x00400000
#define BIT_23	0x00800000
#define BIT_24	0x01000000
#define BIT_25	0x02000000
#define BIT_26	0x04000000
#define BIT_27	0x08000000
#define BIT_28	0x10000000
#define BIT_29	0x20000000
#define BIT_30	0x40000000
#define BIT_31	0x80000000
#define NUL '\0'  // Easier to type and ?portable?
#if !_WIN32 || _WIN32_WCE
	int stricmp(const char *src1, const char *src2);
	int strnicmp(const char *src1, const char *src2, size_t max);
#endif//_WIN32
#ifdef _WIN32_WCE
	char *strdup(const char *src);
#endif
int iStrCmp(ILconst_string src1, ILconst_string src2);

//
// Some math functions
//
// A fast integer squareroot, completely accurate for x < 289.
// Taken from http://atoms.org.uk/sqrt/
// There is also a version that is accurate for all integers
// < 2^31, if we should need it²
int iSqrt(int x);
//
// Useful miscellaneous functions
//
ILboolean	iCheckExtension(ILconst_string Arg, ILconst_string Ext);
ILbyte*		iFgets(ILcontext* context, char *buffer, ILuint maxlen);
ILboolean	iFileExists(ILconst_string FileName);
ILstring	iGetExtension(ILconst_string FileName);
ILstring	ilStrDup(ILcontext* context, ILconst_string Str);
ILuint		ilStrLen(ILconst_string Str);
ILuint		ilCharStrLen(const char *Str);
// Miscellaneous functions
void					ilDefaultStates(ILcontext* context);
ILenum					iGetHint(ILcontext* context, ILenum Target);
ILint					iGetInt(ILcontext* context, ILenum Mode);
void					ilRemoveRegistered(void);
ILAPI void ILAPIENTRY	ilSetCurImage(ILcontext* context, ILimage *Image);
ILuint					ilDetermineSize(ILcontext* context, ILenum Type);
//
// Rle compression
//
#define		IL_TGACOMP 0x01
#define		IL_PCXCOMP 0x02
#define		IL_SGICOMP 0x03
#define     IL_BMPCOMP 0x04
ILboolean	ilRleCompressLine(ILcontext* context, ILubyte *ScanLine, ILuint Width, ILubyte Bpp, ILubyte *Dest, ILuint *DestWidth, ILenum CompressMode);
ILuint		ilRleCompress(ILcontext* context, ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILubyte Bpp, ILubyte *Dest, ILenum CompressMode, ILuint *ScanTable);
void		iSetImage0(ILcontext* context);
// DXTC compression
ILuint			ilNVidiaCompressDXTFile(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtType);
ILAPI ILubyte*	ILAPIENTRY ilNVidiaCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);
ILAPI ILubyte*	ILAPIENTRY ilSquishCompressDXT(ILubyte *Data, ILuint Width, ILuint Height, ILuint Depth, ILenum DxtFormat, ILuint *DxtSize);

// Conversion functions
ILboolean	ilAddAlpha(ILcontext* context);
ILboolean	ilAddAlphaKey(ILcontext* context, ILimage *Image);
ILboolean	iFastConvert(ILcontext* context, ILenum DestFormat);
ILboolean	ilFixCur(ILcontext* context);
ILboolean	ilFixImage(ILcontext* context);
ILboolean	ilRemoveAlpha(ILcontext* context);
ILboolean	ilSwapColours(ILcontext* context);
// Palette functions
ILboolean	iCopyPalette(ILcontext* context, ILpal *Dest, ILpal *Src);
// Miscellaneous functions
char*		iGetString(ILcontext* context, ILenum StringName);  // Internal version of ilGetString

//
// Image loading/saving functions
//
ILboolean ilIsValidBlp(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidBlpF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidBlpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadBlp(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadBlpF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadBlpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidBmp(ILcontext* context, ILconst_string CONST_RESTRICT FileName);
ILboolean ilIsValidBmpF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidBmpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadBmp(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadBmpF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadBmpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveBmp(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveBmpF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveBmpL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilSaveCHeader(ILcontext* context, ILconst_string FileName, char *InternalName);
ILboolean ilLoadCut(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadCutF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadCutL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidDcx(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidDcxF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidDcxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDcx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDcxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDcxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidDds(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidDdsF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidDdsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDds(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDdsF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDdsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveDds(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveDdsF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveDdsL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidDicom(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidDicomF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidDicomL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDicom(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDicomF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDicomL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDoom(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDoomF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDoomL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDoomFlat(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDoomFlatF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDoomFlatL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidDpx(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidDpxF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidDpxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadDpx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadDpxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadDpxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidExr(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidExrF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidExrL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadExr(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadExrF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadExrL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveExr(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveExrF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveExrL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidFits(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidFitsF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidFitsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadFits(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadFitsF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadFitsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadFtx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadFtxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadFtxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidGif(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidGifF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidGifL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadGif(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadGifF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadGifL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidHdr(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidHdrF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidHdrL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadHdr(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadHdrF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadHdrL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveHdr(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveHdrF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveHdrL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilLoadIcon(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadIconF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadIconL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidIcns(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidIcnsF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidIcnsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadIcns(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadIcnsF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadIcnsL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadIff(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadIffF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadIffL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidIlbm(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidIlbmF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidIlbmL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadIlbm(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadIlbmF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadIlbmL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidIwi(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidIwiF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidIwiL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadIwi(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadIwiF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadIwiL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidJp2(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidJp2F(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidJp2L(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadJp2(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadJp2F(ILcontext* context, ILHANDLE File);
ILboolean ilLoadJp2L(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadJp2LInternal(ILcontext* context, const void *Lump, ILuint Size, ILimage *Image);
ILboolean ilSaveJp2(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveJp2F(ILcontext* context, ILHANDLE File);
ILuint    ilSaveJp2L(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidJpeg(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidJpegF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidJpegL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadJpeg(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadJpegF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadJpegL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveJpeg(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveJpegF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveJpegL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidKtx(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidKtxF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidKtxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadKtx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadKtxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadKtxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidLif(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidLifF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidLifL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadLif(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadLifF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadLifL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidMdl(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidMdlF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidMdlL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadMdl(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadMdlF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadMdlL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadMng(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadMngF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadMngL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveMng(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveMngF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveMngL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidMp3(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidMp3F(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidMp3L(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadMp3(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadMp3F(ILcontext* context, ILHANDLE File);
ILboolean ilLoadMp3L(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPcd(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPcdF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPcdL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidPcx(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPcxF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPcxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPcx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPcxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPcxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSavePcx(ILcontext* context, ILconst_string FileName);
ILuint    ilSavePcxF(ILcontext* context, ILHANDLE File);
ILuint    ilSavePcxL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidPic(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPicF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPicL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPic(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPicF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPicL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPix(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPixF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPixL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidPng(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPngF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPngL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPng(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPngF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPngL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSavePng(ILcontext* context, ILconst_string FileName);
ILuint    ilSavePngF(ILcontext* context, ILHANDLE File);
ILuint    ilSavePngL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidPnm(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPnmF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPnmL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPnm(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPnmF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPnmL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSavePnm(ILcontext* context, ILconst_string FileName);
ILuint    ilSavePnmF(ILcontext* context, ILHANDLE File);
ILuint    ilSavePnmL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidPsd(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPsdF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPsdL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPsd(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPsdF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPsdL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSavePsd(ILcontext* context, ILconst_string FileName);
ILuint    ilSavePsdF(ILcontext* context, ILHANDLE File);
ILuint    ilSavePsdL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidPsp(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidPspF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidPspL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPsp(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPspF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPspL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadPxr(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadPxrF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadPxrL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadRaw(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadRawF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadRawL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveRaw(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveRawF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveRawL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidRot(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidRotF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidRotL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadRot(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadRotF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadRotL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidScitex(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidScitexF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidScitexL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadScitex(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadScitexF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadScitexL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidSgi(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidSgiF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidSgiL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadSgi(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadSgiF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadSgiL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveSgi(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveSgiF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveSgiL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidSun(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidSunF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidSunL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadSun(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadSunF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadSunL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidTga(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidTgaF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidTgaL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadTarga(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadTargaF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadTargaL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveTarga(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveTargaF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveTargaL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilLoadTexture(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadTextureF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadTextureL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidTiff(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidTiffF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidTiffL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadTiff(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadTiffF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadTiffL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveTiff(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveTiffF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveTiffL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidTpl(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidTplF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidTplL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadTpl(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadTplF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadTplL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadUtx(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadUtxF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadUtxL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidVtf(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidVtfF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidVtfL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadVtf(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadVtfF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadVtfL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveVtf(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveVtfF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveVtfL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilLoadWal(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadWalF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadWalL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadWbmp(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadWbmpF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadWbmpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilSaveWbmp(ILcontext* context, ILconst_string FileName);
ILuint    ilSaveWbmpF(ILcontext* context, ILHANDLE File);
ILuint    ilSaveWbmpL(ILcontext* context, void *Lump, ILuint Size);
ILboolean ilIsValidWdp(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidWdpF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidWdpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadWdp(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadWdpF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadWdpL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilIsValidXpm(ILcontext* context, ILconst_string FileName);
ILboolean ilIsValidXpmF(ILcontext* context, ILHANDLE File);
ILboolean ilIsValidXpmL(ILcontext* context, const void *Lump, ILuint Size);
ILboolean ilLoadXpm(ILcontext* context, ILconst_string FileName);
ILboolean ilLoadXpmF(ILcontext* context, ILHANDLE File);
ILboolean ilLoadXpmL(ILcontext* context, const void *Lump, ILuint Size);


// OpenEXR is written in C++, so we have to wrap this to avoid linker errors.
/*#ifndef IL_NO_EXR
	#ifdef __cplusplus
	extern "C" {
	#endif
		ILboolean ilLoadExr(ILcontext* context, ILconst_string FileName);
	#ifdef __cplusplus
	}
	#endif
#endif*/

//ILboolean ilLoadExr(ILcontext* context, ILconst_string FileName);


#ifdef __cplusplus
}
#endif

#endif//INTERNAL_H
