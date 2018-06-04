//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/14/2009
//
// Filename: src-IL/src/il_jpeg.cpp
//
// Description: Jpeg (.jpg) functions
//
//-----------------------------------------------------------------------------
//
// Most of the comments here are sufficient, as we're just using libjpeg.
//  I have left most of the libjpeg example's comments intact, though.
//

#include "il_internal.h"

#ifndef IL_NO_JPG
	#ifndef IL_USE_IJL
		#ifdef RGB_RED
			#undef RGB_RED
			#undef RGB_GREEN
			#undef RGB_BLUE
		#endif
		#define RGB_RED		0
		#define RGB_GREEN	1
		#define RGB_BLUE	2

		#if defined(_MSC_VER)
			#pragma warning(push)
			#pragma warning(disable : 4005)  // Redefinitions in
			#pragma warning(disable : 4142)  //  jmorecfg.h
		#endif
	#else
		#include <ijl.h>
		#include <limits.h>
	#endif

#include "il_jpeg.h"
#include <setjmp.h>

#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifdef IL_USE_IJL
			//pragma comment(lib, "ijl15.lib")
		#else
			#ifndef _DEBUG
				#pragma comment(lib, "libjpeg.lib")
			#else
				//#pragma comment(lib, "libjpeg-d.lib")
				#pragma comment(lib, "libjpeg.lib")
			#endif
		#endif//IL_USE_IJL
	#endif
#endif

JpegHandler::JpegHandler(ILcontext* context) :
	context(context)
{

}

// Internal function used to get the .jpg header from the current file.
void iGetJpgHead(ILcontext* context, ILubyte *Header)
{
	Header[0] = context->impl->igetc(context);
	Header[1] = context->impl->igetc(context);
	return;
}

// Internal function used to check if the HEADER is a valid .Jpg header.
ILboolean JpegHandler::check(ILubyte Header[2])
{
	if (Header[0] != 0xFF || Header[1] != 0xD8)
		return IL_FALSE;
	return IL_TRUE;
}

// Internal function to get the header and check it.
ILboolean JpegHandler::isValidInternal()
{
	ILubyte Head[2];

	iGetJpgHead(context, Head);
	context->impl->iseek(context, -2, IL_SEEK_CUR);  // Go ahead and restore to previous state

	return check(Head);
}

//! Checks if the file specified in FileName is a valid .jpg file.
ILboolean JpegHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	JpegFile;
	ILboolean	bJpeg = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("jpg")) &&
		!iCheckExtension(FileName, IL_TEXT("jpe")) &&
		!iCheckExtension(FileName, IL_TEXT("jpeg")) &&
		!iCheckExtension(FileName, IL_TEXT("jif")) &&
		!iCheckExtension(FileName, IL_TEXT("jfif")))
	{
		ilSetError(context, IL_INVALID_EXTENSION);
		return bJpeg;
	}

	JpegFile = context->impl->iopenr(FileName);
	if (JpegFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bJpeg;
	}

	bJpeg = isValidF(JpegFile);
	context->impl->icloser(JpegFile);

	return bJpeg;
}

//! Checks if the ILHANDLE contains a valid .jpg file at the current position.
ILboolean JpegHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

ILboolean JpegHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

#ifndef IL_USE_IJL // Use libjpeg instead of the IJL.

typedef struct {
	struct jpeg_error_mgr pub;	/* public fields */

	JpegHandler* handler;
} error_mgr;

typedef error_mgr* ierror_ptr;

// Overrides libjpeg's stupid error/warning handlers. =P
//void ExitErrorHandle (struct jpeg_common_struct *JpegInfo)
void JpegHandler::ExitErrorHandle(j_common_ptr cinfo)
{
	ierror_ptr err = (ierror_ptr)cinfo->err;
	ilSetError(err->handler->context, IL_LIB_JPEG_ERROR);
	err->handler->jpgErrorOccured = IL_TRUE;
	return;
}

void OutputMsg(struct jpeg_common_struct *JpegInfo)
{
	return;
}

