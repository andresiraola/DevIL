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

class WbmpHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	WbmpHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);

	ILboolean	save(ILconst_string FileName);
	ILuint		saveF(ILHANDLE File);
	ILuint		saveL(void *Lump, ILuint Size);
};