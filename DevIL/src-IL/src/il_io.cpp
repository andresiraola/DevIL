//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_io.cpp
//
// Description: Determines image types and loads/saves images
//
//-----------------------------------------------------------------------------

#include <string>

#include "il_internal.h"
#include "il_register.h"
#include "il_pal.h"

#include "il_blp.h"
#include "il_bmp.h"
#include "il_dds.h"
#include "il_gif.h"
#include "il_hdr.h"
#include "il_icns.h"
#include "il_ilbm.h"
#include "il_iwi.h"
#include "il_jp2.h"
#include "il_jpeg.h"
#include "il_ktx.h"
#include "il_lif.h"
#include "il_mdl.h"
#include "il_mp3.h"
#include "il_pcx.h"
#include "il_pic.h"
#include "il_png.h"
#include "il_pnm.h"
#include "il_psd.h"
#include "il_raw.h"
#include "il_targa.h"
#include "il_tiff.h"

// Returns a widened version of a string.
// Make sure to free this after it is used.  Code help from
//  https://buildsecurityin.us-cert.gov/daisy/bsi-rules/home/g1/769-BSI.html
#if defined(_UNICODE)
wchar_t *WideFromMultiByte(const char *Multi)
{
	ILint	Length;
	wchar_t	*Temp;

	Length = (ILint)mbstowcs(nullptr, (const char*)Multi, 0) + 1; // note error return of -1 is possible
	if (Length == 0) {
		ilSetError(context, IL_INVALID_PARAM);
		return nullptr;
	}
	if (Length > ULONG_MAX / sizeof(wchar_t)) {
		ilSetError(context, IL_INTERNAL_ERROR);
		return nullptr;
	}
	Temp = (wchar_t*)ialloc(Length * sizeof(wchar_t));
	mbstowcs(Temp, (const char*)Multi, Length);

	return Temp;
}
#endif

ILenum ILAPIENTRY ilTypeFromExt(ILcontext* context, ILconst_string FileName)
{
	ILenum		Type;
	ILstring	Ext;

	if (FileName == nullptr || ilStrLen(FileName) < 1) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_TYPE_UNKNOWN;
	}

	Ext = iGetExtension(FileName);
	//added 2003-08-31: fix sf bug 789535
	if (Ext == nullptr) {
		return IL_TYPE_UNKNOWN;
	}

	if (!iStrCmp(Ext, IL_TEXT("tga")) || !iStrCmp(Ext, IL_TEXT("vda")) ||
		!iStrCmp(Ext, IL_TEXT("icb")) || !iStrCmp(Ext, IL_TEXT("vst")))
		Type = IL_TGA;
	else if (!iStrCmp(Ext, IL_TEXT("jpg")) || !iStrCmp(Ext, IL_TEXT("jpe")) ||
		!iStrCmp(Ext, IL_TEXT("jpeg")) || !iStrCmp(Ext, IL_TEXT("jif")) || !iStrCmp(Ext, IL_TEXT("jfif")))
		Type = IL_JPG;
	else if (!iStrCmp(Ext, IL_TEXT("jp2")) || !iStrCmp(Ext, IL_TEXT("jpx")) ||
		!iStrCmp(Ext, IL_TEXT("j2k")) || !iStrCmp(Ext, IL_TEXT("j2c")))
		Type = IL_JP2;
	else if (!iStrCmp(Ext, IL_TEXT("dds")))
		Type = IL_DDS;
	else if (!iStrCmp(Ext, IL_TEXT("png")))
		Type = IL_PNG;
	else if (!iStrCmp(Ext, IL_TEXT("bmp")) || !iStrCmp(Ext, IL_TEXT("dib")))
		Type = IL_BMP;
	else if (!iStrCmp(Ext, IL_TEXT("gif")))
		Type = IL_GIF;
	else if (!iStrCmp(Ext, IL_TEXT("blp")))
		Type = IL_BLP;
	else if (!iStrCmp(Ext, IL_TEXT("cut")))
		Type = IL_CUT;
	else if (!iStrCmp(Ext, IL_TEXT("ch")) || !iStrCmp(Ext, IL_TEXT("ct")) || !iStrCmp(Ext, IL_TEXT("sct")))
		Type = IL_SCITEX;
	else if (!iStrCmp(Ext, IL_TEXT("dcm")) || !iStrCmp(Ext, IL_TEXT("dicom")))
		Type = IL_DICOM;
	else if (!iStrCmp(Ext, IL_TEXT("dpx")))
		Type = IL_DPX;
	else if (!iStrCmp(Ext, IL_TEXT("exr")))
		Type = IL_EXR;
	else if (!iStrCmp(Ext, IL_TEXT("fit")) || !iStrCmp(Ext, IL_TEXT("fits")))
		Type = IL_FITS;
	else if (!iStrCmp(Ext, IL_TEXT("ftx")))
		Type = IL_FTX;
	else if (!iStrCmp(Ext, IL_TEXT("hdr")))
		Type = IL_HDR;
	else if (!iStrCmp(Ext, IL_TEXT("iff")))
		Type = IL_IFF;
	else if (!iStrCmp(Ext, IL_TEXT("ilbm")) || !iStrCmp(Ext, IL_TEXT("lbm")) ||
		!iStrCmp(Ext, IL_TEXT("ham")))
		Type = IL_ILBM;
	else if (!iStrCmp(Ext, IL_TEXT("ico")) || !iStrCmp(Ext, IL_TEXT("cur")))
		Type = IL_ICO;
	else if (!iStrCmp(Ext, IL_TEXT("icns")))
		Type = IL_ICNS;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("iwi")))
		Type = IL_IWI;
	else if (!iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_JNG;
	else if (!iStrCmp(Ext, IL_TEXT("ktx")))
		Type = IL_KTX;
	else if (!iStrCmp(Ext, IL_TEXT("lif")))
		Type = IL_LIF;
	else if (!iStrCmp(Ext, IL_TEXT("mdl")))
		Type = IL_MDL;
	else if (!iStrCmp(Ext, IL_TEXT("mng")) || !iStrCmp(Ext, IL_TEXT("jng")))
		Type = IL_MNG;
	else if (!iStrCmp(Ext, IL_TEXT("mp3")))
		Type = IL_MP3;
	else if (!iStrCmp(Ext, IL_TEXT("pcd")))
		Type = IL_PCD;
	else if (!iStrCmp(Ext, IL_TEXT("pcx")))
		Type = IL_PCX;
	else if (!iStrCmp(Ext, IL_TEXT("pic")))
		Type = IL_PIC;
	else if (!iStrCmp(Ext, IL_TEXT("pix")))
		Type = IL_PIX;
	else if (!iStrCmp(Ext, IL_TEXT("pbm")) || !iStrCmp(Ext, IL_TEXT("pgm")) ||
		!iStrCmp(Ext, IL_TEXT("pnm")) || !iStrCmp(Ext, IL_TEXT("ppm")) || !iStrCmp(Ext, IL_TEXT("pam")))
		Type = IL_PNM;
	else if (!iStrCmp(Ext, IL_TEXT("psd")) || !iStrCmp(Ext, IL_TEXT("pdd")))
		Type = IL_PSD;
	else if (!iStrCmp(Ext, IL_TEXT("psp")))
		Type = IL_PSP;
	else if (!iStrCmp(Ext, IL_TEXT("pxr")))
		Type = IL_PXR;
	else if (!iStrCmp(Ext, IL_TEXT("rot")))
		Type = IL_ROT;
	else if (!iStrCmp(Ext, IL_TEXT("sgi")) || !iStrCmp(Ext, IL_TEXT("bw")) ||
		!iStrCmp(Ext, IL_TEXT("rgb")) || !iStrCmp(Ext, IL_TEXT("rgba")))
		Type = IL_SGI;
	else if (!iStrCmp(Ext, IL_TEXT("sun")) || !iStrCmp(Ext, IL_TEXT("ras")) ||
		!iStrCmp(Ext, IL_TEXT("rs")) || !iStrCmp(Ext, IL_TEXT("im1")) ||
		!iStrCmp(Ext, IL_TEXT("im8")) || !iStrCmp(Ext, IL_TEXT("im24")) ||
		!iStrCmp(Ext, IL_TEXT("im32")))
		Type = IL_SUN;
	else if (!iStrCmp(Ext, IL_TEXT("texture")))
		Type = IL_TEXTURE;
	else if (!iStrCmp(Ext, IL_TEXT("tif")) || !iStrCmp(Ext, IL_TEXT("tiff")))
		Type = IL_TIF;
	else if (!iStrCmp(Ext, IL_TEXT("tpl")))
		Type = IL_TPL;
	else if (!iStrCmp(Ext, IL_TEXT("utx")))
		Type = IL_UTX;
	else if (!iStrCmp(Ext, IL_TEXT("vtf")))
		Type = IL_VTF;
	else if (!iStrCmp(Ext, IL_TEXT("wal")))
		Type = IL_WAL;
	else if (!iStrCmp(Ext, IL_TEXT("wbmp")))
		Type = IL_WBMP;
	else if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp")))
		Type = IL_WDP;
	else if (!iStrCmp(Ext, IL_TEXT("xpm")))
		Type = IL_XPM;
	else
		Type = IL_TYPE_UNKNOWN;

	return Type;
}

