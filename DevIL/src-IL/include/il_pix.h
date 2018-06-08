#pragma once

#include "il_internal.h"

class PixHandler
{
protected:
	ILcontext * context;

	ILboolean	isValidInternal();
	ILboolean	loadInternal();

public:
	PixHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};