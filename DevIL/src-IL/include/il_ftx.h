#pragma once

#include "il_internal.h"

class FtxHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	FtxHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};