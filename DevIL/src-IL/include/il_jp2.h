//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 08/24/2008
//
// Filename: src-IL/src/il_jp2.h
//
// Description: Jpeg-2000 (.jp2) functions
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class Jp2Handler
{
protected:
	ILcontext* context;

	ILboolean	isValidInternal();
	ILboolean	saveInternal();

public:
	Jp2Handler(ILcontext* context);

    static ILboolean LoadLToImage(ILcontext* context, const void *Lump, ILuint Size, ILimage *Image);

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