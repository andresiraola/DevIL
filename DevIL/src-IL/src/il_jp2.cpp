//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_jp2.cpp
//
// Description: Jpeg-2000 (.jp2) functions
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_JP2

#include <jasper/jasper.h>

#include "il_jp2.h"

#if defined(_WIN32) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "libjasper.lib")
		#else
			#pragma comment(lib, "libjasper.lib")  //libjasper-d.lib
		#endif
	#endif
#endif

ILboolean JasperInit = IL_FALSE;

ILboolean		loadInternal(ILcontext* context, jas_stream_t *Stream, ILimage *Image);
jas_stream_t	*iJp2ReadStream(ILcontext* context);

Jp2Handler::Jp2Handler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid .jp2 file.
ILboolean Jp2Handler::isValid(ILconst_string FileName)
{
	ILHANDLE	Jp2File;
	ILboolean	bJp2 = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("jp2")) && !iCheckExtension(FileName, IL_TEXT("jpx")) &&
		!iCheckExtension(FileName, IL_TEXT("j2k")) && !iCheckExtension(FileName, IL_TEXT("j2c"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bJp2;
	}

	Jp2File = context->impl->iopenr(FileName);
	if (Jp2File == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bJp2;
	}

	bJp2 = isValidF(Jp2File);
	context->impl->icloser(Jp2File);

	return bJp2;
}

//! Checks if the ILHANDLE contains a valid .jp2 file at the current position.
ILboolean Jp2Handler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Checks if Lump is a valid .jp2 lump.
ILboolean Jp2Handler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function to get the header and check it.
ILboolean Jp2Handler::isValidInternal()
{
	ILubyte Signature[4];

	context->impl->iseek(context, 4, IL_SEEK_CUR);  // Skip the 4 bytes that tell the size of the signature box.
	if (context->impl->iread(context, Signature, 1, 4) != 4) {
		context->impl->iseek(context, -4, IL_SEEK_CUR);
		return IL_FALSE;  // File read error
	}

	context->impl->iseek(context, -8, IL_SEEK_CUR);  // Restore to previous state

	// Signature is 'jP\040\040' by the specs (or 0x6A502020).
	//  http://www.jpeg.org/public/fcd15444-6.pdf
	if (Signature[0] != 0x6A || Signature[1] != 0x50 ||
		Signature[2] != 0x20 || Signature[3] != 0x20)
		return IL_FALSE;

	return IL_TRUE;
}

