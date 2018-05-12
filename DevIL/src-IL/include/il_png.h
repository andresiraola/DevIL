#pragma once

#include "il_internal.h"

#ifndef IL_NO_PNG
#include <png.h>

class PngHandler
{
protected:
    ILcontext*  context;

    png_structp png_ptr = NULL;
    png_infop   info_ptr = NULL;
    ILint		png_color_type;

	ILint		readpng_init();
	ILboolean	readpng_get_image(ILdouble display_exponent);
	void		readpng_cleanup(void);

    ILboolean	isValidInternal();
	ILboolean	saveInternal();

public:
    PngHandler(ILcontext* context);

	// This is so bad and should be fixed
	ILboolean	loadInternal();

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

#endif // IL_NO_PNG