//! Reads a jpeg file
ILboolean JpegHandler::load(ILconst_string FileName)
{
	ILHANDLE	JpegFile;
	ILboolean	bJpeg = IL_FALSE;

	JpegFile = context->impl->iopenr(FileName);
	if (JpegFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bJpeg;
	}

	bJpeg = loadF(JpegFile);
	context->impl->icloser(JpegFile);

	return bJpeg;
}

//! Reads an already-opened jpeg file
ILboolean JpegHandler::loadF(ILHANDLE File)
{
	ILboolean	bRet;
	ILuint		FirstPos;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

// Reads from a memory "lump" containing a jpeg
ILboolean JpegHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */

  JOCTET * buffer;		/* start of buffer */
  boolean start_of_file;	/* have we gotten any data yet? */
  JpegHandler* handler;
} iread_mgr;

typedef iread_mgr* iread_ptr;

#define INPUT_BUF_SIZE  4096  // choose an efficiently iread'able size

static void init_source(j_decompress_ptr cinfo)
{
	iread_ptr src = (iread_ptr)cinfo->src;
	src->start_of_file = (boolean)IL_TRUE;
}

boolean JpegHandler::fill_input_buffer(j_decompress_ptr cinfo)
{
	iread_ptr src = (iread_ptr)cinfo->src;
	ILint nbytes;

	nbytes = src->handler->context->impl->iread(src->handler->context, src->buffer, 1, INPUT_BUF_SIZE);

	if (nbytes <= 0) {
		if (src->start_of_file) {  // Treat empty input file as fatal error
			//ERREXIT(cinfo, JERR_INPUT_EMPTY);
			src->handler->jpgErrorOccured = IL_TRUE;
		}
		//WARNMS(cinfo, JWRN_JPEG_EOF);
		// Insert a fake EOI marker
		src->buffer[0] = (JOCTET)0xFF;
		src->buffer[1] = (JOCTET)JPEG_EOI;
		nbytes = 2;
		return (boolean)IL_FALSE;
	}
	if (nbytes < INPUT_BUF_SIZE) {
		ilGetError(src->handler->context);  // Gets rid of the IL_FILE_READ_ERROR.
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = (boolean)IL_FALSE;

	return (boolean)IL_TRUE;
}

void JpegHandler::skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	iread_ptr src = (iread_ptr)cinfo->src;

	if (num_bytes > 0) {
		while (num_bytes > (long)src->pub.bytes_in_buffer) {
			num_bytes -= (long)src->pub.bytes_in_buffer;
			(void)fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t)num_bytes;
		src->pub.bytes_in_buffer -= (size_t)num_bytes;
	}
}

static void term_source(j_decompress_ptr cinfo)
{
	// no work necessary here
}

void JpegHandler::devil_jpeg_read_init(j_decompress_ptr cinfo)
{
	iread_ptr src;

	if (cinfo->src == NULL) {  // first time for this JPEG object?
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(iread_mgr));
		src = (iread_ptr)cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
				INPUT_BUF_SIZE * sizeof(JOCTET));
	}

	src = (iread_ptr)cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;  // use default method
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = 0;  // forces fill_input_buffer on first read
	src->pub.next_input_byte = NULL;  // until buffer loaded
	src->handler = this;
}

void JpegHandler::iJpegErrorExit(j_common_ptr cinfo)
{
	ierror_ptr err = (ierror_ptr)cinfo->err;
	ilSetError(err->handler->context, IL_LIB_JPEG_ERROR);
	jpeg_destroy(cinfo);
	longjmp(err->handler->context->impl->jumpBuffer, 1);
}

