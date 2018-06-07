//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_exr.cpp
//
// Description: Reads from an OpenEXR (.exr) file using the OpenEXR library.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_EXR

#ifndef HAVE_CONFIG_H // We are probably on a Windows box .
//#define OPENEXR_DLL
#define HALF_EXPORTS
#endif //HAVE_CONFIG_H

#include <ImfRgba.h>
#include <ImfArray.h>
#include <ImfRgbaFile.h>
//#include <ImfTiledRgbaFile.h>
//#include <ImfInputFile.h>
//#include <ImfTiledInputFile.h>
//#include <ImfPreviewImage.h>
//#include <ImfChannelList.h>

#include "il_exr.h"

#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "openexr.lib")
		#else
			#pragma comment(lib, "openexr-d.lib")
		#endif
	#endif
#endif

#include <ImfIO.h>

typedef struct EXRHEAD
{
	ILuint		MagicNumber;		// File signature (0x76, 0x2f, 0x31, 0x01)
	ILuint		Version;			// Treated as two bitfields
} IL_PACKSTRUCT EXRHEAD;

//@TODO: Should I just do these as enums?
#define EXR_UINT 0
#define EXR_HALF 1
#define EXR_FLOAT 2

#define EXR_NO_COMPRESSION    0
#define EXR_RLE_COMPRESSION   1
#define EXR_ZIPS_COMPRESSION  2
#define EXR_ZIP_COMPRESSION   3
#define EXR_PIZ_COMPRESSION   4
#define EXR_PXR24_COMPRESSION 5
#define EXR_B44_COMPRESSION   6
#define EXR_B44A_COMPRESSION  7

#ifdef __cplusplus
extern "C" {
#endif

ILboolean iCheckExr(EXRHEAD *Header);

#ifdef __cplusplus
}
#endif

class ilIStream : public Imf::IStream
{
protected:
	ILcontext * context;

public:
	ilIStream(ILcontext * context/*, ILHANDLE Handle*/);
	virtual bool	read(char c[/*n*/], int n);
	// I don't think I need this one, since we are taking care of the file handles ourselves.
	//virtual char *	readMemoryMapped (int n);
	virtual Imf::Int64	tellg();
	virtual void	seekg(Imf::Int64 Pos);
	virtual void	clear();
};

class ilOStream : public Imf::OStream
{
protected:
	ILcontext * context;

public:
	ilOStream(ILcontext * context/*, ILHANDLE Handle*/);
	virtual void	write(const char c[/*n*/], int n);
	// I don't think I need this one, since we are taking care of the file handles ourselves.
	//virtual char *	readMemoryMapped (int n);
	virtual Imf::Int64	tellp();
	virtual void	seekp(Imf::Int64 Pos);
};

ExrHandler::ExrHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid EXR file.
ILboolean ExrHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	ExrFile;
	ILboolean	bExr = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("exr"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bExr;
	}
	
	ExrFile = context->impl->iopenr(FileName);
	if (ExrFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bExr;
	}
	
	bExr = isValidF(ExrFile);
	context->impl->icloser(ExrFile);
	
	return bExr;
}

