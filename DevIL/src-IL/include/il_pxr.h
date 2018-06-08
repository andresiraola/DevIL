#pragma once

#include "il_internal.h"

class PxrHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	PxrHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};