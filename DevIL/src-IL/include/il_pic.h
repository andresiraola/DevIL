//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/21/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pic.h
//
// Description: Softimage Pic (.pic) functions
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class PicHandler
{
protected:
	ILcontext* context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	PicHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};