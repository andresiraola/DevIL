//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 08/23/2008
//
// Filename: src-IL/include/il_icns.h
//
// Description: Reads from a Mac OS X icon (.icns) file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class IcnsHandler
{
protected:
	ILcontext* context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	IcnsHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};