//changed 2003-09-17 to ILAPIENTRY
ILenum ILAPIENTRY ilDetermineType(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE File;
	ILenum Type;

	if (FileName == nullptr)
	{
		return IL_TYPE_UNKNOWN;
	}

	File = context->impl->iopenr(FileName);

	if (File == nullptr)
	{
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	Type = ilDetermineTypeF(context, File);
	context->impl->icloser(File);

	return Type;
}

ILenum ILAPIENTRY ilDetermineTypeF(ILcontext* context, ILHANDLE File)
{
	if (File == nullptr)
	{
		return IL_TYPE_UNKNOWN;
	}

#ifndef IL_NO_JPG
	{
		JpegHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_JPG;
		}
	}
#endif

#ifndef IL_NO_DDS
	{
		DdsHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_DDS;
		}
	}
#endif

#ifndef IL_NO_PNG
	{
		PngHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_PNG;
		}
	}
#endif

#ifndef IL_NO_BMP
	{
		BmpHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_BMP;
		}
	}
#endif

#ifndef IL_NO_BLP
	{
		BlpHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_BLP;
		}
	}
#endif

#ifndef IL_NO_EXR
	if (ilIsValidExrF(context, File))
		return IL_EXR;
#endif

#ifndef IL_NO_GIF
	{
		GifHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_GIF;
		}
	}
#endif

#ifndef IL_NO_HDR
	{
		HdrHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_HDR;
		}
	}
#endif

#ifndef IL_NO_ICNS
	{
		IcnsHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_ICNS;
		}
	}
#endif

#ifndef IL_NO_ILBM
	{
		IcnsHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_ILBM;
		}
	}
#endif

#ifndef IL_NO_IWI
	{
		IwiHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_IWI;
		}
	}
#endif

#ifndef IL_NO_JP2
	{
		Jp2Handler handler(context);

		if (handler.isValidF(File))
		{
			return IL_JP2;
		}
	}
#endif

#ifndef IL_NO_KTX
	{
		KtxHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_KTX;
		}
	}
#endif

#ifndef IL_NO_LIF
	{
		LifHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_LIF;
		}
	}
#endif

#ifndef IL_NO_MDL
	{
		MdlHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_MDL;
		}
	}
#endif

#ifndef IL_NO_MP3
	{
		Mp3Handler handler(context);

		if (handler.isValidF(File))
		{
			return IL_MP3;
		}
	}
#endif

#ifndef IL_NO_PCX
	{
		PcxHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_PCX;
		}
	}
#endif

#ifndef IL_NO_PIC
	{
		PicHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_PIC;
		}
	}
#endif

#ifndef IL_NO_PNM
	{
		PnmHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_PNM;
		}
	}
#endif

#ifndef IL_NO_PSD
	{
		PsdHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_PSD;
		}
	}
#endif

