#pragma once

#include "il_internal.h"

#ifndef IL_NO_TIF

class TiffHandler
{
protected:
    ILcontext*  context;

    ILboolean	isValidInternal();
    ILboolean	loadInternal();
	ILboolean	saveInternal();

public:
    TiffHandler(ILcontext* context);

    ILboolean	isValid(ILconst_string FileName);
    ILboolean	isValidF(ILHANDLE File);
    ILboolean	isValidL(const void *Lump, ILuint Size);
    
	ILboolean	load(ILconst_string FileName);
    ILboolean	loadF(ILHANDLE File);
    ILboolean	loadL(const void *Lump, ILuint Size);
    
	ILboolean	save(ILconst_string FileName);
    ILuint		saveF(ILHANDLE File);
    ILuint		saveL(void *Lump, ILuint Size);
};

#endif // IL_NO_TIF