//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/include/il_gif.h
//
// Description: Reads from a Graphics Interchange Format (.gif) file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

#define GIF87A 87
#define GIF89A 89

#ifdef _WIN32
	#pragma pack(push, gif_struct, 1)
#endif

typedef struct GIFHEAD
{
	char		Sig[6];
	ILushort	Width;
	ILushort	Height;
	ILubyte		ColourInfo;
	ILubyte		Background;
	ILubyte		Aspect;
} IL_PACKSTRUCT GIFHEAD;

typedef struct IMAGEDESC
{
	ILubyte		Separator;
	ILushort	OffX;
	ILushort	OffY;
	ILushort	Width;
	ILushort	Height;
	ILubyte		ImageInfo;
} IL_PACKSTRUCT IMAGEDESC;

typedef struct GFXCONTROL
{
	ILubyte		Size;
	ILubyte		Packed;
	ILushort	Delay;
	ILubyte		Transparent;
	ILubyte		Terminator;
	ILboolean	Used; //this stores if a gfxcontrol was read - it is IL_FALSE (!)

			//if a gfxcontrol was read from the file, IL_TRUE otherwise
} IL_PACKSTRUCT GFXCONTROL;

#ifdef _WIN32
	#pragma pack(pop, gif_struct)
#endif

class GifHandler
{
protected:
	ILcontext * context;

	ILenum		GifType;

	ILboolean	success;

	ILint		curr_size, clear, ending, newcodes, top_slot, slot, navail_bytes = 0, nbits_left = 0;
	ILubyte		b1;
	ILubyte		byte_buff[257];
	ILubyte*	pbytes;
	ILubyte*	stack;
	ILubyte*	suffix;
	ILshort*	prefix;

	ILint		get_next_code();
	void		cleanUpGifLoadState();

	ILboolean	GetImages(ILpal *GlobalPal, GIFHEAD *GifHead);
	ILboolean	GifGetData(ILimage *Image, ILubyte *Data, ILuint ImageSize, ILuint Width, ILuint Height, ILuint Stride, ILuint PalOffset, GFXCONTROL *Gfx);

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	GifHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};