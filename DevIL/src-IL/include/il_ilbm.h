#pragma once

#include "il_internal.h"

class IlbmHandler
{
protected:
	ILcontext* context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	IlbmHandler(ILcontext* context);

	ILboolean	isValid(ILconst_string FileName);
	ILboolean	isValidF(ILHANDLE File);
	ILboolean	isValidL(const void *Lump, ILuint Size);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};