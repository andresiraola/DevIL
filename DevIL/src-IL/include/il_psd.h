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


#ifndef PSD_H
#define PSD_H

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

ILushort	ChannelNum;

ILboolean	iIsValidPsd(ILcontext* context);
ILboolean	iCheckPsd(PSDHEAD *Header);
ILboolean	iLoadPsdInternal(ILcontext* context);
ILboolean	ReadPsd(ILcontext* context, PSDHEAD *Head);
ILboolean	ReadGrey(ILcontext* context, PSDHEAD *Head);
ILboolean	ReadIndexed(ILcontext* context, PSDHEAD *Head);
ILboolean	ReadRGB(ILcontext* context, PSDHEAD *Head);
ILboolean	ReadCMYK(ILcontext* context, PSDHEAD *Head);
ILuint		*GetCompChanLen(ILcontext* context, PSDHEAD *Head);
ILboolean	PsdGetData(ILcontext* context, PSDHEAD *Head, void *Buffer, ILboolean Compressed);
ILboolean	ParseResources(ILcontext* context, ILuint ResourceSize, ILubyte *Resources);
ILboolean	GetSingleChannel(ILcontext* context, PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed);
ILboolean	iSavePsdInternal(ILcontext* context);



#endif//PSD_H
