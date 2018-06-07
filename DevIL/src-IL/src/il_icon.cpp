//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_icon.cpp
//
// Description: Reads from a Windows icon (.ico) file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_ICO

#include "il_icon.h"

#ifndef IL_NO_PNG
	#include <png.h>
#endif

#ifdef _WIN32
	#pragma pack(push, ico_struct, 1)
#endif

typedef struct ICODIR
{
	ILshort		Reserved;	// Reserved (must be 0)
	ILshort		Type;		// Type (1 for icons, 2 for cursors)
	ILshort		Count;		// How many different images?
} IL_PACKSTRUCT ICODIR;

typedef struct ICODIRENTRY
{
	ILubyte		Width;			// Width, in pixels
	ILubyte		Height;			// Height, in pixels
	ILubyte		NumColours;		// Number of colors in image (0 if >=8bpp)
	ILubyte		Reserved;		// Reserved (must be 0)
	ILshort		Planes;			// Colour planes
	ILshort		Bpp;			// Bits per pixel
	ILuint		SizeOfData;		// How many bytes in this resource?
	ILuint		Offset;			// Offset from beginning of the file
} IL_PACKSTRUCT ICODIRENTRY;

typedef struct INFOHEAD
{
	ILint		Size;
	ILint		Width;
	ILint		Height;
	ILshort		Planes;
	ILshort		BitCount;
	ILint		Compression;
	ILint		SizeImage;
	ILint		XPixPerMeter;
	ILint		YPixPerMeter;
	ILint		ColourUsed;
	ILint		ColourImportant;
} IL_PACKSTRUCT INFOHEAD;

typedef struct ICOIMAGE
{
	INFOHEAD	Head;
	ILubyte		*Pal;	// Palette
	ILubyte		*Data;	// XOR mask
	ILubyte		*AND;	// AND mask
} ICOIMAGE;

#ifdef _WIN32
	#pragma pack(pop, ico_struct)
#endif

ILboolean iLoadIconPNG(ILcontext* context, ICOIMAGE *Icon);

IconHandler::IconHandler(ILcontext* context) :
	context(context)
{

}

//! Reads an icon file.
ILboolean IconHandler::load(ILconst_string FileName)
{
	ILHANDLE	IconFile;
	ILboolean	bIcon = IL_FALSE;

	IconFile = context->impl->iopenr(FileName);
	if (IconFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bIcon;
	}

	bIcon = loadF(IconFile);
	context->impl->icloser(IconFile);

	return bIcon;
}

//! Reads an already-opened icon file.
ILboolean IconHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains an icon.
ILboolean IconHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

