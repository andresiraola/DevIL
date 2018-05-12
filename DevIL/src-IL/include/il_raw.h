#pragma once

#include "il_internal.h"

class RawHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
	RawHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);

	ILboolean	save(ILconst_string FileName);
	ILuint		saveF(ILHANDLE File);
	ILuint		saveL(void *Lump, ILuint Size);
};