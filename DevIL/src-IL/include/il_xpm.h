#pragma once

#include "il_internal.h"

class XpmHandler
{
protected:
	ILcontext * context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	XpmHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};