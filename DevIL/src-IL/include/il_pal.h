//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_pal.h
//
// Description: Loads palettes from different file formats
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class PalHandler
{
protected:
	ILcontext * context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	PalHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);

	ILboolean	save(ILconst_string FileName);
};