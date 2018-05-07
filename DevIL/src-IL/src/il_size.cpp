//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/30/2009
//
// Filename: src-IL/src/il_size.cpp
//
// Description: Determines the size of output files for lump writing.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

ILuint iTargaSize(ILcontext* context);

//! Fake seek function
ILint ILAPIENTRY iSizeSeek(ILcontext* context, ILint Offset, ILuint Mode)
{
	switch (Mode)
	{
		case IL_SEEK_SET:
			context->impl->CurPos = Offset;
			if (context->impl->CurPos > context->impl->MaxPos)
				context->impl->MaxPos = context->impl->CurPos;
			break;

		case IL_SEEK_CUR:
			context->impl->CurPos = context->impl->CurPos + Offset;
			break;

		case IL_SEEK_END:
			context->impl->CurPos = context->impl->MaxPos + Offset;  // Offset should be negative in this case.
			break;

		default:
			ilSetError(context, IL_INTERNAL_ERROR);  // Should never happen!
			return -1;  // Error code
	}

	if (context->impl->CurPos > context->impl->MaxPos)
		context->impl->MaxPos = context->impl->CurPos;

	return 0;  // Code for success
}

ILuint ILAPIENTRY iSizeTell(ILcontext* context)
{
	return context->impl->CurPos;
}

ILint ILAPIENTRY iSizePutc(ILcontext* context, ILubyte Char)
{
	context->impl->CurPos++;
	if (context->impl->CurPos > context->impl->MaxPos)
		context->impl->MaxPos = context->impl->CurPos;
	return Char;
}

ILint ILAPIENTRY iSizeWrite(ILcontext* context, const void *Buffer, ILuint Size, ILuint Number)
{
	context->impl->CurPos += Size * Number;
	if (context->impl->CurPos > context->impl->MaxPos)
		context->impl->MaxPos = context->impl->CurPos;
	return Number;
}


//@TODO: Do computations for uncompressed formats without going through the
//       whole writing process.

//! Returns the size of the memory buffer needed to save the current image into this Type.
//  A return value of 0 is an error.
ILuint ilDetermineSize(ILcontext* context, ILenum Type)
{
	context->impl->MaxPos = context->impl->CurPos = 0;
	iSetOutputFake(context);  // Sets iputc, context->impl->iwrite(, etc. to functions above.

	switch (Type)
	{
		#ifndef IL_NO_BMP
		case IL_BMP:
			ilSaveBmpL(context, NULL, 0);
			break;
		#endif//IL_NO_BMP

		#ifndef IL_NO_DDS
		case IL_DDS:
			ilSaveDdsL(context, NULL, 0);
			break;
		#endif//IL_NO_DDS

		#ifndef IL_NO_EXR
		case IL_EXR:
			ilSaveExrL(context, NULL, 0);
			break;
		#endif//IL_NO_EXR

		#ifndef IL_NO_HDR
		case IL_HDR:
			ilSaveHdrL(context, NULL, 0);
			break;
		#endif//IL_NO_HDR

		#ifndef IL_NO_JP2
		case IL_JP2:
			ilSaveJp2L(context, NULL, 0);
			break;
		#endif//IL_NO_JP2

		#ifndef IL_NO_JPG
		case IL_JPG:
			ilSaveJpegL(context, NULL, 0);
			break;
		#endif//IL_NO_JPG

		#ifndef IL_NO_PCX
		case IL_PCX:
			ilSavePcxL(context, NULL, 0);
			break;
		#endif//IL_NO_PCX

		#ifndef IL_NO_PNG
		case IL_PNG:
			ilSavePngL(context, NULL, 0);
			break;
		#endif//IL_NO_PNG

		#ifndef IL_NO_PNM
		case IL_PNM:
			ilSavePnmL(context, NULL, 0);
			break;
		#endif//IL_NO_PNM

		#ifndef IL_NO_PSD
		case IL_PSD:
			ilSavePsdL(context, NULL, 0);
			break;
		#endif//IL_NO_PSD

		#ifndef IL_NO_RAW
		case IL_RAW:
			ilSaveRawL(context, NULL, 0);
			break;
		#endif//IL_NO_RAW

		#ifndef IL_NO_SGI
		case IL_SGI:
			ilSaveSgiL(context, NULL, 0);
			break;
		#endif//IL_NO_SGI

		#ifndef IL_NO_TGA
		case IL_TGA:
			//ilSaveTargaL(context, NULL, 0);
			return iTargaSize(context);
			break;
		#endif//IL_NO_TGA

		#ifndef IL_NO_TIF
		case IL_TIF:
			ilSaveTiffL(context, NULL, 0);
			break;
		#endif//IL_NO_TIF

		#ifndef IL_NO_WBMP
		case IL_WBMP:
			ilSaveWbmpL(context, NULL, 0);
			break;
		#endif//IL_NO_WBMP

		default:
			// 0 is an error for this.
			ilSetError(context, IL_INVALID_ENUM);
			return 0;
	}

	return context->impl->MaxPos;
}
