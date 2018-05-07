//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pcx.h
//
// Description: Reads and writes from/to a .pcx file.
//
//-----------------------------------------------------------------------------


#ifndef PCX_H
#define PCX_H

#include "il_internal.h"


#ifdef _WIN32
#pragma pack(push, packed_struct, 1)
#endif
typedef struct PCXHEAD
{
	ILubyte		Manufacturer;
	ILubyte		Version;
	ILubyte		Encoding;
	ILubyte		Bpp;
	ILushort	Xmin, Ymin, Xmax, Ymax;
	ILushort	HDpi;
	ILushort	VDpi;
	ILubyte		ColMap[48];
	ILubyte		Reserved;
	ILubyte		NumPlanes;
	ILushort	Bps;
	ILushort	PaletteInfo;
	ILushort	HScreenSize;
	ILushort	VScreenSize;
	ILubyte		Filler[54];
} IL_PACKSTRUCT PCXHEAD;
#ifdef _WIN32
#pragma pack(pop, packed_struct)
#endif

// For checking and reading
ILboolean iIsValidPcx(ILcontext* context);
ILboolean iCheckPcx(PCXHEAD *Header);
ILboolean iLoadPcxInternal(ILcontext* context);
ILboolean iSavePcxInternal(ILcontext* context);
ILboolean iUncompressPcx(ILcontext* context, PCXHEAD *Header);
ILboolean iUncompressSmall(ILcontext* context, PCXHEAD *Header);

// For writing
ILuint encput(ILcontext* context, ILubyte byt, ILubyte cnt);
ILuint encLine(ILcontext* context, ILubyte *inBuff, ILint inLen, ILubyte Stride);


#endif//PCX_H
