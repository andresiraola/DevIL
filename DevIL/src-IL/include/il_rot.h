#pragma once

#include "il_internal.h"

class RotHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	RotHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};