// Internal function used to load the icon.
ILboolean IconHandler::loadInternal()
{
	ICODIR		IconDir;
	ICODIRENTRY	*DirEntries = NULL;
	ICOIMAGE	*IconImages = NULL;
	ILimage		*Image = NULL;
	ILint		i;
	ILuint		Size, PadSize, ANDPadSize, j, k, l, m, x, w, CurAndByte, AndBytes;
	ILboolean	BaseCreated = IL_FALSE;
	ILubyte		PNGTest[3];

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	IconDir.Reserved = GetLittleShort(context);
	IconDir.Type = GetLittleShort(context);
	IconDir.Count = GetLittleShort(context);

	if (context->impl->ieof(context))
		goto file_read_error;

	DirEntries = (ICODIRENTRY*)ialloc(context, sizeof(ICODIRENTRY) * IconDir.Count);
	IconImages = (ICOIMAGE*)ialloc(context, sizeof(ICOIMAGE) * IconDir.Count);
	if (DirEntries == NULL || IconImages == NULL) {
		ifree(DirEntries);
		ifree(IconImages);
		return IL_FALSE;
	}

	// Make it easier for file_read_error.
	for (i = 0; i < IconDir.Count; i++)
		imemclear(&IconImages[i], sizeof(ICOIMAGE));

	for (i = 0; i < IconDir.Count; ++i) {
		DirEntries[i].Width = (ILubyte)context->impl->igetc(context);
		DirEntries[i].Height = (ILubyte)context->impl->igetc(context);
		DirEntries[i].NumColours = (ILubyte)context->impl->igetc(context);
		DirEntries[i].Reserved = (ILubyte)context->impl->igetc(context);
		DirEntries[i].Planes = GetLittleShort(context);
		DirEntries[i].Bpp = GetLittleShort(context);
		DirEntries[i].SizeOfData = GetLittleUInt(context);
		DirEntries[i].Offset = GetLittleUInt(context);

		if (context->impl->ieof(context))
			goto file_read_error;
	}

	for (i = 0; i < IconDir.Count; i++) {
		context->impl->iseek(context, DirEntries[i].Offset, IL_SEEK_SET);

		// 08-22-2008: Adding test for compressed PNG data
		context->impl->igetc(context); // Skip the first character...seems to vary.
		context->impl->iread(context, PNGTest, 3, 1);
		if (!strnicmp((char*)PNGTest, "PNG", 3))  // Characters 'P', 'N', 'G' for PNG header
		{
#ifdef IL_NO_PNG
			ilSetError(context, IL_FORMAT_NOT_SUPPORTED);  // Cannot handle these without libpng.
			goto file_read_error;
#else
			context->impl->iseek(context, DirEntries[i].Offset, IL_SEEK_SET);
			if (!iLoadIconPNG(context, &IconImages[i]))
				goto file_read_error;
#endif
		}
		else
		{
			// Need to go back the 4 bytes that were just read.
			context->impl->iseek(context, DirEntries[i].Offset, IL_SEEK_SET);

			IconImages[i].Head.Size = GetLittleInt(context);
			IconImages[i].Head.Width = GetLittleInt(context);
			IconImages[i].Head.Height = GetLittleInt(context);
			IconImages[i].Head.Planes = GetLittleShort(context);
			IconImages[i].Head.BitCount = GetLittleShort(context);
			IconImages[i].Head.Compression = GetLittleInt(context);
			IconImages[i].Head.SizeImage = GetLittleInt(context);
			IconImages[i].Head.XPixPerMeter = GetLittleInt(context);
			IconImages[i].Head.YPixPerMeter = GetLittleInt(context);
			IconImages[i].Head.ColourUsed = GetLittleInt(context);
			IconImages[i].Head.ColourImportant = GetLittleInt(context);

			if (context->impl->ieof(context))
				goto file_read_error;

			if (IconImages[i].Head.BitCount < 8) {
				if (IconImages[i].Head.ColourUsed == 0) {
					switch (IconImages[i].Head.BitCount)
					{
						case 1:
							IconImages[i].Head.ColourUsed = 2;
							break;
						case 4:
							IconImages[i].Head.ColourUsed = 16;
							break;
					}
				}
				IconImages[i].Pal = (ILubyte*)ialloc(context, IconImages[i].Head.ColourUsed * 4);
				if (IconImages[i].Pal == NULL)
					goto file_read_error;  // @TODO: Rename the label.
				if (context->impl->iread(context, IconImages[i].Pal, IconImages[i].Head.ColourUsed * 4, 1) != 1)
					goto file_read_error;
			}
			else if (IconImages[i].Head.BitCount == 8) {
				IconImages[i].Pal = (ILubyte*)ialloc(context, 256 * 4);
				if (IconImages[i].Pal == NULL)
					goto file_read_error;
				if (context->impl->iread(context, IconImages[i].Pal, 1, 256 * 4) != 256*4)
					goto file_read_error;
			}
			else {
				IconImages[i].Pal = NULL;
			}

			PadSize = (4 - ((IconImages[i].Head.Width*IconImages[i].Head.BitCount + 7) / 8) % 4) % 4;  // Has to be DWORD-aligned.
			ANDPadSize = (4 - ((IconImages[i].Head.Width + 7) / 8) % 4) % 4;  // AND is 1 bit/pixel
			Size = ((IconImages[i].Head.Width*IconImages[i].Head.BitCount + 7) / 8 + PadSize)
								* (IconImages[i].Head.Height / 2);


			IconImages[i].Data = (ILubyte*)ialloc(context, Size);
			if (IconImages[i].Data == NULL)
				goto file_read_error;
			if (context->impl->iread(context, IconImages[i].Data, 1, Size) != Size)
				goto file_read_error;

			Size = (((IconImages[i].Head.Width + 7) /8) + ANDPadSize) * (IconImages[i].Head.Height / 2);
			IconImages[i].AND = (ILubyte*)ialloc(context, Size);
			if (IconImages[i].AND == NULL)
				goto file_read_error;
			if (context->impl->iread(context, IconImages[i].AND, 1, Size) != Size)
				goto file_read_error;
		}
	}


	for (i = 0; i < IconDir.Count; i++) {
		if (IconImages[i].Head.BitCount != 1 && IconImages[i].Head.BitCount != 4 &&
			IconImages[i].Head.BitCount != 8 && IconImages[i].Head.BitCount != 24 &&

			IconImages[i].Head.BitCount != 32)
			continue;

		if (!BaseCreated) {
			if (IconImages[i].Head.Size == 0)  // PNG compressed icon
				ilTexImage(context, IconImages[i].Head.Width, IconImages[i].Head.Height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
			else
				ilTexImage(context, IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
			context->impl->iCurImage->Origin = IL_ORIGIN_LOWER_LEFT;
			Image = context->impl->iCurImage;
			BaseCreated = IL_TRUE;
		}
		else {
			if (IconImages[i].Head.Size == 0)  // PNG compressed icon
				Image->Next = ilNewImage(context, IconImages[i].Head.Width, IconImages[i].Head.Height, 1, 4, 1);
			else
				Image->Next = ilNewImage(context, IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, 1);
			Image = Image->Next;
			Image->Format = IL_BGRA;
		}
		Image->Type = IL_UNSIGNED_BYTE;

		j = 0;  k = 0;  l = 128;  CurAndByte = 0; x = 0;

		w = IconImages[i].Head.Width;
		PadSize = (4 - ((w*IconImages[i].Head.BitCount + 7) / 8) % 4) % 4;  // Has to be DWORD-aligned.

		ANDPadSize = (4 - ((w + 7) / 8) % 4) % 4;  // AND is 1 bit/pixel
		AndBytes = (w + 7) / 8;

		if (IconImages[i].Head.BitCount == 1) {
			for (; j < Image->SizeOfData; k++) {
				for (m = 128; m && x < w; m >>= 1) {
					Image->Data[j] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4];
					Image->Data[j+1] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 1];
					Image->Data[j+2] = IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 2];
					Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
					j += 4;
					l >>= 1;

					++x;
				}
				if (l == 0 || x == w) {
					l = 128;
					CurAndByte++;
					if (x == w) {
						CurAndByte += ANDPadSize;
						k += PadSize;
						x = 0;
					}

				}
			}
		}
		else if (IconImages[i].Head.BitCount == 4) {
			for (; j < Image->SizeOfData; j += 8, k++) {
				Image->Data[j] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4];
				Image->Data[j+1] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 1];
				Image->Data[j+2] = IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 2];
				Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;

				++x;

				if(x < w) {
					Image->Data[j+4] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4];
					Image->Data[j+5] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 1];
					Image->Data[j+6] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 2];
					Image->Data[j+7] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
					l >>= 1;

					++x;

				}

				else

					j -= 4;


				if (l == 0 || x == w) {
					l = 128;
					CurAndByte++;
					if (x == w) {
						CurAndByte += ANDPadSize;

						k += PadSize;
						x = 0;
					}
				}
			}
		}
		else if (IconImages[i].Head.BitCount == 8) {
			for (; j < Image->SizeOfData; j += 4, k++) {
				Image->Data[j] = IconImages[i].Pal[IconImages[i].Data[k] * 4];
				Image->Data[j+1] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 1];
				Image->Data[j+2] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 2];
				if (IconImages[i].AND == NULL)  // PNG Palette
				{
					Image->Data[j+3] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 3];
				}
				else
				{
					Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				}
				l >>= 1;

				++x;
				if (l == 0 || x == w) {
					l = 128;
					CurAndByte++;
					if (x == w) {
						CurAndByte += ANDPadSize;

						k += PadSize;
						x = 0;
					}
				}
			}
		}
		else if (IconImages[i].Head.BitCount == 24) {
			for (; j < Image->SizeOfData; j += 4, k += 3) {
				Image->Data[j] = IconImages[i].Data[k];
				Image->Data[j+1] = IconImages[i].Data[k+1];
				Image->Data[j+2] = IconImages[i].Data[k+2];
				Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
				l >>= 1;

				++x;
				if (l == 0 || x == w) {
					l = 128;
					CurAndByte++;
					if (x == w) {
						CurAndByte += ANDPadSize;

						k += PadSize;
						x = 0;
					}
				}
			}
		}

		else if (IconImages[i].Head.BitCount == 32) {
			for (; j < Image->SizeOfData; j += 4, k += 4) {
				Image->Data[j] = IconImages[i].Data[k];
				Image->Data[j+1] = IconImages[i].Data[k+1];
				Image->Data[j+2] = IconImages[i].Data[k+2];

				//If the icon has 4 channels, use 4th channel for alpha...
				//(for Windows XP style icons with true alpha channel
				Image->Data[j+3] = IconImages[i].Data[k+3];
			}
		}
	}


	for (i = 0; i < IconDir.Count; i++) {
		ifree(IconImages[i].Pal);
		ifree(IconImages[i].Data);
		ifree(IconImages[i].AND);
	}
	ifree(IconImages);
	ifree(DirEntries);

	return ilFixImage(context);