//! Checks if the ILHANDLE contains a valid EXR file at the current position.
ILboolean ExrHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Checks if Lump is a valid EXR lump.
ILboolean ExrHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function used to get the EXR header from the current file.
ILboolean iGetExrHead(ILcontext* context, EXRHEAD *Header)
{
	Header->MagicNumber = GetLittleUInt(context);
	Header->Version = GetLittleUInt(context);

	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean ExrHandler::isValidInternal()
{
	EXRHEAD Head;

	if (!iGetExrHead(context, &Head))
		return IL_FALSE;
	context->impl->iseek(context, -8, IL_SEEK_CUR);
	
	return iCheckExr(&Head);
}

// Internal function used to check if the HEADER is a valid EXR header.
ILboolean iCheckExr(EXRHEAD *Header)
{
	// The file magic number (signature) is 0x76, 0x2f, 0x31, 0x01
	if (Header->MagicNumber != 0x01312F76)
		return IL_FALSE;
	// The only valid version so far is version 2.  The upper value has
	//  to do with tiling.
	if (Header->Version != 0x002 && Header->Version != 0x202)
		return IL_FALSE;

	return IL_TRUE;
}

// Nothing to do here in the constructor.
ilIStream::ilIStream(ILcontext* context) :
	Imf::IStream("N/A"), context(context)
{
	return;
}

bool ilIStream::read(char c[], int n)
{
	if (context->impl->iread(context, c, 1, n) != n)
		return false;
	return true;
}

//@TODO: Make this work with 64-bit values.
Imf::Int64 ilIStream::tellg()
{
	Imf::Int64 Pos;

	// itell only returns a 32-bit value!
	Pos = context->impl->itell(context);

	return Pos;
}

// Note that there is no return value here, even though there probably should be.
//@TODO: Make this work with 64-bit values.
void ilIStream::seekg(Imf::Int64 Pos)
{
	// iseek only uses a 32-bit value!
	context->impl->iseek(context, (ILint)Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
	return;
}

void ilIStream::clear()
{
	return;
}

//! Reads an .exr file.
ILboolean ExrHandler::load(ILconst_string FileName)
{
	ILHANDLE	ExrFile;
	ILboolean	bExr = IL_FALSE;
	
	ExrFile = context->impl->iopenr(FileName);
	if (ExrFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bExr;
	}

	bExr = loadF(ExrFile);
	context->impl->icloser(ExrFile);

	return bExr;
}

//! Reads an already-opened .exr file
ILboolean ExrHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains an .exr
ILboolean ExrHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

ILboolean ExrHandler::loadInternal()
{
	Imf::Array<Imf::Rgba> pixels;
	Imath::Box2i dataWindow;
	float pixelAspectRatio;
	ILfloat *FloatData;

	ilIStream File(context);
	Imf::RgbaInputFile in(File);

	Imf::Rgba a;
    dataWindow = in.dataWindow();
    pixelAspectRatio = in.pixelAspectRatio();

    int dw, dh, dx, dy;
 
	dw = dataWindow.max.x - dataWindow.min.x + 1;
    dh = dataWindow.max.y - dataWindow.min.y + 1;
    dx = dataWindow.min.x;
    dy = dataWindow.min.y;

    pixels.resizeErase (dw * dh);
	in.setFrameBuffer (pixels - dx - dy * dw, 1, dw);

	try
    {
		in.readPixels (dataWindow.min.y, dataWindow.max.y);
    }
    catch (const std::exception &e)
    {
	// If some of the pixels in the file cannot be read,
	// print an error message, and return a partial image
	// to the caller.
		ilSetError(context, IL_LIB_EXR_ERROR);  // Could I use something a bit more descriptive based on e?
		e;  // Prevent the compiler from yelling at us about this being unused.
		return IL_FALSE;
    }

	//if (ilTexImage(dw, dh, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL) == IL_FALSE)
	//if (ilTexImage(dw, dh, 1, 4, IL_RGBA, IL_UNSIGNED_SHORT, NULL) == IL_FALSE)
	if (ilTexImage(context, dw, dh, 1, 4, IL_RGBA, IL_FLOAT, NULL) == IL_FALSE)
		return IL_FALSE;

	// Determine where the origin is in the original file.
	if (in.lineOrder() == Imf::INCREASING_Y)
		context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;
	else
		context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;

	// Better to access FloatData instead of recasting everytime.
	FloatData = (ILfloat*)context->impl->iCurImage->Data;

	for (int i = 0; i < dw * dh; i++)
	{
		// Too much data lost
		//context->impl->iCurImage->Data[i * 4 + 0] = (ILubyte)(pixels[i].r.bits() >> 8);
		//context->impl->iCurImage->Data[i * 4 + 1] = (ILubyte)(pixels[i].g.bits() >> 8);
		//context->impl->iCurImage->Data[i * 4 + 2] = (ILubyte)(pixels[i].b.bits() >> 8);
		//context->impl->iCurImage->Data[i * 4 + 3] = (ILubyte)(pixels[i].a.bits() >> 8);

		// The images look kind of washed out with this.
		//((ILshort*)(context->impl->iCurImage->Data))[i * 4 + 0] = pixels[i].r.bits();
		//((ILshort*)(context->impl->iCurImage->Data))[i * 4 + 1] = pixels[i].g.bits();
		//((ILshort*)(context->impl->iCurImage->Data))[i * 4 + 2] = pixels[i].b.bits();
		//((ILshort*)(context->impl->iCurImage->Data))[i * 4 + 3] = pixels[i].a.bits();

		// This gives the best results, since no data is lost.
		FloatData[i * 4]     = pixels[i].r;
		FloatData[i * 4 + 1] = pixels[i].g;
		FloatData[i * 4 + 2] = pixels[i].b;
		FloatData[i * 4 + 3] = pixels[i].a;
	}

	// Converts the image to predefined type, format and/or origin if needed.
	return ilFixImage(context);
}

// Nothing to do here in the constructor.
ilOStream::ilOStream(ILcontext * context) :
	Imf::OStream("N/A"), context(context)
{
	return;
}

void ilOStream::write(const char c[], int n)
{
	context->impl->iwrite(context, c, 1, n);  //@TODO: Throw an exception here.
	return;
}

//@TODO: Make this work with 64-bit values.
Imf::Int64 ilOStream::tellp()
{
	Imf::Int64 Pos;

	// itellw only returns a 32-bit value!
	Pos = context->impl->itellw(context);

	return Pos;
}

// Note that there is no return value here, even though there probably should be.
//@TODO: Make this work with 64-bit values.
void ilOStream::seekp(Imf::Int64 Pos)
{
	// iseekw only uses a 32-bit value!
	context->impl->iseekw(context, (ILint)Pos, IL_SEEK_SET);  // I am assuming this is seeking from the beginning.
	return;
}

//! Writes a Exr file
ILboolean ExrHandler::save(const ILstring FileName)
{
	ILHANDLE	ExrFile;
	ILuint		ExrSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	ExrFile = context->impl->iopenw(FileName);
	if (ExrFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	ExrSize = saveF(ExrFile);
	context->impl->iclosew(ExrFile);

	if (ExrSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a Exr to an already-opened file
ILuint ExrHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a Exr to a memory "lump"
ILuint ExrHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos = context->impl->itellw(context);
	iSetOutputLump(context, Lump, Size);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

ILboolean ExrHandler::saveInternal()
{
	Imath::Box2i DataWindow(Imath::V2i(0, 0), Imath::V2i(context->impl->iCurImage->Width-1, context->impl->iCurImage->Height-1));
	Imf::LineOrder Order;
	if (context->impl->iCurImage->Origin == IL_ORIGIN_LOWER_LEFT)
		Order = Imf::DECREASING_Y;
	else
		Order = Imf::INCREASING_Y;
	Imf::Header Head(context->impl->iCurImage->Width, context->impl->iCurImage->Height, DataWindow, 1, Imath::V2f (0, 0), 1, Order);

	ilOStream File(context);
	Imf::RgbaOutputFile Out(File, Head);
	ILimage *TempImage = context->impl->iCurImage;

	//@TODO: Can we always assume that Rgba is packed the same?
	Imf::Rgba *HalfData = (Imf::Rgba*)ialloc(context, TempImage->Width * TempImage->Height * sizeof(Imf::Rgba));
	if (HalfData == NULL)
		return IL_FALSE;

	if (context->impl->iCurImage->Format != IL_RGBA || context->impl->iCurImage->Type != IL_FLOAT) {
		TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGBA, IL_FLOAT);
		if (TempImage == NULL) {
			ifree(HalfData);
			return IL_FALSE;
		}
	}

	ILuint Offset = 0;
	ILfloat *FloatPtr = (ILfloat*)TempImage->Data;
	for (unsigned int y = 0; y < TempImage->Height; y++) {
		for (unsigned int x = 0; x < TempImage->Width; x++) {
			HalfData[y * TempImage->Width + x].r = FloatPtr[Offset];
			HalfData[y * TempImage->Width + x].g = FloatPtr[Offset + 1];
			HalfData[y * TempImage->Width + x].b = FloatPtr[Offset + 2];
			HalfData[y * TempImage->Width + x].a = FloatPtr[Offset + 3];
			Offset += 4;  // 4 floats
		}
	}

	Out.setFrameBuffer(HalfData, 1, TempImage->Width);
	Out.writePixels(TempImage->Height);  //@TODO: Do each scanline separately to keep from using so much memory.

	// Free our half data.
	ifree(HalfData);
	// Destroy our temporary image if we used one.
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}

#endif //IL_NO_EXR