// Internal function used to load the jpeg.
ILboolean JpegHandler::loadInternal()
{
	ierror_ptr Error = new error_mgr();
	struct jpeg_decompress_struct	JpegInfo;
	ILboolean						result;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	JpegInfo.err = jpeg_std_error(&Error->pub);		// init standard error handlers
	Error->pub.error_exit = iJpegErrorExit;				// add our exit handler
	Error->pub.output_message = OutputMsg;

	if ((result = setjmp(context->impl->jumpBuffer) == 0) != IL_FALSE) {
		jpeg_create_decompress(&JpegInfo);
		JpegInfo.do_block_smoothing = (boolean)IL_TRUE;
		JpegInfo.do_fancy_upsampling = (boolean)IL_TRUE;

		//jpeg_stdio_src(&JpegInfo, iGetFile());

		devil_jpeg_read_init(&JpegInfo);
		jpeg_read_header(&JpegInfo, (boolean)IL_TRUE);

		result = loadFromJpegStruct(&JpegInfo);

		jpeg_finish_decompress(&JpegInfo);
		jpeg_destroy_decompress(&JpegInfo);
	}
	else
	{
		jpeg_destroy_decompress(&JpegInfo);
	}

	//return ilFixImage(context);  // No need to call it again (called first in ilLoadFromJpegStruct).
	return result;
}

typedef struct
{
	struct jpeg_destination_mgr		pub;
	JOCTET							*buffer;
	ILboolean						bah;
	ILcontext* context;
} iwrite_mgr;

typedef iwrite_mgr* iwrite_ptr;

#define OUTPUT_BUF_SIZE 4096

static void init_destination(j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->buffer = (JOCTET *)
		(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
			OUTPUT_BUF_SIZE * sizeof(JOCTET));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->context->impl->iwrite(dest->context, dest->buffer, 1, OUTPUT_BUF_SIZE);
	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	return (boolean)IL_TRUE;
}

static void term_destination(j_compress_ptr cinfo)
{
	iwrite_ptr dest = (iwrite_ptr)cinfo->dest;
	dest->context->impl->iwrite(dest->context, dest->buffer, 1, OUTPUT_BUF_SIZE - (ILuint)dest->pub.free_in_buffer);
	return;
}

void devil_jpeg_write_init(ILcontext* context, j_compress_ptr cinfo)
{
	iwrite_ptr dest;

	if (cinfo->dest == NULL) {	// first time for this JPEG object?
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
				sizeof(iwrite_mgr));
		dest = (iwrite_ptr)cinfo->dest;
	}

	dest = (iwrite_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->context = context;

	return;
}

//! Writes a Jpeg file
ILboolean JpegHandler::save(const ILstring FileName)
{
	ILHANDLE	JpegFile;
	ILuint		JpegSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	JpegFile = context->impl->iopenw(FileName);
	if (JpegFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	JpegSize = saveF(JpegFile);
	context->impl->iclosew(JpegFile);

	if (JpegSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a Jpeg to an already-opened file
ILuint JpegHandler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a Jpeg to a memory "lump"
ILuint JpegHandler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Internal function used to save the Jpeg.
ILboolean JpegHandler::saveInternal()
{
	struct		jpeg_compress_struct JpegInfo;
	struct		jpeg_error_mgr Error;
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	ILenum		Type = 0;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	/*if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Quality = 85;  // Not sure how low we should dare go...
	else
		Quality = 99;*/

	if ((context->impl->iCurImage->Format != IL_RGB && context->impl->iCurImage->Format != IL_LUMINANCE) || context->impl->iCurImage->Bpc != 1) {
		TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = context->impl->iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	JpegInfo.err = jpeg_std_error(&Error);
	// Now we can initialize the JPEG compression object.
	jpeg_create_compress(&JpegInfo);

	//jpeg_stdio_dest(&JpegInfo, JpegFile);
	devil_jpeg_write_init(context, &JpegInfo);

	JpegInfo.image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo.image_height = TempImage->Height;
	JpegInfo.input_components = TempImage->Bpp;  // # of color components per pixel

	// John Villar's addition
	if (TempImage->Bpp == 1)
		JpegInfo.in_color_space = JCS_GRAYSCALE;
	else
		JpegInfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&JpegInfo);

	/*#ifndef IL_USE_JPEGLIB_UNMODIFIED
		Type = iGetInt(IL_JPG_SAVE_FORMAT);
		if (Type == IL_EXIF) {
			JpegInfo.write_JFIF_header = FALSE;
			JpegInfo.write_EXIF_header = TRUE;
		}
		else if (Type == IL_JFIF) {
			JpegInfo.write_JFIF_header = TRUE;
			JpegInfo.write_EXIF_header = FALSE;
		} //EXIF not present in libjpeg...
	#else*/
	Type = Type;
	JpegInfo.write_JFIF_header = (boolean)TRUE;
	//#endif//IL_USE_JPEGLIB_UNMODIFIED

		// Set the quality output
	jpeg_set_quality(&JpegInfo, iGetInt(context, IL_JPG_QUALITY), (boolean)IL_TRUE);
	// Sets progressive saving here
	if (ilGetBoolean(context, IL_JPG_PROGRESSIVE))
		jpeg_simple_progression(&JpegInfo);

	jpeg_start_compress(&JpegInfo, (boolean)IL_TRUE);

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo.next_scanline < JpegInfo.image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo.next_scanline * TempImage->Bps];
		(void)jpeg_write_scanlines(&JpegInfo, row_pointer, 1);
	}

	// Step 6: Finish compression
	jpeg_finish_compress(&JpegInfo);

	// Step 7: release JPEG compression object

	// This is an important step since it will release a good deal of memory.
	jpeg_destroy_compress(&JpegInfo);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}

#else // Use the IJL instead of libjpeg.

//! Reads a jpeg file
ILboolean JpegHandler::load(ILconst_string FileName)
{
	if (!iFileExists(FileName)) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}
	return iLoadJpegInternal(FileName, NULL, 0);
}

