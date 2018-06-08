//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/02/2009
//
// Filename: src-IL/include/il_utx.h
//
// Description: Reads from an Unreal and Unreal Tournament Texture (.utx) file.
//				Specifications can be found at
//				http://wiki.beyondunreal.com/Legacy:Package_File_Format.
//
//-----------------------------------------------------------------------------

#pragma once

class UtxHandler
{
protected:
	ILcontext * context;

	ILboolean	loadInternal();

public:
	UtxHandler(ILcontext* context);

	ILboolean	load(ILconst_string FileName);
	ILboolean	loadF(ILHANDLE File);
	ILboolean	loadL(const void *Lump, ILuint Size);
};