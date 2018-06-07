//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pnm.h
//
// Description: Reads/writes to/from pbm/pgm/ppm formats
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "il_internal.h"

class PnmHandler
{
protected:
	ILcontext* context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	PnmHandler(ILcontext* context);

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