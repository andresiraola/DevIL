//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_lif.c
//
// Description: Reads a Homeworld image.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class LifHandler
{
protected:
	ILcontext * context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	LifHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};