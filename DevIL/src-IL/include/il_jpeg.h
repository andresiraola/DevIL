//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/include/il_jpeg.h
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------

#pragma once

#include "il_internal.h"

#ifndef IL_NO_JPG
	#ifndef IL_USE_IJL
		#include <jpeglib.h>

		#if JPEG_LIB_VERSION < 62
			#warning DevIL was designed with libjpeg 6b or higher in mind.Consider upgrading at www.ijg.org
		#endif
	#endif

class JpegHandler
{
protected:
	ILcontext*	context;

	ILboolean	jpgErrorOccured = IL_FALSE;

#ifndef IL_USE_IJL
	static void ExitErrorHandle(j_common_ptr cinfo);
	static boolean fill_input_buffer(j_decompress_ptr cinfo);
	static void skip_input_data(j_decompress_ptr cinfo, long num_bytes);
	static void iJpegErrorExit(j_common_ptr cinfo);
	
	void		devil_jpeg_read_init(j_decompress_ptr cinfo);
#endif

	ILboolean	loadFromJpegStruct(void *_JpegInfo);
	ILboolean	saveFromJpegStruct(void *_JpegInfo);

	ILboolean	check(ILubyte Header[2]);
	ILboolean	isValidInternal();

#ifndef IL_USE_IJL
	ILboolean	saveInternal();
#else
	ILboolean	saveInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
#endif

public:
	JpegHandler(ILcontext* context);

	// This is so bad and should be fixed
#ifndef IL_USE_IJL
	ILboolean	loadInternal();
#else
	ILboolean	loadInternal(ILconst_string FileName, ILvoid *Lump, ILuint Size);
#endif

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

#endif // IL_NO_JPG
