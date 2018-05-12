//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 09/01/2003 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_bmp.h
//
// Description: Reads and writes to a bitmap (.bmp) file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

#ifdef _WIN32
#pragma pack(push, bmp_struct, 1)
#endif

typedef struct BMPHEAD {
	ILushort	bfType;
	ILint		bfSize;
	ILuint		bfReserved;
	ILint		bfDataOff;
	ILint		biSize;
	ILint		biWidth;
	ILint		biHeight;
	ILshort		biPlanes;
	ILshort		biBitCount;
	ILint		biCompression;
	ILint		biSizeImage;
	ILint		biXPelsPerMeter;
	ILint		biYPelsPerMeter;
	ILint		biClrUsed;
	ILint		biClrImportant;
} IL_PACKSTRUCT BMPHEAD;

typedef struct OS2_HEAD
{
	// Bitmap file header.
	ILushort	bfType;
	ILuint		biSize;
	ILshort		xHotspot;
	ILshort		yHotspot;
	ILuint		DataOff;

	// Bitmap core header.
	ILuint		cbFix;
	//2003-09-01: changed cx, cy to ushort according to MSDN
	ILushort		cx;
	ILushort		cy;
	ILushort	cPlanes;
	ILushort	cBitCount;
} IL_PACKSTRUCT OS2_HEAD;

#ifdef _WIN32
#pragma pack(pop, bmp_struct)
#endif

// Internal functions
ILboolean	iGetBmpHead(ILcontext* context, BMPHEAD * const Header);
ILboolean	iGetOS2Head(ILcontext* context, OS2_HEAD * const Header);
ILboolean	iCheckOS2(const OS2_HEAD *CONST_RESTRICT Header);
ILboolean	ilReadUncompBmp(ILcontext* context, BMPHEAD *Info);
ILboolean	ilReadRLE8Bmp(ILcontext* context, BMPHEAD *Info);
ILboolean	ilReadRLE4Bmp(ILcontext* context, BMPHEAD *Info);
ILboolean	iGetOS2Bmp(ILcontext* context, OS2_HEAD *Header);

#ifdef IL_BMP_C
#undef NOINLINE
#undef INLINE
#define INLINE
#endif

class BmpHandler
{
protected:
	ILcontext * context;

	ILboolean	check(const BMPHEAD *CONST_RESTRICT Header);

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	BmpHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);

	ILboolean	save(ILconst_string FileName);
	ILuint		saveF(ILHANDLE File);
	ILuint		saveL(void *Lump, ILuint Size);
};