file_read_error:
	if (IconImages) {
		for (i = 0; i < IconDir.Count; i++) {
			if (IconImages[i].Pal)
				ifree(IconImages[i].Pal);
			if (IconImages[i].Data)
				ifree(IconImages[i].Data);
			if (IconImages[i].AND)
				ifree(IconImages[i].AND);
		}
		ifree(IconImages);
	}
	if (DirEntries)
		ifree(DirEntries);
	return IL_FALSE;
}

#ifndef IL_NO_PNG
// 08-22-2008: Copying a lot of this over from il_png.c for the moment.
// @TODO: Make .ico and .png use the same functions.
png_structp ico_png_ptr = NULL;
png_infop   ico_info_ptr = NULL;
ILint		ico_color_type;

#define GAMMA_CORRECTION 1.0  // Doesn't seem to be doing anything...

ILint ico_readpng_init(ILcontext* context);
ILboolean ico_readpng_get_image(ILcontext* context, ICOIMAGE *Icon, ILdouble display_exponent);
void ico_readpng_cleanup();
#endif

ILboolean iLoadIconPNG(ILcontext* context, ICOIMAGE *Icon)
{
#ifndef IL_NO_PNG
	ILint init;
	init = ico_readpng_init(context);
	if (init)
		return IL_FALSE;
	if (!ico_readpng_get_image(context, Icon, GAMMA_CORRECTION))
		return IL_FALSE;

	ico_readpng_cleanup();
	Icon->Head.Size = 0;	// Easiest way to tell that this is a PNG.
							// In the normal routine, it will not be 0.

	return IL_TRUE;
#else
	Icon;
	return IL_FALSE;
#endif
}

