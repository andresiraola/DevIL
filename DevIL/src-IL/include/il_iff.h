#pragma once

#include "il_internal.h"

class IffHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	IffHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};