#ifndef IL_NO_PSP
	if (ilIsValidPspF(context, File))
		return IL_PSP;
#endif

#ifndef IL_NO_SGI
	if (ilIsValidSgiF(context, File))
		return IL_SGI;
#endif

#ifndef IL_NO_SCITEX
	if (ilIsValidScitexF(context, File))
		return IL_SCITEX;
#endif

#ifndef IL_NO_SUN
	if (ilIsValidSunF(context, File))
		return IL_SUN;
#endif

#ifndef IL_NO_TIF
	{
		TiffHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_TIF;
		}
	}
#endif

#ifndef IL_NO_TPL
	if (ilIsValidTplF(context, File))
		return IL_TPL;
#endif

#ifndef IL_NO_VTF
	if (ilIsValidVtfF(context, File))
		return IL_VTF;
#endif

#ifndef IL_NO_XPM
	if (ilIsValidXpmF(context, File))
		return IL_XPM;
#endif

	//moved tga to end of list because it has no magic number
	//in header to assure that this is really a tga... (20040218)
#ifndef IL_NO_TGA
	{
		TargaHandler handler(context);

		if (handler.isValidF(File))
		{
			return IL_TGA;
		}
	}
#endif

	return IL_TYPE_UNKNOWN;
}


ILenum ILAPIENTRY ilDetermineTypeL(ILcontext* context, const void *Lump, ILuint Size)
{
	if (Lump == nullptr)
		return IL_TYPE_UNKNOWN;

#ifndef IL_NO_JPG
	{
		JpegHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_JPG;
		}
	}
#endif

#ifndef IL_NO_DDS
	{
		DdsHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_DDS;
		}
	}
#endif

#ifndef IL_NO_PNG
	{
		PngHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_PNG;
		}
	}
#endif

#ifndef IL_NO_BMP
	{
		BmpHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_BMP;
		}
	}
#endif

#ifndef IL_NO_BLP
	{
		BlpHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_BLP;
		}
	}
#endif

#ifndef IL_NO_EXR
	if (ilIsValidExrL(context, Lump, Size))
		return IL_EXR;
#endif

#ifndef IL_NO_GIF
	{
		GifHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_GIF;
		}
	}
#endif

#ifndef IL_NO_HDR
	{
		HdrHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_HDR;
		}
	}
#endif

#ifndef IL_NO_ICNS
	{
		IcnsHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_ICNS;
		}
	}
#endif

#ifndef IL_NO_IWI
	{
		IwiHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_IWI;
		}
	}
#endif

#ifndef IL_NO_ILBM
	{
		IcnsHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_ILBM;
		}
	}
#endif

#ifndef IL_NO_JP2
	{
		Jp2Handler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_JP2;
		}
	}
#endif

#ifndef IL_NO_KTX
	{
		KtxHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_KTX;
		}
	}
#endif

#ifndef IL_NO_LIF
	{
		LifHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_LIF;
		}
	}
#endif

#ifndef IL_NO_MDL
	{
		MdlHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_MDL;
		}
	}
#endif

#ifndef IL_NO_MP3
	{
		Mp3Handler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_MP3;
		}
	}
#endif

#ifndef IL_NO_PCX
	{
		PcxHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_PCX;
		}
	}
#endif

#ifndef IL_NO_PIC
	{
		PicHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_PIC;
		}
	}
#endif

#ifndef IL_NO_PNM
	{
		PnmHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_PNM;
		}
	}
#endif

#ifndef IL_NO_PSD
	{
		PsdHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_PSD;
		}
	}
#endif

#ifndef IL_NO_PSP
	if (ilIsValidPspL(context, Lump, Size))
		return IL_PSP;
#endif

#ifndef IL_NO_SCITEX
	if (ilIsValidScitexL(context, Lump, Size))
		return IL_SCITEX;
#endif

#ifndef IL_NO_SGI
	if (ilIsValidSgiL(context, Lump, Size))
		return IL_SGI;
#endif

#ifndef IL_NO_SUN
	if (ilIsValidSunL(context, Lump, Size))
		return IL_SUN;
#endif

#ifndef IL_NO_TIF
	{
		TiffHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_TIF;
		}
	}
#endif

#ifndef IL_NO_TPL
	if (ilIsValidTplL(context, Lump, Size))
		return IL_TPL;
#endif

#ifndef IL_NO_VTF
	if (ilIsValidVtfL(context, Lump, Size))
		return IL_VTF;
#endif

#ifndef IL_NO_XPM
	if (ilIsValidXpmL(context, Lump, Size))
		return IL_XPM;
#endif

	//Moved Targa to end of list because it has no magic number
	// in header to assure that this is really a tga... (20040218).
#ifndef IL_NO_TGA
	{
		TargaHandler handler(context);

		if (handler.isValidL(Lump, Size))
		{
			return IL_TGA;
		}
	}
#endif

	return IL_TYPE_UNKNOWN;
}


