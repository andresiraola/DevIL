//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/sgi.h
//
// Description: Reads from and writes to SGI graphics files.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class SgiHandler
{
protected:
	ILcontext * context;

	char* FName = nullptr;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	SgiHandler(ILcontext* context);

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