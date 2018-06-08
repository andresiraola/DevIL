//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/16/2009
//
// Filename: src-IL/src/il_texture.cpp
//
// Description: Reads from a Medieval II: Total War	(by Creative Assembly)
//				Texture (.texture) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_TEXTURE

#include "il_dds.h"
#include "il_texture.h"

TextureHandler::TextureHandler(ILcontext* context) :
	context(context)
{

}

//! Reads a TEXTURE file
ILboolean TextureHandler::load(ILconst_string FileName)
{
	ILHANDLE	TextureFile;
	ILboolean	bTexture = IL_FALSE;
	
	TextureFile = context->impl->iopenr(FileName);
	if (TextureFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bTexture;
	}

	bTexture = loadF(TextureFile);
	context->impl->icloser(TextureFile);

	return bTexture;
}

//! Reads an already-opened TEXTURE file
ILboolean TextureHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	// From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
	//  is to strip out the first 48 bytes, and then it is DDS data.
	context->impl->iseek(context, 48, IL_SEEK_CUR);

	DdsHandler handler(context);

	bRet = handler.loadF(File);
	
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Reads from a memory "lump" that contains a TEXTURE
ILboolean TextureHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	// From http://forums.totalwar.org/vb/showthread.php?t=70886, all that needs to be done
	//  is to strip out the first 48 bytes, and then it is DDS data.
	context->impl->iseek(context, 48, IL_SEEK_CUR);

	DdsHandler handler(context);

	return handler.loadL(Lump, Size);
}

#endif // IL_NO_TEXTURE