// Reads from a memory "lump" containing a jpeg
ILboolean JpegHandler::loadL(ILcontext* context, void *Lump, ILuint Size)
{
	return iLoadJpegInternal(NULL, Lump, Size);
}

// Internal function used to load the jpeg.
ILboolean JpegHandler::iLoadJpegInternal(ILstring FileName, void *Lump, ILuint Size)
{
    JPEG_CORE_PROPERTIES Image;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (ijlInit(&Image) != IJL_OK) {
		ilSetError(context, IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlRead(&Image, IJL_JFILE_READPARAMS) != IJL_OK) {
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size > 0 ? Size : UINT_MAX;
		if (ijlRead(&Image, IJL_JBUFF_READPARAMS) != IJL_OK) {
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	switch (Image.JPGChannels)
	{
		case 1:
			Image.JPGColor		= IJL_G;
			Image.DIBChannels	= 1;
			Image.DIBColor		= IJL_G;
			context->impl->iCurImage->Format	= IL_LUMINANCE;
			break;

		case 3:
			Image.JPGColor		= IJL_YCBCR;
			Image.DIBChannels	= 3;
			Image.DIBColor		= IJL_RGB;
			context->impl->iCurImage->Format	= IL_RGB;
			break;

        case 4:
			Image.JPGColor		= IJL_YCBCRA_FPX;
			Image.DIBChannels	= 4;
			Image.DIBColor		= IJL_RGBA_FPX;
			context->impl->iCurImage->Format	= IL_RGBA;
			break;

        default:
			// This catches everything else, but no
			// color twist will be performed by the IJL.
			/*Image.DIBColor = (IJL_COLOR)IJL_OTHER;
			Image.JPGColor = (IJL_COLOR)IJL_OTHER;
			Image.DIBChannels = Image.JPGChannels;
			break;*/
			ijlFree(&Image);
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
	}

	if (!ilTexImage(context, Image.JPGWidth, Image.JPGHeight, 1, (ILubyte)Image.DIBChannels, context->impl->iCurImage->Format, IL_UNSIGNED_BYTE, NULL)) {
		ijlFree(&Image);
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	Image.DIBWidth		= Image.JPGWidth;
	Image.DIBHeight		= Image.JPGHeight;
	Image.DIBPadBytes	= 0;
	Image.DIBBytes		= context->impl->iCurImage->Data;

	if (FileName != NULL) {
		if (ijlRead(&Image, IJL_JFILE_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		if (ijlRead(&Image, IJL_JBUFF_READWHOLEIMAGE) != IJL_OK) {
			ijlFree(&Image);
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);
	return ilFixImage(context);
}

//! Writes a Jpeg file
ILboolean JpegHandler::save(ILconst_string FileName)
{
	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	return saveInternal(FileName, NULL, 0);
}


//! Writes a Jpeg to a memory "lump"
ILboolean JpegHandler::saveL(void *Lump, ILuint Size)
{
	return saveInternal(NULL, Lump, Size);
}


// Internal function used to save the Jpeg.
ILboolean JpegHandler::saveInternal(ILstring FileName, void *Lump, ILuint Size)
{
	JPEG_CORE_PROPERTIES	Image;
	ILuint	Quality;
	ILimage	*TempImage;
	ILubyte	*TempData;

	imemclear(&Image, sizeof(JPEG_CORE_PROPERTIES));

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (FileName == NULL && Lump == NULL) {
		ilSetError(context, IL_INVALID_PARAM);
		return IL_FALSE;
	}

	if (iGetHint(IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Quality = 85;  // Not sure how low we should dare go...
	else
		Quality = 99;

	if (ijlInit(&Image) != IJL_OK) {
		ilSetError(context, IL_LIB_JPEG_ERROR);
		return IL_FALSE;
	}

	if ((context->impl->iCurImage->Format != IL_RGB && context->impl->iCurImage->Format != IL_RGBA && context->impl->iCurImage->Format != IL_LUMINANCE)
		|| context->impl->iCurImage->Bpc != 1) {
		if (context->impl->iCurImage->Format == IL_BGRA)
			Temp = iConvertImage(context->impl->iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			Temp = iConvertImage(context->impl->iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (Temp == NULL) {
			return IL_FALSE;
		}
	}
	else {
		Temp = context->impl->iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	// Setup DIB
	Image.DIBWidth		= TempImage->Width;
	Image.DIBHeight		= TempImage->Height;
	Image.DIBChannels	= TempImage->Bpp;
	Image.DIBBytes		= TempData;
	Image.DIBPadBytes	= 0;

	// Setup JPEG
	Image.JPGWidth		= TempImage->Width;
	Image.JPGHeight		= TempImage->Height;
	Image.JPGChannels	= TempImage->Bpp;

	switch (Temp->Bpp)
	{
		case 1:
			Image.DIBColor			= IJL_G;
			Image.JPGColor			= IJL_G;
			Image.JPGSubsampling	= IJL_NONE;
			break;
		case 3:
			Image.DIBColor			= IJL_RGB;
			Image.JPGColor			= IJL_YCBCR;
			Image.JPGSubsampling	= IJL_411;
			break;
		case 4:
			Image.DIBColor			= IJL_RGBA_FPX;
			Image.JPGColor			= IJL_YCBCRA_FPX;
			Image.JPGSubsampling	= IJL_4114;
			break;
	}

	if (FileName != NULL) {
		Image.JPGFile = FileName;
		if (ijlWrite(&Image, IJL_JFILE_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}
	else {
		Image.JPGBytes = Lump;
		Image.JPGSizeBytes = Size;
		if (ijlWrite(&Image, IJL_JBUFF_WRITEWHOLEIMAGE) != IJL_OK) {
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			ilSetError(context, IL_LIB_JPEG_ERROR);
			return IL_FALSE;
		}
	}

	ijlFree(&Image);

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (Temp != context->impl->iCurImage)
		ilCloseImage(Temp);

	return IL_TRUE;
}

#endif//IL_USE_IJL

// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The decompressor must be set up with an input source and all desired parameters
// this function is called. The caller must call jpeg_finish_decompress because
// the caller may still need decompressor after calling this for e.g. examining
// saved markers.
ILboolean JpegHandler::loadFromJpegStruct(void *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	// sam. void (*errorHandler)(j_common_ptr);
	ILubyte	*TempPtr[1];
	ILuint	Returned;
	j_decompress_ptr JpegInfo = (j_decompress_ptr)_JpegInfo;

	//added on 2003-08-31 as explained in sf bug 596793
	jpgErrorOccured = IL_FALSE;

	// sam. errorHandler = JpegInfo->err->error_exit;
	// sam. JpegInfo->err->error_exit = ExitErrorHandle;
	jpeg_start_decompress((j_decompress_ptr)JpegInfo);

	if (!ilTexImage(context, JpegInfo->output_width, JpegInfo->output_height, 1, (ILubyte)JpegInfo->output_components, 0, IL_UNSIGNED_BYTE, NULL)) {
		return IL_FALSE;
	}
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	switch (context->impl->iCurImage->Bpp)
	{
	case 1:
		context->impl->iCurImage->Format = IL_LUMINANCE;
		break;
	case 3:
		context->impl->iCurImage->Format = IL_RGB;
		break;
	case 4:
		context->impl->iCurImage->Format = IL_RGBA;
		break;
	default:
		//@TODO: Anyway to get here?  Need to error out or something...
		break;
	}

	TempPtr[0] = context->impl->iCurImage->Data;
	while (JpegInfo->output_scanline < JpegInfo->output_height) {
		Returned = jpeg_read_scanlines(JpegInfo, TempPtr, 1);  // anyway to make it read all at once?
		TempPtr[0] += context->impl->iCurImage->Bps;
		if (Returned == 0)
			break;
	}

	// sam. JpegInfo->err->error_exit = errorHandler;

	if (jpgErrorOccured)
		return IL_FALSE;

	return ilFixImage(context);
#endif
#endif
	return IL_FALSE;
}

// Access point for applications wishing to use the jpeg library directly in
// conjunction with DevIL.
//
// The caller must set up the desired parameters by e.g. calling
// jpeg_set_defaults and overriding the parameters the caller wishes
// to change, such as quality, before calling this function. The caller
// is also responsible for calling jpeg_finish_compress in case the
// caller still needs to compressor for something.
// 
ILboolean JpegHandler::saveFromJpegStruct(void *_JpegInfo)
{
#ifndef IL_NO_JPG
#ifndef IL_USE_IJL
	void(*errorHandler)(j_common_ptr);
	JSAMPROW	row_pointer[1];
	ILimage		*TempImage;
	ILubyte		*TempData;
	j_compress_ptr JpegInfo = (j_compress_ptr)_JpegInfo;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//added on 2003-08-31 as explained in SF bug 596793
	jpgErrorOccured = IL_FALSE;

	errorHandler = JpegInfo->err->error_exit;
	JpegInfo->err->error_exit = ExitErrorHandle;


	if ((context->impl->iCurImage->Format != IL_RGB && context->impl->iCurImage->Format != IL_LUMINANCE) || context->impl->iCurImage->Bpc != 1) {
		TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = context->impl->iCurImage;
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(context, TempImage);
		if (TempData == NULL) {
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	JpegInfo->image_width = TempImage->Width;  // image width and height, in pixels
	JpegInfo->image_height = TempImage->Height;
	JpegInfo->input_components = TempImage->Bpp;  // # of color components per pixel

	jpeg_start_compress(JpegInfo, (boolean)IL_TRUE);

	//row_stride = image_width * 3;	// JSAMPLEs per row in image_buffer

	while (JpegInfo->next_scanline < JpegInfo->image_height) {
		// jpeg_write_scanlines expects an array of pointers to scanlines.
		// Here the array is only one element long, but you could pass
		// more than one scanline at a time if that's more convenient.
		row_pointer[0] = &TempData[JpegInfo->next_scanline * TempImage->Bps];
		(void)jpeg_write_scanlines(JpegInfo, row_pointer, 1);
	}

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT)
		ifree(TempData);
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return (!jpgErrorOccured);
#endif//IL_USE_IJL
#endif//IL_NO_JPG
	return IL_FALSE;
}

#if defined(_MSC_VER)
	#pragma warning(pop)
	//#pragma warning(disable : 4756)  // Disables 'named type definition in parentheses' warning
#endif

#endif//IL_NO_JPG