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

#include "il_bmp.h"
#include "il_dds.h"
#include "il_exr.h"
#include "il_hdr.h"
#include "il_jp2.h"
#include "il_jpeg.h"
#include "il_pcx.h"
#include "il_png.h"
#include "il_pnm.h"
#include "il_psd.h"
#include "il_raw.h"
#include "il_sgi.h"
#include "il_targa.h"
#include "il_tiff.h"
#include "il_wbmp.h"

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
		{
			BmpHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_BMP

		#ifndef IL_NO_DDS
		case IL_DDS:
		{
			DdsHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_DDS

		#ifndef IL_NO_EXR
		case IL_EXR:
		{
			ExrHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_EXR

		#ifndef IL_NO_HDR
		case IL_HDR:
		{
			HdrHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_HDR

		#ifndef IL_NO_JP2
		case IL_JP2:
		{
			Jp2Handler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_JP2

		#ifndef IL_NO_JPG
		case IL_JPG:
		{
			JpegHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_JPG

		#ifndef IL_NO_PCX
		case IL_PCX:
		{
			PcxHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_PCX

		#ifndef IL_NO_PNG
		case IL_PNG:
		{
			PngHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_PNG

		#ifndef IL_NO_PNM
		case IL_PNM:
		{
			PnmHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_PNM

		#ifndef IL_NO_PSD
		case IL_PSD:
		{
			PsdHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_PSD

		#ifndef IL_NO_RAW
		case IL_RAW:
		{
			RawHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_RAW

		#ifndef IL_NO_SGI
		case IL_SGI:
		{
			SgiHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_SGI

		#ifndef IL_NO_TGA
		case IL_TGA:
		{
			TargaHandler handler(context);

			handler.size();
		}
		break;
		#endif//IL_NO_TGA

		#ifndef IL_NO_TIF
		case IL_TIF:
		{
			TiffHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_TIF

		#ifndef IL_NO_WBMP
		case IL_WBMP:
		{
			WbmpHandler handler(context);

			handler.saveL(NULL, 0);
		}
		break;
		#endif//IL_NO_WBMP

		default:
			// 0 is an error for this.
			ilSetError(context, IL_INVALID_ENUM);
			return 0;
	}

	return context->impl->MaxPos;
}
