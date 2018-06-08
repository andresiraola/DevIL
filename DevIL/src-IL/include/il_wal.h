#pragma once

#include "il_internal.h"

class WalHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	WalHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};