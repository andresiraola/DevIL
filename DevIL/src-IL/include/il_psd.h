//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_il_psd.c
//
// Description: Reads from a PhotoShop (.psd) file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

#ifdef _MSC_VER
#pragma pack(push, packed_struct, 1)
#endif

typedef struct PSDHEAD
{
	ILubyte		Signature[4];
	ILushort	Version;
	ILubyte		Reserved[6];
	ILushort	Channels;
	ILuint		Height;
	ILuint		Width;
	ILushort	Depth;
	ILushort	Mode;
} IL_PACKSTRUCT PSDHEAD;

#ifdef _MSC_VER
#pragma pack(pop,  packed_struct)
#endif

class PsdHandler
{
protected:
	ILcontext * context;

	ILushort	ChannelNum;

	ILuint*		GetCompChanLen(PSDHEAD *Head);
	ILboolean	PsdGetData(PSDHEAD *Head, void *Buffer, ILboolean Compressed);
	ILboolean	ReadCMYK(PSDHEAD *Head);
	ILboolean	ReadGrey(PSDHEAD *Head);
	ILboolean	ReadIndexed(PSDHEAD *Head);
	ILboolean	ReadPsd(PSDHEAD *Head);
	ILboolean	ReadRGB(PSDHEAD *Head);

	ILboolean	check(PSDHEAD *Header);

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	PsdHandler(ILcontext* context);

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