#ifndef IL_NO_PNG
static void ico_png_read(png_structp png_ptr, png_bytep data, png_size_t length)
{
	ILcontext* context = (ILcontext*)png_get_io_ptr(png_ptr);
	context->impl->iread(context, data, 1, (ILuint)length);
	return;
}

static void ico_png_error_func(png_structp png_ptr, png_const_charp message)
{
	ILcontext* context = (ILcontext*)png_get_io_ptr(png_ptr);

	message;
	ilSetError(context, IL_LIB_PNG_ERROR);

	/*
	  changed 20040224
	  From the libpng docs:
	  "Errors handled through png_error() are fatal, meaning that png_error()
	   should never return to its caller. Currently, this is handled via
	   setjmp() and longjmp()"
	*/
	//return;
	longjmp(png_jmpbuf(png_ptr), 1);
}

static void ico_png_warn_func(png_structp png_ptr, png_const_charp message)
{
	png_ptr; message;
	return;
}

ILint ico_readpng_init(ILcontext* context)
{
	ico_png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, ico_png_error_func, ico_png_warn_func);
	if (!ico_png_ptr)
		return 4;	/* out of memory */

	ico_info_ptr = png_create_info_struct(ico_png_ptr);
	if (!ico_info_ptr) {
		png_destroy_read_struct(&ico_png_ptr, NULL, NULL);
		return 4;	/* out of memory */
	}


	/* we could create a second info struct here (end_info), but it's only
	 * useful if we want to keep pre- and post-IDAT chunk info separated
	 * (mainly for PNG-aware image editors and converters) */


	/* setjmp() must be called in every function that calls a PNG-reading
	 * libpng function */

	if (setjmp(png_jmpbuf(ico_png_ptr))) {
		png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
		return 2;
	}


	png_set_read_fn(ico_png_ptr, context, ico_png_read);
	png_set_error_fn(ico_png_ptr, context, ico_png_error_func, ico_png_warn_func);

