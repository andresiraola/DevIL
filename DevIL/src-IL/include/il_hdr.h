//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2004 by Denton Woods (this file by thakis)
// Last modified: 06/09/2004
//
// Filename: src-IL/include/il_hdr.h
//
// Description: Reads a RADIANCE High Dynamic Range Image
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

#ifdef _WIN32
#pragma pack(push, gif_struct, 1)
#endif

typedef struct HDRHEADER
{
	char Signature[10]; //must be "#?RADIANCE"
	ILuint Width, Height;
} IL_PACKSTRUCT HDRHEADER;

#ifdef _WIN32
#pragma pack(pop, gif_struct)
#endif

class HdrHandler
{
protected:
	ILcontext* context;

	ILboolean	check(HDRHEADER *Header);

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

	void		ReadScanline(ILubyte *scanline, ILuint w);

public:
	HdrHandler(ILcontext* context);

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