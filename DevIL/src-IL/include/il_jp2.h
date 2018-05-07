//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 08/24/2008
//
// Filename: src-IL/src/il_jp2.h
//
// Description: Jpeg-2000 (.jp2) functions
//
//-----------------------------------------------------------------------------

#ifndef JP2_H
#define JP2_H

#include "il_internal.h"

ILboolean		iLoadJp2Internal(ILcontext* context, jas_stream_t *Stream, ILimage *Image);
ILboolean		iSaveJp2Internal(ILcontext* context);
jas_stream_t	*iJp2ReadStream(ILcontext* context);

#endif//JP2_H