//	png_set_sig_bytes(ico_png_ptr, 8);	/* we already read the 8 signature bytes */

	png_read_info(ico_png_ptr, ico_info_ptr);  /* read all PNG info up to image data */

	/* alternatively, could make separate calls to png_get_image_width(),
	 * etc., but want bit_depth and ico_color_type for later [don't care about
	 * compression_type and filter_type => NULLs] */

	/* OK, that's all we need for now; return happy */

	return 0;
}

ILboolean ico_readpng_get_image(ILcontext* context, ICOIMAGE *Icon, ILdouble display_exponent)
{
	png_bytepp	row_pointers = NULL;
	png_uint_32 width, height; // Changed the type to fix AMD64 bit problems, thanks to Eric Werness
	ILdouble	screen_gamma = 1.0;
	ILuint		i, channels;
	ILenum		format;
	png_colorp	palette;
	ILint		num_palette, j, bit_depth;
#if _WIN32 || DJGPP
	ILdouble image_gamma;
#endif

	display_exponent;

	/* setjmp() must be called in every function that calls a PNG-reading
	 * libpng function */

	if (setjmp(png_jmpbuf(ico_png_ptr))) {
		png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
		return IL_FALSE;
	}

	png_get_IHDR(ico_png_ptr, ico_info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
	             &bit_depth, &ico_color_type, NULL, NULL, NULL);

	// Expand low-bit-depth grayscale images to 8 bits
	if (ico_color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(ico_png_ptr);
	}

	// Expand RGB images with transparency to full alpha channels
	//	so the data will be available as RGBA quartets.
 	// But don't expand paletted images, since we want alpha palettes!
	if (png_get_valid(ico_png_ptr, ico_info_ptr, PNG_INFO_tRNS) && !(png_get_valid(ico_png_ptr, ico_info_ptr, PNG_INFO_PLTE)))
		png_set_tRNS_to_alpha(ico_png_ptr);

	//refresh information (added 20040224)
	png_get_IHDR(ico_png_ptr, ico_info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
	             &bit_depth, &ico_color_type, NULL, NULL, NULL);

	if (bit_depth < 8) {	// Expanded earlier for grayscale, now take care of palette and rgb
		bit_depth = 8;
		png_set_packing(ico_png_ptr);
	}

	// Perform gamma correction.
	// @TODO:  Determine if we should call png_set_gamma if image_gamma is 1.0.
#if _WIN32 || DJGPP
	screen_gamma = 2.2;
	if (png_get_gAMA(ico_png_ptr, ico_info_ptr, &image_gamma))
		png_set_gamma(ico_png_ptr, screen_gamma, image_gamma);
#else
	screen_gamma = screen_gamma;
#endif

	//fix endianess
#ifdef __LITTLE_ENDIAN__
	if (bit_depth == 16)
		png_set_swap(ico_png_ptr);
#endif


	png_read_update_info(ico_png_ptr, ico_info_ptr);
	channels = (ILint)png_get_channels(ico_png_ptr, ico_info_ptr);
	// added 20040224: update ico_color_type so that it has the correct value
	// in iLoadPngInternal (globals rule...)
	ico_color_type = png_get_color_type(ico_png_ptr, ico_info_ptr);

	// Determine internal format
	switch (ico_color_type)
	{
		case PNG_COLOR_TYPE_PALETTE:
			Icon->Head.BitCount = 8;
			format = IL_COLOUR_INDEX;
			break;
		case PNG_COLOR_TYPE_RGB:
			Icon->Head.BitCount = 24;
			format = IL_RGB;
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			Icon->Head.BitCount = 32;
			format = IL_RGBA;
			break;
		default:
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
			return IL_FALSE;
	}

	if (ico_color_type & PNG_COLOR_MASK_COLOR)
		png_set_bgr(ico_png_ptr);

	Icon->Head.Width = width;
	Icon->Head.Height = height;
	Icon->Data = (ILubyte*)ialloc(context, width * height * Icon->Head.BitCount / 8);
	if (Icon->Data == NULL)
	{
		png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
		return IL_FALSE;
	}

	// Copy Palette
	if (format == IL_COLOUR_INDEX)
	{
		int chans;
		png_bytep trans = NULL;
		int num_trans = -1;
		if (!png_get_PLTE(ico_png_ptr, ico_info_ptr, &palette, &num_palette)) {
			ilSetError(context, IL_ILLEGAL_FILE_VALUE);
			png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
			return IL_FALSE;
		}

		chans = 4;

		if (png_get_valid(ico_png_ptr, ico_info_ptr, PNG_INFO_tRNS)) {
			png_get_tRNS(ico_png_ptr, ico_info_ptr, &trans, &num_trans, NULL);
		}

// @TODO: We may need to keep track of the size of the palette.
		Icon->Pal = (ILubyte*)ialloc(context, num_palette * chans);

		for (j = 0; j < num_palette; ++j) {
			Icon->Pal[chans*j + 0] = palette[j].blue;
			Icon->Pal[chans*j + 1] = palette[j].green;
			Icon->Pal[chans*j + 2] = palette[j].red;
			if (trans != NULL) {
				if (j < num_trans)
					Icon->Pal[chans*j + 3] = trans[j];
				else
					Icon->Pal[chans*j + 3] = 255;
			}
		}

		Icon->AND = NULL;  // Transparency information is obtained from libpng.
	}

	//allocate row pointers
	if ((row_pointers = (png_bytepp)ialloc(context, height * sizeof(png_bytep))) == NULL) {
		png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
		return IL_FALSE;
	}


	// Set the individual row_pointers to point at the correct offsets
	// Needs to be flipped
	for (i = 0; i < height; i++)
		row_pointers[height - i - 1] = Icon->Data + i * width * Icon->Head.BitCount / 8;
		//row_pointers[i] = Icon->Data + i * width * Icon->Head.BitCount / 8;


	// Now we can go ahead and just read the whole image
	png_read_image(ico_png_ptr, row_pointers);

	/* and we're done!	(png_read_end() can be omitted if no processing of
	 * post-IDAT text/time/etc. is desired) */
	//png_read_end(ico_png_ptr, NULL);
	ifree(row_pointers);

	return IL_TRUE;
}

void ico_readpng_cleanup()
{
	if (ico_png_ptr && ico_info_ptr) {
		png_destroy_read_struct(&ico_png_ptr, &ico_info_ptr, NULL);
		ico_png_ptr = NULL;
		ico_info_ptr = NULL;
	}
}
#endif//IL_NO_PNG

#endif//IL_NO_ICO