//! Reads a Jpeg2000 file.
ILboolean Jp2Handler::load(ILconst_string FileName)
{
	ILHANDLE	Jp2File;
	ILboolean	bRet;

	Jp2File = context->impl->iopenr(FileName);
	if (Jp2File == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	bRet = loadF(Jp2File);
	context->impl->icloser(Jp2File);

	return bRet;
}

//! Reads an already-opened Jpeg2000 file.
ILboolean Jp2Handler::loadF(ILHANDLE File)
{
	ILuint			FirstPos;
	ILboolean		bRet;
	jas_stream_t	*Stream;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);

	if (!JasperInit) {
		if (jas_init()) {
			ilSetError(context, IL_LIB_JP2_ERROR);
			return IL_FALSE;
		}
		JasperInit = IL_TRUE;
	}
	Stream = iJp2ReadStream(context);
	if (!Stream)
	{
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	bRet = loadInternal(context, Stream, NULL);
	// Close the input stream.
	jas_stream_close(Stream);

	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains a Jpeg2000 stream.
ILboolean Jp2Handler::loadL(const void *Lump, ILuint Size)
{
	return LoadLToImage(context, Lump, Size, NULL);
}

//! This is separated so that it can be called for other file types, such as .icns.
ILboolean Jp2Handler::LoadLToImage(ILcontext* context, const void *Lump, ILuint Size, ILimage *Image)
{
	ILboolean		bRet;
	jas_stream_t	*Stream;

	if (!JasperInit) {
		if (jas_init()) {
			ilSetError(context, IL_LIB_JP2_ERROR);
			return IL_FALSE;
		}
		JasperInit = IL_TRUE;
	}
	Stream = jas_stream_memopen((char*)Lump, Size);
	if (!Stream)
	{
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	bRet = loadInternal(context, Stream, Image);
	// Close the input stream.
	jas_stream_close(Stream);

	return bRet;
}

// Internal function used to load the Jpeg2000 stream.
ILboolean loadInternal(ILcontext* context, jas_stream_t	*Stream, ILimage *Image)
{
	jas_image_t		*Jp2Image = NULL;
	jas_matrix_t	*origdata;
	ILuint			x, y, c, Error;
	ILimage			*TempImage;

	// Decode image
	Jp2Image = jas_image_decode(Stream, -1, 0);
	if (!Jp2Image)
	{
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		jas_stream_close(Stream);
		return IL_FALSE;
	}

	// JasPer likes to buffer a lot, so it may try buffering past the end
	//  of the file.  iread naturally sets IL_FILE_READ_ERROR if it tries
	//  reading past the end of the file, but this actually is not an error.
	Error = ilGetError(context);
	// Put the error back if it is not IL_FILE_READ_ERROR.
	if (Error != IL_FILE_READ_ERROR)
		ilSetError(context, Error);


	// We're not supporting anything other than 8 bits/component yet.
	if (jas_image_cmptprec(Jp2Image, 0) != 8)
	{
		jas_image_destroy(Jp2Image);
		ilSetError(context, IL_ILLEGAL_FILE_VALUE);
		return IL_FALSE;
	}

	switch (jas_image_numcmpts(Jp2Image))
	{
		//@TODO: Can we do alpha data?  jas_image_cmpttype always returns 0 for this case.
		case 1:  // Assuming this is luminance data.
			if (Image == NULL) {
				ilTexImage(context, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
				TempImage = context->impl->iCurImage;
			}
			else {
				ifree(Image->Data);  // @TODO: Not really the most efficient way to do this...
				ilInitImage(context, Image, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, NULL);
				TempImage = Image;
			}
			break;

		case 2:  // Assuming this is luminance-alpha data.
			if (Image == NULL) {
				ilTexImage(context, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 2, IL_LUMINANCE_ALPHA, IL_UNSIGNED_BYTE, NULL);
				TempImage = context->impl->iCurImage;
			}
			else {
				ifree(Image->Data);  // @TODO: Not really the most efficient way to do this...
				ilInitImage(context, Image, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 2, IL_LUMINANCE_ALPHA, IL_UNSIGNED_BYTE, NULL);
				TempImage = Image;
			}
			break;

		case 3:
			if (Image == NULL) {
				ilTexImage(context, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
				TempImage = context->impl->iCurImage;
			}
			else {
				ifree(Image->Data);  // @TODO: Not really the most efficient way to do this...
				ilInitImage(context, Image, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);
				TempImage = Image;
			}
			break;
		case 4:
			if (Image == NULL) {
				ilTexImage(context, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
				TempImage = context->impl->iCurImage;
			}
			else {
				ifree(Image->Data);  // @TODO: Not really the most efficient way to do this...
				ilInitImage(context, Image, jas_image_width(Jp2Image), jas_image_height(Jp2Image), 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
				TempImage = Image;
			}
			break;
		default:
			jas_image_destroy(Jp2Image);
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			return IL_FALSE;
	}
	TempImage->Origin = IL_ORIGIN_UPPER_LEFT;

	// JasPer stores the data channels separately.
	//  I am assuming RGBA format.  Is it possible for other formats to be included?
	for (c = 0; c < TempImage->Bpp; c++)
	{
		origdata = jas_matrix_create(TempImage->Height, TempImage->Width);
		if (!origdata)
		{
			ilSetError(context, IL_LIB_JP2_ERROR);
			return IL_FALSE;  // @TODO: Error
		}
		// Have to convert data into an intermediate matrix format.
		if (jas_image_readcmpt(Jp2Image, c, 0, 0, TempImage->Width, TempImage->Height, origdata))
		{
			return IL_FALSE;
		}

		for (y = 0; y < TempImage->Height; y++)
		{
			for (x = 0; x < TempImage->Width; x++)
			{
				TempImage->Data[y * TempImage->Width * TempImage->Bpp + x * TempImage->Bpp + c] = origdata->data_[y * origdata->numcols_ + x];
			}
		}

		jas_matrix_destroy(origdata);
	}

	jas_image_destroy(Jp2Image);

	return ilFixImage(context);
}

static int iJp2_file_read(jas_stream_obj_t *obj, char *buf, int cnt)
{
	ILcontext* context = (ILcontext*)obj;
	return context->impl->iread(context, buf, 1, cnt);
}

static int iJp2_file_write(jas_stream_obj_t *obj, char *buf, int cnt)
{
	ILcontext* context = (ILcontext*)obj;
	return context->impl->iwrite(context, buf, 1, cnt);
}

static long iJp2_file_seek(jas_stream_obj_t *obj, long offset, int origin)
{
	ILcontext* context = (ILcontext*)obj;

	// We could just pass origin to iseek, but this is probably more portable.
	switch (origin)
	{
		case SEEK_SET:
			return context->impl->iseek(context, offset, IL_SEEK_SET);
		case SEEK_CUR:
			return context->impl->iseek(context, offset, IL_SEEK_CUR);
		case SEEK_END:
			return context->impl->iseek(context, offset, IL_SEEK_END);
	}
	return 0;  // Failed
}

static int iJp2_file_close(jas_stream_obj_t *obj)
{
	obj;
	return 0;  // We choose when we want to close the file.
}

static jas_stream_ops_t jas_stream_devilops = {
	iJp2_file_read,
	iJp2_file_write,
	iJp2_file_seek,
	iJp2_file_close
};

static jas_stream_t *jas_stream_create(void);
static void jas_stream_destroy(jas_stream_t *stream);
static void jas_stream_initbuf(jas_stream_t *stream, int bufmode );

// Modified version of jas_stream_fopen and jas_stream_memopen from jas_stream.c of JasPer
//  so that we can use our own file routines.
jas_stream_t *iJp2ReadStream(ILcontext* context)
{
	jas_stream_t *stream;

	if (!(stream = jas_stream_create())) {
		return 0;
	}

	/* A stream associated with a memory buffer is always opened
	for both reading and writing in binary mode. */
	stream->openmode_ = JAS_STREAM_READ | JAS_STREAM_BINARY;

	/* We use buffering whether it is from memory or a file. */
	jas_stream_initbuf(stream, JAS_STREAM_FULLBUF);

	/* Select the operations for a memory stream. */
	stream->ops_ = &jas_stream_devilops;
	stream->obj_ = (void *) context;

	// Shouldn't need any of this.

	///* If the buffer size specified is nonpositive, then the buffer
	//is allocated internally and automatically grown as needed. */
	//if (bufsize <= 0) {
	//	obj->bufsize_ = 1024;
	//	obj->growable_ = 1;
	//} else {
	//	obj->bufsize_ = bufsize;
	//	obj->growable_ = 0;
	//}
	//if (buf) {
	//	obj->buf_ = (unsigned char *) buf;
	//} else {
	//	obj->buf_ = jas_malloc(obj->bufsize_ * sizeof(char));
	//	obj->myalloc_ = 1;
	//}
	//if (!obj->buf_) {
	//	jas_stream_close(stream);
	//	return 0;
	//}

	//if (bufsize > 0 && buf) {
	//	/* If a buffer was supplied by the caller and its length is positive,
	//	  make the associated buffer data appear in the stream initially. */
	//	obj->len_ = bufsize;
	//} else {
	//	/* The stream is initially empty. */
	//	obj->len_ = 0;
	//}
	//obj->pos_ = 0;
	
	return stream;
}

// The following functions are taken directly from jas_stream.c of JasPer,
//  since they are designed to be used within JasPer only.

static void jas_stream_initbuf(jas_stream_t *stream, int bufmode )
{
	/* If this function is being called, the buffer should not have been
	  initialized yet. */
	assert(!stream->bufbase_);

	if (bufmode != JAS_STREAM_UNBUF) {
		/* The full- or line-buffered mode is being employed. */
        if ((stream->bufbase_ = (unsigned char*)jas_malloc(JAS_STREAM_BUFSIZE +
          JAS_STREAM_MAXPUTBACK))) {
            stream->bufmode_ |= JAS_STREAM_FREEBUF;
            stream->bufsize_ = JAS_STREAM_BUFSIZE;
        } else {
            /* The buffer allocation has failed.  Resort to unbuffered
              operation. */
            stream->bufbase_ = stream->tinybuf_;
            stream->bufsize_ = 1;
        }
	} else {
		/* The unbuffered mode is being employed. */
		/* Use a trivial one-character buffer. */
		stream->bufbase_ = stream->tinybuf_;
		stream->bufsize_ = 1;
	}
	stream->bufstart_ = &stream->bufbase_[JAS_STREAM_MAXPUTBACK];
	stream->ptr_ = stream->bufstart_;
	stream->cnt_ = 0;
	stream->bufmode_ |= bufmode & JAS_STREAM_BUFMODEMASK;
}

static jas_stream_t *jas_stream_create()
{
	jas_stream_t *stream;

	if (!(stream = (jas_stream_t*)jas_malloc(sizeof(jas_stream_t)))) {
		return 0;
	}
	stream->openmode_ = 0;
	stream->bufmode_ = 0;
	stream->flags_ = 0;
	stream->bufbase_ = 0;
	stream->bufstart_ = 0;
	stream->bufsize_ = 0;
	stream->ptr_ = 0;
	stream->cnt_ = 0;
	stream->ops_ = 0;
	stream->obj_ = 0;
	stream->rwcnt_ = 0;
	stream->rwlimit_ = -1;

	return stream;
}

static void jas_stream_destroy(jas_stream_t *stream)
{
	/* If the memory for the buffer was allocated with malloc, free
	this memory. */
	if ((stream->bufmode_ & JAS_STREAM_FREEBUF) && stream->bufbase_) {
		jas_free(stream->bufbase_);
		stream->bufbase_ = 0;
	}
	jas_free(stream);
}

jas_stream_t *iJp2WriteStream(ILcontext* context)
{
	jas_stream_t *stream;

	if (!(stream = jas_stream_create())) {
		return 0;
	}

	/* A stream associated with a memory buffer is always opened
	for both reading and writing in binary mode. */
	stream->openmode_ = JAS_STREAM_WRITE | JAS_STREAM_BINARY;

	/* We use buffering whether it is from memory or a file. */
	jas_stream_initbuf(stream, JAS_STREAM_FULLBUF);

	/* Select the operations for a memory stream. */
	stream->ops_ = &jas_stream_devilops;
	stream->obj_ = (void *) context;
	
	return stream;
}

//! Writes a Jp2 file
ILboolean Jp2Handler::save(const ILstring FileName)
{
	ILHANDLE	Jp2File;
	ILuint		Jp2Size;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	Jp2File = context->impl->iopenw(FileName);
	if (Jp2File == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	Jp2Size = saveF(Jp2File);
	context->impl->iclosew(Jp2File);

	if (Jp2Size == 0)
		return IL_FALSE;
	return IL_TRUE;
}

//! Writes a Jp2 to an already-opened file
ILuint Jp2Handler::saveF(ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

//! Writes a Jp2 to a memory "lump"
ILuint Jp2Handler::saveL(void *Lump, ILuint Size)
{
	ILuint Pos;
	iSetOutputLump(context, Lump, Size);
	Pos = context->impl->itellw(context);
	if (saveInternal() == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}

// Function from OpenSceneGraph (originally called getdata in their sources):
//  http://openscenegraph.sourcearchive.com/documentation/2.2.0/ReaderWriterJP2_8cpp-source.html
ILint Jp2ConvertData(jas_stream_t *in, jas_image_t *image)
{
	int ret;
	int numcmpts;
	int cmptno;
	jas_matrix_t *data[4];
	int x;
	int y;
	int width, height;

	width = jas_image_cmptwidth(image, 0);
	height = jas_image_cmptheight(image, 0);
	numcmpts = jas_image_numcmpts(image);

	ret = -1;

	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
		if (!(data[cmptno] = jas_matrix_create(1, width))) {
			goto done;
		}
	}
 
	for (y = height - 1; y >= 0; --y)
	//        for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			for (cmptno = 0; cmptno < numcmpts; ++cmptno)
			{
				// The sample data is unsigned.
				int c;
				if ((c = jas_stream_getc(in)) == EOF) {
					return -1;
				}
				jas_matrix_set(data[cmptno], 0, x, c);
			}
		}
		for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
			if (jas_image_writecmpt(image, cmptno, 0, y, width, 1,
				data[cmptno])) {
				goto done;
			}
		}
	}

	jas_stream_flush(in);
	ret = 0;

done:
	for (cmptno = 0; cmptno < numcmpts; ++cmptno) {
		if (data[cmptno]) {
			jas_matrix_destroy(data[cmptno]);
		}
	}

	return ret;
}

// Internal function used to save the Jp2.
// Since the JasPer documentation is extremely incomplete, I had to look at how OpenSceneGraph uses it:
//  http://openscenegraph.sourcearchive.com/documentation/2.2.0/ReaderWriterJP2_8cpp-source.html

//@TODO: Do we need to worry about images with depths > 1?
ILboolean Jp2Handler::saveInternal()
{
	jas_image_t *Jp2Image;
	jas_image_cmptparm_t cmptparm[4];
	jas_stream_t *Mem, *Stream;
	ILuint	NumChans, i;
	ILenum	NewFormat, NewType = IL_UNSIGNED_BYTE;
	ILimage	*TempImage = context->impl->iCurImage;

	if (!JasperInit) {
		if (jas_init()) {
			ilSetError(context, IL_LIB_JP2_ERROR);
			return IL_FALSE;
		}
		JasperInit = IL_TRUE;
	}

	if (context->impl->iCurImage->Type != IL_UNSIGNED_BYTE) {  //@TODO: Support greater than 1 bpc.
		NewType = IL_UNSIGNED_BYTE;
	}
	//@TODO: Do luminance/luminance-alpha/alpha separately.
	switch (context->impl->iCurImage->Format)
	{
		case IL_LUMINANCE:
			NewFormat = IL_LUMINANCE;
			NumChans = 1;
			break;
		case IL_ALPHA:
			NewFormat = IL_ALPHA;
			NumChans = 1;
			break;
		case IL_LUMINANCE_ALPHA:
			NewFormat = IL_LUMINANCE_ALPHA;
			NumChans = 2;
			break;
		case IL_COLOUR_INDEX:  // Assuming the color palette does not have an alpha value.
								//@TODO: Check for this in the future.
		case IL_RGB:
		case IL_BGR:
			NewFormat = IL_RGB;
			NumChans = 3;
			break;
		case IL_RGBA:
		case IL_BGRA:
			NewFormat = IL_RGBA;
			NumChans = 4;
			break;
	}

	if (NewType != context->impl->iCurImage->Type || NewFormat != context->impl->iCurImage->Format) {
		TempImage = iConvertImage(context, context->impl->iCurImage, NewFormat, NewType);
		if (TempImage == NULL)
			return IL_FALSE;
	}

	// The origin is always in the lower left corner.  Flip the buffer if it is not.
	if (TempImage->Origin == IL_ORIGIN_UPPER_LEFT)
		iFlipBuffer(context, TempImage->Data, TempImage->Depth, TempImage->Bps, TempImage->Height);

	// Have to tell JasPer about each channel.  In our case, they all have the same information.
	for (i = 0; i < NumChans; i++) {
		cmptparm[i].width = context->impl->iCurImage->Width;
		cmptparm[i].height = context->impl->iCurImage->Height;
		cmptparm[i].hstep = 1;
		cmptparm[i].vstep = 1;
		cmptparm[i].tlx = 0;
		cmptparm[i].tly = 0;
		cmptparm[i].prec = 8;
		cmptparm[i].sgnd = 0;  // Unsigned data
	}

	// Using the unknown color space, since we have not determined the space yet.
	//  This is done in the following switch statement.
	Jp2Image = jas_image_create(NumChans, cmptparm, JAS_CLRSPC_UNKNOWN);
	if (Jp2Image == NULL) {
		ilSetError(context, IL_LIB_JP2_ERROR);
		return IL_FALSE;
	}

	switch (NumChans)
	{
		case 1:
			jas_image_setclrspc(Jp2Image, JAS_CLRSPC_SGRAY);
			if (NewFormat == IL_LUMINANCE)
				jas_image_setcmpttype(Jp2Image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
			else // IL_ALPHA
				jas_image_setcmpttype(Jp2Image, 0, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_OPACITY));
			break;
		case 2:
			jas_image_setclrspc(Jp2Image, JAS_CLRSPC_SGRAY);
			jas_image_setcmpttype(Jp2Image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y));
			jas_image_setcmpttype(Jp2Image, 1, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_OPACITY));
			break;
		case 3:
			jas_image_setclrspc(Jp2Image, JAS_CLRSPC_SRGB);
			jas_image_setcmpttype(Jp2Image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
			jas_image_setcmpttype(Jp2Image, 1, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
			jas_image_setcmpttype(Jp2Image, 2, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
			break;
		case 4:
			jas_image_setclrspc(Jp2Image, JAS_CLRSPC_SRGB);
			jas_image_setcmpttype(Jp2Image, 0, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R));
			jas_image_setcmpttype(Jp2Image, 1, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G));
			jas_image_setcmpttype(Jp2Image, 2, JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B));
			jas_image_setcmpttype(Jp2Image, 3, JAS_IMAGE_CT_COLOR(JAS_IMAGE_CT_OPACITY));
			break;
	}

	Mem = jas_stream_memopen((char*)TempImage->Data, TempImage->SizeOfData);
	if (Mem == NULL) {
		jas_image_destroy(Jp2Image);
		ilSetError(context, IL_LIB_JP2_ERROR);
		return IL_FALSE;
	}
	Stream = iJp2WriteStream(context);
	if (Stream == NULL) {
		jas_stream_close(Mem);
		jas_image_destroy(Jp2Image);
		ilSetError(context, IL_LIB_JP2_ERROR);
		return IL_FALSE;
	}

	// Puts data in the format that JasPer wants it in.
	Jp2ConvertData(Mem, Jp2Image);

	// Does all of the encoding.
	if (jas_image_encode(Jp2Image, Stream, jas_image_strtofmt((char*)"jp2"), NULL)) {  //@TODO: Do we want to use any options?
		jas_stream_close(Mem);
		jas_stream_close(Stream);
		jas_image_destroy(Jp2Image);
		ilSetError(context, IL_LIB_JP2_ERROR);
		return IL_FALSE;
	}
	jas_stream_flush(Stream);  // Do any final writing.

	// Close the memory and output streams.
	jas_stream_close(Mem);
	jas_stream_close(Stream);
	// Destroy the JasPer image.
	jas_image_destroy(Jp2Image);

	// Destroy our temporary image if we used one.
	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	return IL_TRUE;
}

#endif//IL_NO_JP2