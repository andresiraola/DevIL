//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_icon.h
//
// Description: Reads from a Windows icon (.ico) file.
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

class IconHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	IconHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};