ILboolean ILAPIENTRY ilIsValid(ILcontext* context, ILenum Type, ILconst_string FileName)
{
	if (FileName == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		return ilIsValidDicom(context, FileName);
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilIsValidExr(context, FileName);
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_LIF
	{
		LifHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		return ilIsValidPsp(context, FileName);
#endif

#ifndef IL_NO_SCITEX
	case IL_SCITEX:
		return ilIsValidScitex(context, FileName);
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilIsValidSgi(context, FileName);
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		return ilIsValidSun(context, FileName);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.isValid(FileName);
	}
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		return ilIsValidTpl(context, FileName);
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilIsValidVtf(context, FileName);
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		return ilIsValidXpm(context, FileName);
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


ILboolean ILAPIENTRY ilIsValidF(ILcontext* context, ILenum Type, ILHANDLE File)
{
	if (File == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		return ilIsValidDicomF(context, File);
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilIsValidExrF(context, File);
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_LIF
	case IL_LIF:
	{
		LifHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		return ilIsValidPspF(context, File);
#endif

#ifndef IL_NO_SCITEX
	case IL_SCITEX:
		return ilIsValidScitexF(context, File);
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilIsValidSgiF(context, File);
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		return ilIsValidSunF(context, File);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.isValidF(File);
	}
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		return ilIsValidTplF(context, File);
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilIsValidVtfF(context, File);
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		return ilIsValidXpmF(context, File);
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


ILboolean ILAPIENTRY ilIsValidL(ILcontext* context, ILenum Type, void *Lump, ILuint Size)
{
	if (Lump == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		return ilIsValidDicomL(context, Lump, Size);
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilIsValidExrL(context, Lump, Size);
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_LIF
	case IL_LIF:
	{
		LifHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		return ilIsValidPspL(context, Lump, Size);
#endif

#ifndef IL_NO_SCITEX
	case IL_SCITEX:
		return ilIsValidScitexL(context, Lump, Size);
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilIsValidSgiL(context, Lump, Size);
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		return ilIsValidSunL(context, Lump, Size);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.isValidL(Lump, Size);
	}
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		return ilIsValidTplL(context, Lump, Size);
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilIsValidVtfL(context, Lump, Size);
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		return ilIsValidXpmL(context, Lump, Size);
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


//! Attempts to load an image from a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
If IL_TYPE_UNKNOWN is specified, ilLoad will try to determine the type of the file and load it.
\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
the filename of the file to load.
\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
have been tried and failed.*/
ILboolean ILAPIENTRY ilLoad(ILcontext* context, ILenum Type, ILconst_string FileName)
{
	ILboolean	bRet;

	if (FileName == nullptr || ilStrLen(FileName) < 1) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	switch (Type)
	{
	case IL_TYPE_UNKNOWN:
		bRet = ilLoadImage(context, FileName);
		break;

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_DPX
	case IL_DPX:
		bRet = ilLoadDpx(context, FileName);
		break;
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_CUT
	case IL_CUT:
		bRet = ilLoadCut(context, FileName);
		break;
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		bRet = ilLoadDicom(context, FileName);
		break;
#endif

#ifndef IL_NO_DOOM
	case IL_DOOM:
		bRet = ilLoadDoom(context, FileName);
		break;
	case IL_DOOM_FLAT:
		bRet = ilLoadDoomFlat(context, FileName);
		break;
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		bRet = ilLoadExr(context, FileName);
		break;
#endif

#ifndef IL_NO_FITS
	case IL_FITS:
		bRet = ilLoadFits(context, FileName);
		break;
#endif

#ifndef IL_NO_FTX
	case IL_FTX:
		bRet = ilLoadFtx(context, FileName);
		break;
#endif

#ifndef IL_NO_ICO
	case IL_ICO:
		bRet = ilLoadIcon(context, FileName);
		break;
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_IFF
	case IL_IFF:
		bRet = ilLoadIff(context, FileName);
		break;
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_LIF
	case IL_LIF:
	{
		LifHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_MNG
	case IL_MNG:
		bRet = ilLoadMng(context, FileName);
		break;
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PCD
	case IL_PCD:
		ilLoadPcd(context, FileName);
		break;
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PIX
	case IL_PIX:
		bRet = ilLoadPix(context, FileName);
		break;
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		bRet = ilLoadPsp(context, FileName);
		break;
#endif

#ifndef IL_NO_PXR
	case IL_PXR:
		bRet = ilLoadPxr(context, FileName);
		break;
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_ROT
	case IL_ROT:
		bRet = ilLoadRot(context, FileName);
		break;
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		bRet = ilLoadSgi(context, FileName);
		break;
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		bRet = ilLoadSun(context, FileName);
		break;
#endif

#ifndef IL_NO_TEXTURE
	case IL_TEXTURE:
		bRet = ilLoadTexture(context, FileName);
		break;
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		bRet = handler.load(FileName);
	}
	break;
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		bRet = ilLoadTpl(context, FileName);
		break;
#endif

#ifndef IL_NO_UTX
	case IL_UTX:
		bRet = ilLoadUtx(context, FileName);
		break;
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		bRet = ilLoadVtf(context, FileName);
		break;
#endif

#ifndef IL_NO_WAL
	case IL_WAL:
		bRet = ilLoadWal(context, FileName);
		break;
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		bRet = ilLoadWbmp(context, FileName);
		break;
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		bRet = ilLoadXpm(context, FileName);
		break;
#endif

#ifndef IL_NO_WDP
	case IL_WDP:
		bRet = ilLoadWdp(context, FileName);
		break;
#endif

	default:
		ilSetError(context, IL_INVALID_ENUM);
		bRet = IL_FALSE;
	}

	return bRet;
}


//! Attempts to load an image from a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
If IL_TYPE_UNKNOWN is specified, ilLoadF will try to determine the type of the file and load it.
\param File File stream to load from.
\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILboolean ILAPIENTRY ilLoadF(ILcontext* context, ILenum Type, ILHANDLE File)
{
	if (File == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (Type == IL_TYPE_UNKNOWN)
		Type = ilDetermineTypeF(context, File);

	switch (Type)
	{
	case IL_TYPE_UNKNOWN:
		return IL_FALSE;

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.loadF(File);
	}
#endif
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_CUT
	case IL_CUT:
		return ilLoadCutF(context, File);
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		return ilLoadDicomF(context, File);
#endif

#ifndef IL_NO_DOOM
	case IL_DOOM:
		return ilLoadDoomF(context, File);
	case IL_DOOM_FLAT:
		return ilLoadDoomFlatF(context, File);
#endif

#ifndef IL_NO_DPX
	case IL_DPX:
		return ilLoadDpxF(context, File);
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilLoadExrF(context, File);
#endif

#ifndef IL_NO_FITS
	case IL_FITS:
		return ilLoadFitsF(context, File);
#endif

#ifndef IL_NO_FTX
	case IL_FTX:
		return ilLoadFtxF(context, File);
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_ICO
	case IL_ICO:
		return ilLoadIconF(context, File);
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_IFF
	case IL_IFF:
		return ilLoadIffF(context, File);
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_LIF
	case IL_LIF:
	{
		LifHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_MNG
	case IL_MNG:
		return ilLoadMngF(context, File);
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PCD
	case IL_PCD:
		return ilLoadPcdF(context, File);
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PIX
	case IL_PIX:
		return ilLoadPixF(context, File);
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		return ilLoadPspF(context, File);
#endif

#ifndef IL_NO_PXR
	case IL_PXR:
		return ilLoadPxrF(context, File);
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_ROT
	case IL_ROT:
		return ilLoadRotF(context, File);
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilLoadSgiF(context, File);
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		return ilLoadSunF(context, File);
#endif

#ifndef IL_NO_TEXTURE
	case IL_TEXTURE:
		return ilLoadTextureF(context, File);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.loadF(File);
	}
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		return ilLoadTplF(context, File);
#endif

#ifndef IL_NO_UTX
	case IL_UTX:
		return ilLoadUtxF(context, File);
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilLoadVtfF(context, File);
#endif

#ifndef IL_NO_WAL
	case IL_WAL:
		return ilLoadWalF(context, File);
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		return ilLoadWbmpF(context, File);
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		return ilLoadXpmF(context, File);
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


//! Attempts to load an image from a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BLP, IL_BMP, IL_CUT, IL_DCX, IL_DDS,
IL_DICOM, IL_DOOM, IL_DOOM_FLAT, IL_DPX, IL_EXR, IL_FITS, IL_FTX, IL_GIF, IL_HDR, IL_ICO, IL_ICNS,
IL_IFF, IL_IWI, IL_JP2, IL_JPG, IL_LIF, IL_MDL,	IL_MNG, IL_MP3, IL_PCD, IL_PCX, IL_PIX, IL_PNG,
IL_PNM, IL_PSD, IL_PSP, IL_PXR, IL_ROT, IL_SGI, IL_SUN, IL_TEXTURE, IL_TGA, IL_TIF, IL_TPL,
IL_UTX, IL_VTF, IL_WAL, IL_WBMP, IL_XPM, IL_RAW, IL_JASC_PAL and IL_TYPE_UNKNOWN.
If IL_TYPE_UNKNOWN is specified, ilLoadL will try to determine the type of the file and load it.
\param Lump The buffer where the file data is located
\param Size Size of the buffer
\return Boolean value of failure or success.  Returns IL_FALSE if loading fails.*/
ILboolean ILAPIENTRY ilLoadL(ILcontext* context, ILenum Type, const void *Lump, ILuint Size)
{
	if (Lump == nullptr || Size == 0) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (Type == IL_TYPE_UNKNOWN)
		Type = ilDetermineTypeL(context, Lump, Size);

	switch (Type)
	{
	case IL_TYPE_UNKNOWN:
		return IL_FALSE;

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_BLP
	case IL_BLP:
	{
		BlpHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_CUT
	case IL_CUT:
		return ilLoadCutL(context, Lump, Size);
#endif

#ifndef IL_NO_DICOM
	case IL_DICOM:
		return ilLoadDicomL(context, Lump, Size);
#endif

#ifndef IL_NO_DOOM
	case IL_DOOM:
		return ilLoadDoomL(context, Lump, Size);
	case IL_DOOM_FLAT:
		return ilLoadDoomFlatL(context, Lump, Size);
#endif

#ifndef IL_NO_DPX
	case IL_DPX:
		return ilLoadDpxL(context, Lump, Size);
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilLoadExrL(context, Lump, Size);
#endif

#ifndef IL_NO_FITS
	case IL_FITS:
		return ilLoadFitsL(context, Lump, Size);
#endif

#ifndef IL_NO_FTX
	case IL_FTX:
		return ilLoadFtxL(context, Lump, Size);
#endif

#ifndef IL_NO_GIF
	case IL_GIF:
	{
		GifHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_ICO
	case IL_ICO:
		return ilLoadIconL(context, Lump, Size);
#endif

#ifndef IL_NO_ICNS
	case IL_ICNS:
	{
		IcnsHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_IFF
	case IL_IFF:
		return ilLoadIffL(context, Lump, Size);
#endif

#ifndef IL_NO_ILBM
	case IL_ILBM:
	{
		IlbmHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_IWI
	case IL_IWI:
	{
		IwiHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_KTX
	case IL_KTX:
	{
		KtxHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_LIF
	case IL_LIF:
	{
		LifHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_MDL
	case IL_MDL:
	{
		MdlHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_MNG
	case IL_MNG:
		return ilLoadMngL(context, Lump, Size);
#endif

#ifndef IL_NO_MP3
	case IL_MP3:
	{
		Mp3Handler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PCD
	case IL_PCD:
		return ilLoadPcdL(context, Lump, Size);
#endif

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PIC
	case IL_PIC:
	{
		PicHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PIX
	case IL_PIX:
		return ilLoadPixL(context, Lump, Size);
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_PSP
	case IL_PSP:
		return ilLoadPspL(context, Lump, Size);
#endif

#ifndef IL_NO_PXR
	case IL_PXR:
		return ilLoadPxrL(context, Lump, Size);
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_ROT
	case IL_ROT:
		return ilLoadRotL(context, Lump, Size);
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilLoadSgiL(context, Lump, Size);
#endif

#ifndef IL_NO_SUN
	case IL_SUN:
		return ilLoadSunL(context, Lump, Size);
#endif

#ifndef IL_NO_TEXTURE
	case IL_TEXTURE:
		return ilLoadTextureL(context, Lump, Size);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.loadL(Lump, Size);
	}
#endif

#ifndef IL_NO_TPL
	case IL_TPL:
		return ilLoadTplL(context, Lump, Size);
#endif

#ifndef IL_NO_UTX
	case IL_UTX:
		return ilLoadUtxL(context, Lump, Size);
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilLoadVtfL(context, Lump, Size);
#endif

#ifndef IL_NO_WAL
	case IL_WAL:
		return ilLoadWalL(context, Lump, Size);
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		return ilLoadWbmpL(context, Lump, Size);
#endif

#ifndef IL_NO_XPM
	case IL_XPM:
		return ilLoadXpmL(context, Lump, Size);
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


//! Attempts to load an image from a file with various different methods before failing - very generic.
/*! The ilLoadImage function allows a general interface to the specific internal file-loading
routines.  First, it finds the extension and checks to see if any user-registered functions
(registered through ilRegisterLoad) match the extension. If nothing matches, it takes the
extension and determines which function to call based on it. Lastly, it attempts to identify
the image based on various image header verification functions, such as ilIsValidPngF.
If all this checking fails, IL_FALSE is returned with no modification to the current bound image.
\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
the filename of the file to load.
\return Boolean value of failure or success.  Returns IL_FALSE if all three loading methods
have been tried and failed.*/
ILboolean ILAPIENTRY ilLoadImage(ILcontext* context, ILconst_string FileName)
{
	ILstring	Ext;
	ILenum		Type;
	ILboolean	bRet = IL_FALSE;

	if (context->impl->iCurImage == nullptr) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (FileName == nullptr || ilStrLen(FileName) < 1) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	Ext = iGetExtension(FileName);

	// Try registered procedures first (so users can override default lib functions).
	if (Ext) {
		if (iRegisterLoad(context, FileName))
			return IL_TRUE;

#ifndef IL_NO_TGA
		if (!iStrCmp(Ext, IL_TEXT("tga")) || !iStrCmp(Ext, IL_TEXT("vda")) ||
			!iStrCmp(Ext, IL_TEXT("icb")) || !iStrCmp(Ext, IL_TEXT("vst")))
		{
			TargaHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_JPG
		if (!iStrCmp(Ext, IL_TEXT("jpg")) || !iStrCmp(Ext, IL_TEXT("jpe")) ||
			!iStrCmp(Ext, IL_TEXT("jpeg")) || !iStrCmp(Ext, IL_TEXT("jif")) || !iStrCmp(Ext, IL_TEXT("jfif")))
		{
			JpegHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_JP2
		if (!iStrCmp(Ext, IL_TEXT("jp2")) || !iStrCmp(Ext, IL_TEXT("jpx")) ||
			!iStrCmp(Ext, IL_TEXT("j2k")) || !iStrCmp(Ext, IL_TEXT("j2c")))
		{
			Jp2Handler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_DDS
		if (!iStrCmp(Ext, IL_TEXT("dds")))
		{
			DdsHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PNG
		if (!iStrCmp(Ext, IL_TEXT("png")))
		{
			PngHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_BMP
		if (!iStrCmp(Ext, IL_TEXT("bmp")) || !iStrCmp(Ext, IL_TEXT("dib")))
		{
			BmpHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_BLP
		if (!iStrCmp(Ext, IL_TEXT("blp")))
		{
			BlpHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_SCITEX
		if (!iStrCmp(Ext, IL_TEXT("ch")) || !iStrCmp(Ext, IL_TEXT("ct")) || !iStrCmp(Ext, IL_TEXT("sct"))) {
			bRet = ilLoadScitex(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_DPX
		if (!iStrCmp(Ext, IL_TEXT("dpx"))) {
			bRet = ilLoadDpx(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_EXR
		if (!iStrCmp(Ext, IL_TEXT("exr"))) {
			bRet = ilLoadExr(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_GIF
		if (!iStrCmp(Ext, IL_TEXT("gif")))
		{
			GifHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_HDR
		if (!iStrCmp(Ext, IL_TEXT("hdr")))
		{
			HdrHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_CUT
		if (!iStrCmp(Ext, IL_TEXT("cut"))) {
			bRet = ilLoadCut(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_DCX
		if (!iStrCmp(Ext, IL_TEXT("dcx"))) {
			bRet = ilLoadDcx(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_DICOM
		if (!iStrCmp(Ext, IL_TEXT("dicom")) || !iStrCmp(Ext, IL_TEXT("dcm"))) {
			bRet = ilLoadDicom(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_FITS
		if (!iStrCmp(Ext, IL_TEXT("fits")) || !iStrCmp(Ext, IL_TEXT("fit"))) {
			bRet = ilLoadFits(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_FTX
		if (!iStrCmp(Ext, IL_TEXT("ftx"))) {
			bRet = ilLoadFtx(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_ICO
		if (!iStrCmp(Ext, IL_TEXT("ico")) || !iStrCmp(Ext, IL_TEXT("cur"))) {
			bRet = ilLoadIcon(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_ICNS
		if (!iStrCmp(Ext, IL_TEXT("icns")))
		{
			IcnsHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_IFF
		if (!iStrCmp(Ext, IL_TEXT("iff"))) {
			bRet = ilLoadIff(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_ILBM
		if (!iStrCmp(Ext, IL_TEXT("ilbm")) || !iStrCmp(Ext, IL_TEXT("lbm")) || !iStrCmp(Ext, IL_TEXT("ham")))
		{
			IcnsHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_IWI
		if (!iStrCmp(Ext, IL_TEXT("iwi")))
		{
			IwiHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_KTX
		if (!iStrCmp(Ext, IL_TEXT("ktx")))
		{
			IwiHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_LIF
		if (!iStrCmp(Ext, IL_TEXT("lif")))
		{
			LifHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_MDL
		if (!iStrCmp(Ext, IL_TEXT("mdl")))
		{
			MdlHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_MNG
		if (!iStrCmp(Ext, IL_TEXT("mng")) || !iStrCmp(Ext, IL_TEXT("jng"))) {
			bRet = ilLoadMng(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_MP3
		if (!iStrCmp(Ext, IL_TEXT("mp3")))
		{
			Mp3Handler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PCD
		if (!iStrCmp(Ext, IL_TEXT("pcd"))) {
			bRet = ilLoadPcd(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_PCX
		if (!iStrCmp(Ext, IL_TEXT("pcx")))
		{
			PcxHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PIC
		if (!iStrCmp(Ext, IL_TEXT("pic")))
		{
			PicHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PIX
		if (!iStrCmp(Ext, IL_TEXT("pix"))) {
			bRet = ilLoadPix(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_PNM
		if (!iStrCmp(Ext, IL_TEXT("pbm")) || !iStrCmp(Ext, IL_TEXT("pgm")) ||
			!iStrCmp(Ext, IL_TEXT("pnm")) || !iStrCmp(Ext, IL_TEXT("ppm")) || 
			!iStrCmp(Ext, IL_TEXT("pam")))
		{
			PnmHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PSD
		if (!iStrCmp(Ext, IL_TEXT("psd")) || !iStrCmp(Ext, IL_TEXT("pdd")))
		{
			PsdHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_PSP
		if (!iStrCmp(Ext, IL_TEXT("psp"))) {
			bRet = ilLoadPsp(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_PXR
		if (!iStrCmp(Ext, IL_TEXT("pxr"))) {
			bRet = ilLoadPxr(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_ROT
		if (!iStrCmp(Ext, IL_TEXT("rot"))) {
			bRet = ilLoadRot(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_SGI
		if (!iStrCmp(Ext, IL_TEXT("sgi")) || !iStrCmp(Ext, IL_TEXT("bw")) ||
			!iStrCmp(Ext, IL_TEXT("rgb")) || !iStrCmp(Ext, IL_TEXT("rgba"))) {
			bRet = ilLoadSgi(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_SUN
		if (!iStrCmp(Ext, IL_TEXT("sun")) || !iStrCmp(Ext, IL_TEXT("ras")) ||
			!iStrCmp(Ext, IL_TEXT("rs")) || !iStrCmp(Ext, IL_TEXT("im1")) ||
			!iStrCmp(Ext, IL_TEXT("im8")) || !iStrCmp(Ext, IL_TEXT("im24")) ||
			!iStrCmp(Ext, IL_TEXT("im32"))) {
			bRet = ilLoadSun(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_TEXTURE
		if (!iStrCmp(Ext, IL_TEXT("texture"))) {
			bRet = ilLoadTexture(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_TIF
		if (!iStrCmp(Ext, IL_TEXT("tif")) || !iStrCmp(Ext, IL_TEXT("tiff")))
		{
			TiffHandler handler(context);

			bRet = handler.load(FileName);

			goto finish;
		}
#endif

#ifndef IL_NO_TPL
		if (!iStrCmp(Ext, IL_TEXT("tpl"))) {
			bRet = ilLoadTpl(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_UTX
		if (!iStrCmp(Ext, IL_TEXT("utx"))) {
			bRet = ilLoadUtx(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_VTF
		if (!iStrCmp(Ext, IL_TEXT("vtf"))) {
			bRet = ilLoadVtf(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_WAL
		if (!iStrCmp(Ext, IL_TEXT("wal"))) {
			bRet = ilLoadWal(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_WBMP
		if (!iStrCmp(Ext, IL_TEXT("wbmp"))) {
			bRet = ilLoadWbmp(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_WDP
		if (!iStrCmp(Ext, IL_TEXT("wdp")) || !iStrCmp(Ext, IL_TEXT("hdp"))) {
			bRet = ilLoadWdp(context, FileName);
			goto finish;
		}
#endif

#ifndef IL_NO_XPM
		if (!iStrCmp(Ext, IL_TEXT("xpm"))) {
			bRet = ilLoadXpm(context, FileName);
			goto finish;
		}
#endif
	}

	// As a last-ditch effort, try to identify the image
	Type = ilDetermineType(context, FileName);
	if (Type == IL_TYPE_UNKNOWN) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return IL_FALSE;
	}
	return ilLoad(context, Type, FileName);

finish:
	return bRet;
}


//! Attempts to save an image to a file.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
IL_VTF, IL_WBMP and IL_JASC_PAL.
\param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
the filename to save to.
\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILboolean ILAPIENTRY ilSave(ILcontext* context, ILenum Type, ILconst_string FileName)
{
	switch (Type)
	{
	case IL_TYPE_UNKNOWN:
		return ilSaveImage(context, FileName);

#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_CHEAD
	case IL_CHEAD:
		return ilSaveCHeader(context, FileName, (char*)"IL_IMAGE");
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilSaveExr(context, FileName);
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.save(FileName);
	}
#endif

	/*#ifndef IL_NO_KTX
	case IL_KTX:
	return ilSaveKtx(context, FileName);
	#endif*/

#ifndef IL_NO_PCX
	case IL_PCX:
	{
		PcxHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilSaveSgi(context, FileName);
#endif

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.save(FileName);
	}
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilSaveVtf(context, FileName);
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		return ilSaveWbmp(context, FileName);
#endif

	case IL_JASC_PAL:
		return ilSaveJascPal(context, FileName);
	}

	ilSetError(context, IL_INVALID_ENUM);
	return IL_FALSE;
}


//! Attempts to save an image to a file stream.  The file format is specified by the user.
/*! \param Type Format of this file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
IL_VTF, IL_WBMP and IL_JASC_PAL.
\param File File stream to save to.
\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILuint ILAPIENTRY ilSaveF(ILcontext* context, ILenum Type, ILHANDLE File)
{
	ILboolean Ret;

	if (File == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return 0;
	}

	switch (Type)
	{
#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		Ret = ilSaveExrF(context, File);
		break;
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	case IL_JPG:
	{
		JpegHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		Ret = ilSaveSgiF(context, File);
		break;
#endif

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		Ret = ilSaveVtfF(context, File);
		break;
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		Ret = ilSaveWbmpF(context, File);
		break;
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		Ret = handler.saveF(File);
	}
	break;
#endif

	default:
		ilSetError(context, IL_INVALID_ENUM);
		return 0;
	}

	if (Ret == IL_FALSE)
		return 0;

	return context->impl->itellw(context);
}


//! Attempts to save an image to a memory buffer.  The file format is specified by the user.
/*! \param Type Format of this image file.  Acceptable values are IL_BMP, IL_CHEAD, IL_DDS, IL_EXR,
IL_HDR, IL_JP2, IL_JPG, IL_PCX, IL_PNG, IL_PNM, IL_PSD, IL_RAW, IL_SGI, IL_TGA, IL_TIF,
IL_VTF, IL_WBMP and IL_JASC_PAL.
\param Lump Memory buffer to save to
\param Size Size of the memory buffer
\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILuint ILAPIENTRY ilSaveL(ILcontext* context, ILenum Type, void *Lump, ILuint Size)
{
	if (Lump == nullptr) {
		if (Size != 0) {
			ilSetError(context, IL_INVALID_PARAM);
			return 0;
		}
		// The user wants to know how large of a buffer they need.
		else {
			return ilDetermineSize(context, Type);
		}
	}

	switch (Type)
	{
#ifndef IL_NO_BMP
	case IL_BMP:
	{
		BmpHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_EXR
	case IL_EXR:
		return ilSaveExrL(context, Lump, Size);
#endif

#ifndef IL_NO_HDR
	case IL_HDR:
	{
		HdrHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_JP2
	case IL_JP2:
	{
		Jp2Handler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_JPG
	case IL_JPG:
	{
		JpegHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_PNG
	case IL_PNG:
	{
		PngHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_PNM
	case IL_PNM:
	{
		PnmHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_PSD
	case IL_PSD:
	{
		PsdHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_RAW
	case IL_RAW:
	{
		RawHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_SGI
	case IL_SGI:
		return ilSaveSgiL(context, Lump, Size);
#endif

#ifndef IL_NO_TGA
	case IL_TGA:
	{
		TargaHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_DDS
	case IL_DDS:
	{
		DdsHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif

#ifndef IL_NO_VTF
	case IL_VTF:
		return ilSaveVtfL(context, Lump, Size);
#endif

#ifndef IL_NO_WBMP
	case IL_WBMP:
		return ilSaveWbmpL(context, Lump, Size);
#endif

#ifndef IL_NO_TIF
	case IL_TIF:
	{
		TiffHandler handler(context);

		return handler.saveL(Lump, Size);
	}
#endif
	}

	ilSetError(context, IL_INVALID_ENUM);
	return 0;
}


//! Saves the current image based on the extension given in FileName.
/*! \param FileName Ansi or Unicode string, depending on the compiled version of DevIL, that gives
the filename to save to.
\return Boolean value of failure or success.  Returns IL_FALSE if saving failed.*/
ILboolean ILAPIENTRY ilSaveImage(ILcontext* context, ILconst_string FileName)
{
	ILstring Ext;
	ILboolean	bRet = IL_FALSE;

	if (FileName == nullptr || ilStrLen(FileName) < 1) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (context->impl->iCurImage == nullptr) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Ext = iGetExtension(FileName);
	if (Ext == nullptr) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

#ifndef IL_NO_BMP
	if (!iStrCmp(Ext, IL_TEXT("bmp")))
	{
		BmpHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_CHEAD
	if (!iStrCmp(Ext, IL_TEXT("h"))) {
		bRet = ilSaveCHeader(context, FileName, (char*)"IL_IMAGE");
		goto finish;
	}
#endif

#ifndef IL_NO_DDS
	if (!iStrCmp(Ext, IL_TEXT("dds")))
	{
		DdsHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_EXR
	if (!iStrCmp(Ext, IL_TEXT("exr"))) {
		bRet = ilSaveExr(context, FileName);
		goto finish;
	}
#endif

#ifndef IL_NO_HDR
	if (!iStrCmp(Ext, IL_TEXT("hdr")))
	{
		HdrHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_JP2
	if (!iStrCmp(Ext, IL_TEXT("jp2")))
	{
		Jp2Handler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_JPG
	if (!iStrCmp(Ext, IL_TEXT("jpg")) || !iStrCmp(Ext, IL_TEXT("jpeg")) || !iStrCmp(Ext, IL_TEXT("jpe")))
	{
		JpegHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_PCX
	if (!iStrCmp(Ext, IL_TEXT("pcx")))
	{
		PcxHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_PNG
	if (!iStrCmp(Ext, IL_TEXT("png")))
	{
		PngHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_PNM  // Not sure if binary or ascii should be defaulted...maybe an option?
	if (!iStrCmp(Ext, IL_TEXT("pbm")) || !iStrCmp(Ext, IL_TEXT("pgm")) || 
		!iStrCmp(Ext, IL_TEXT("ppm")))
	{
		PnmHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_PSD
	if (!iStrCmp(Ext, IL_TEXT("psd")))
	{
		PsdHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_RAW
	if (!iStrCmp(Ext, IL_TEXT("raw")))
	{
		RawHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_SGI
	if (!iStrCmp(Ext, IL_TEXT("sgi")) || !iStrCmp(Ext, IL_TEXT("bw")) ||
		!iStrCmp(Ext, IL_TEXT("rgb")) || !iStrCmp(Ext, IL_TEXT("rgba"))) {
		bRet = ilSaveSgi(context, FileName);
		goto finish;
	}
#endif

#ifndef IL_NO_TGA
	if (!iStrCmp(Ext, IL_TEXT("tga")))
	{
		TargaHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_TIF
	if (!iStrCmp(Ext, IL_TEXT("tif")) || !iStrCmp(Ext, IL_TEXT("tiff")))
	{
		TiffHandler handler(context);

		bRet = handler.save(FileName);

		goto finish;
	}
#endif

#ifndef IL_NO_VTF
	if (!iStrCmp(Ext, IL_TEXT("vtf"))) {
		bRet = ilSaveVtf(context, FileName);
		goto finish;
	}
#endif

#ifndef IL_NO_WBMP
	if (!iStrCmp(Ext, IL_TEXT("wbmp"))) {
		bRet = ilSaveWbmp(context, FileName);
		goto finish;
	}
#endif

#ifndef IL_NO_MNG
	if (!iStrCmp(Ext, IL_TEXT("mng"))) {
		bRet = ilSaveMng(context, FileName);
		goto finish;
	}
#endif

	// Check if we just want to save the palette.
	if (!iStrCmp(Ext, IL_TEXT("pal"))) {
		bRet = ilSavePal(context, FileName);
		goto finish;
	}

	// Try registered procedures
	if (iRegisterSave(context, FileName))
		return IL_TRUE;

	ilSetError(context, IL_INVALID_EXTENSION);
	return IL_FALSE;

finish:
	return bRet;
}