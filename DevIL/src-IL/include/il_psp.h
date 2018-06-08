//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/02/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_psp.h
//
// Description: Reads a Paint Shop Pro file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class PspHandler
{
protected:
	ILcontext * context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	PspHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};