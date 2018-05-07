//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_tiff.cpp
//
// Description: Tiff (.tif) functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_TIF

#include "tiffio.h"

#include <time.h>

#define MAGIC_HEADER1	0x4949
#define MAGIC_HEADER2	0x4D4D


#if (defined(_WIN32) || defined(_WIN64)) && defined(IL_USE_PRAGMA_LIBS)
	#if defined(_MSC_VER) || defined(__BORLANDC__)
		#ifndef _DEBUG
			#pragma comment(lib, "tiff.lib")
		#else
			//#pragma comment(lib, "tiff-d.lib")
			#pragma comment(lib, "tiff.lib")
		#endif
	#endif
#endif


/*----------------------------------------------------------------------------*/

// No need for a separate header
static ILboolean iLoadTiffInternal(ILcontext* context);
static char*     iMakeString(void);
static TIFF*     iTIFFOpen(ILcontext* context, char *Mode);
static ILboolean iSaveTiffInternal(ILcontext* context/*, ILconst_string Filename*/);

/*----------------------------------------------------------------------------*/

ILboolean ilisValidTiffExtension(ILconst_string FileName)
{
	if (!iCheckExtension((ILstring)FileName, IL_TEXT("tif")) &&
		!iCheckExtension((ILstring)FileName, IL_TEXT("tiff")))
		return IL_FALSE;
	else
		return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

//! Checks if the file specified in FileName is a valid tiff file.
ILboolean ilIsValidTiff(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	TiffFile;
	ILboolean	bTiff = IL_FALSE;

	if (!ilisValidTiffExtension((ILstring) FileName)) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bTiff;
	}

	TiffFile = context->impl->iopenr((ILstring)FileName);
	if (TiffFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bTiff;
	}

	bTiff = ilIsValidTiffF(context, TiffFile);
	context->impl->icloser(TiffFile);

	return bTiff;
}

/*----------------------------------------------------------------------------*/

ILboolean ilisValidTiffFunc(ILcontext* context)
{
	ILushort Header1, Header2;

	Header1 = GetLittleUShort(context);

	if (Header1 != MAGIC_HEADER1 && Header1 != MAGIC_HEADER2)
		return IL_FALSE;

	if (Header1 == MAGIC_HEADER1)
		Header2 = GetLittleUShort(context);
	else
		Header2 = GetBigUShort(context);

	if (Header2 != 42)
		return IL_FALSE;

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/

//! Checks if the ILHANDLE contains a valid tiff file at the current position.
ILboolean ilIsValidTiffF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = ilisValidTiffFunc(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Checks if Lump is a valid Tiff lump.
ILboolean ilIsValidTiffL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return ilisValidTiffFunc(context);
}

/*----------------------------------------------------------------------------*/

//! Reads a Tiff file
ILboolean ilLoadTiff(ILcontext* context, ILconst_string FileName)
{
	ILHANDLE	TiffFile;
	ILboolean	bTiff = IL_FALSE;

	TiffFile = context->impl->iopenr(FileName);
	if (TiffFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
	}
	else {
		bTiff = ilLoadTiffF(context, TiffFile);
		context->impl->icloser(TiffFile);
	}

	return bTiff;
}

/*----------------------------------------------------------------------------*/

//! Reads an already-opened Tiff file
ILboolean ilLoadTiffF(ILcontext* context, ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = iLoadTiffInternal(context);
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

/*----------------------------------------------------------------------------*/

//! Reads from a memory "lump" that contains a Tiff
ILboolean ilLoadTiffL(ILcontext* context, const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return iLoadTiffInternal(context);
}

/*----------------------------------------------------------------------------*/

void warningHandler(const char* mod, const char* fmt, va_list ap)
{
	mod; fmt; ap;
	//char buff[1024];
	//vsnprintf(buff, 1024, fmt, ap);
}

void errorHandler(const char* mod, const char* fmt, va_list ap)
{
	mod; fmt; ap;
	//char buff[1024];
	//vsnprintf(buff, 1024, fmt, ap);
}

////

/*
ILboolean iLoadTiffInternal (TIFF* tif, ILimage* Image)
{
	////

	uint16   photometric, planarconfig, orientation;
	uint16   samplesperpixel, bitspersample, *sampleinfo, extrasamples;
	uint32   w, h, d, linesize, tilewidth, tilelength;
	ILushort si;

	////

	TIFFSetDirectory(tif, directory);
	////

	// Process fields

	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

	TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDEPTH,      &d); //TODO: d is ignored...
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
	TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE,   &bitspersample);
	TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,    &extrasamples, &sampleinfo);
	TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION,     &orientation);

	linesize = TIFFScanlineSize(tif);

	TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC,  &photometric);
	TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);

	tilewidth = w; tilelength = h;
	TIFFGetFieldDefaulted(tif, TIFFTAG_TILEWIDTH,  &tilewidth);
	TIFFGetFieldDefaulted(tif, TIFFTAG_TILELENGTH, &tilelength);

	////

	if (extrasamples != 0) {
		return IL_FALSE;
	}

	////

	if (!Image) {
		int type = IL_UNSIGNED_BYTE;
		if (bitspersample == 16) type = IL_UNSIGNED_SHORT;
		if(!ilTexImage(context, w, h, 1, 1, IL_LUMINANCE, type, NULL)) {
			TIFFClose(tif);
			return IL_FALSE;
		}
		context->impl->iCurImage->NumNext = 0;
		Image = context->impl->iCurImage;
	}
	else {
		Image->Next = ilNewImage(w, h, 1, 1, 1);
		if(Image->Next == NULL) {
			TIFFClose(tif);
			return IL_FALSE;
		}
		Image = Image->Next;
		context->impl->iCurImage->NumNext++;
	}
}
*/

////


// Internal function used to load the Tiff.
ILboolean iLoadTiffInternal(ILcontext* context)
{
	TIFF	 *tif;
	uint16	 photometric, planarconfig, orientation;
	uint16	 samplesperpixel, bitspersample, *sampleinfo, extrasamples;
	uint32	 w, h, d, linesize, tilewidth, tilelength;
	ILubyte  *pImageData;
	ILuint	 i, ProfileLen, DirCount = 0;
	void	 *Buffer;
	ILimage  *Image, *TempImage;
	ILushort si;
        ILfloat  x_position, x_resolution, y_position, y_resolution;
	//TIFFRGBAImage img;
	//char emsg[1024];

	// to avoid high order bits garbage when used as shorts
	w = h = d = linesize = tilewidth = tilelength = 0;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	TIFFSetWarningHandler (NULL);
	TIFFSetErrorHandler   (NULL);

	//for debugging only
	//TIFFSetWarningHandler(warningHandler);
	//TIFFSetErrorHandler(errorHandler);

	tif = iTIFFOpen(context, (char*)"r");
	if (tif == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	do {
		DirCount++;
	} while (TIFFReadDirectory(tif));

	/*
	 if (!ilTexImage(1, 1, 1, 1, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
		 TIFFClose(tif);
		 return IL_FALSE;
	}
	Image = context->impl->iCurImage;
	for (i = 1; i < DirCount; i++) {
		 Image->Next = ilNewImage(context, 1, 1, 1, 1, 1);
		if (Image->Next == NULL) {
			TIFFClose(tif);
			return IL_FALSE;
		}
		Image = Image->Next;
	}
	context->impl->iCurImage->NumNext = DirCount - 1;
	*/
	Image = NULL;
	for (i = 0; i < DirCount; i++) {
		TIFFSetDirectory(tif, (tdir_t)i);
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH,  &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);

		TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDEPTH,		&d); //TODO: d is ignored...
		TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
		TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE,	&bitspersample);
		TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,	&extrasamples, &sampleinfo);
		TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, 	&orientation);

		linesize = (uint32)TIFFScanlineSize(tif);
		
		//added 2003-08-31
		//1 bpp tiffs are not neccessarily greyscale, they can
		//have a palette (photometric == 3)...get this information
		TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC,  &photometric);
		TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);

		//special-case code for frequent data cases that may be read more
		//efficiently than with the TIFFReadRGBAImage() interface.
		
		//added 2004-05-12
		//Get tile sizes and use TIFFReadRGBAImage() for tiled images for now
		tilewidth = w; tilelength = h;
		TIFFGetFieldDefaulted(tif, TIFFTAG_TILEWIDTH,  &tilewidth);
		TIFFGetFieldDefaulted(tif, TIFFTAG_TILELENGTH, &tilelength);


		if (extrasamples == 0
			&& samplesperpixel == 1  //luminance or palette
			&& (bitspersample == 8 || bitspersample == 1 || bitspersample == 16)
			&& (photometric == PHOTOMETRIC_MINISWHITE
				|| photometric == PHOTOMETRIC_MINISBLACK
				|| photometric == PHOTOMETRIC_PALETTE)
			&& (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
			&& tilewidth == w && tilelength == h
			) {
			ILubyte* strip;
			tsize_t stripsize;
			ILuint y;
			uint32 rowsperstrip, j, linesread;

			//TODO: 1 bit/pixel images should not be stored as 8 bits...
			//(-> add new format)
			if (!Image) {
				int type = IL_UNSIGNED_BYTE;
				if (bitspersample == 16) type = IL_UNSIGNED_SHORT;
				if (!ilTexImage(context, w, h, 1, 1, IL_LUMINANCE, type, NULL)) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = context->impl->iCurImage;
			}
			else {
				Image->Next = ilNewImage(context, w, h, 1, 1, 1);
				if (Image->Next == NULL) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = Image->Next;
			}

			if (photometric == PHOTOMETRIC_PALETTE) { //read palette
				uint16 *red, *green, *blue;
				//ILboolean is16bitpalette = IL_FALSE;
				ILubyte *entry;
				uint32 count = 1 << bitspersample, j;
		
				TIFFGetField(tif, TIFFTAG_COLORMAP, &red, &green, &blue);

				Image->Format = IL_COLOUR_INDEX;
				Image->Pal.PalSize = (count)*3;
				Image->Pal.PalType = IL_PAL_RGB24;
				Image->Pal.Palette = (ILubyte*)ialloc(context, Image->Pal.PalSize);
				entry = Image->Pal.Palette;
				for (j = 0; j < count; ++j) {
					entry[0] = (ILubyte)(red[j] >> 8);
					entry[1] = (ILubyte)(green[j] >> 8);
					entry[2] = (ILubyte)(blue[j] >> 8);

					entry += 3;
				}
			}

			TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
			stripsize = TIFFStripSize(tif);

			strip = (ILubyte*)ialloc(context, stripsize);

			if (bitspersample == 8 || bitspersample == 16) {
				ILubyte *dat = Image->Data;
				for (y = 0; y < h; y += rowsperstrip) {
					//the last strip may contain less data if the image
					//height is not evenly divisible by rowsperstrip
					if (y + rowsperstrip > h) {
						stripsize = linesize*(h - y);
						linesread = h - y;
					}
					else
						linesread = rowsperstrip;

					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
						ilSetError(context, IL_LIB_TIFF_ERROR);
						ifree(strip);
						TIFFClose(tif);
						return IL_FALSE;
					}

					if (photometric == PHOTOMETRIC_MINISWHITE) { //invert channel
						uint32 k, t2;
						for (j = 0; j < linesread; ++j) {
							t2 = j*linesize;
							//this works for 16bit images as well: the two bytes
							//making up a pixel can be inverted independently
							for (k = 0; k < Image->Bps; ++k)
								dat[k] = ~strip[t2 + k];
							dat += w;
						}
					}
					else
						for(j = 0; j < linesread; ++j)
							memcpy(&Image->Data[(y + j)*Image->Bps], &strip[j*linesize], Image->Bps);
				}
			}
			else if (bitspersample == 1) {
				//TODO: add a native format to devil, so we don't have to
				//unpack the values here
				ILubyte mask, curr, *dat = Image->Data;
				uint32 k, sx, t2;
				for (y = 0; y < h; y += rowsperstrip) {
					//the last strip may contain less data if the image
					//height is not evenly divisible by rowsperstrip
					if (y + rowsperstrip > h) {
						stripsize = linesize*(h - y);
						linesread = h - y;
					}
					else
						linesread = rowsperstrip;

					if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
						ilSetError(context, IL_LIB_TIFF_ERROR);
						ifree(strip);
						TIFFClose(tif);
						return IL_FALSE;
					}

					for (j = 0; j < linesread; ++j) {
						k = 0;
						sx = 0;
						t2 = j*linesize;
						while (k < w) {
							curr = strip[t2 + sx];
							if (photometric == PHOTOMETRIC_MINISWHITE)
								curr = ~curr;
							for (mask = 0x80; mask != 0 && k < w; mask >>= 1){
								if((curr & mask) != 0)
									dat[k] = 255;
								else
									dat[k] = 0;
								++k;
							}
							++sx;
						}
						dat += w;
					}
				}
			}

			ifree(strip);

			if(orientation == ORIENTATION_TOPLEFT)
				Image->Origin = IL_ORIGIN_UPPER_LEFT;
			else if(orientation == ORIENTATION_BOTLEFT)
				Image->Origin = IL_ORIGIN_LOWER_LEFT;
		}
		//for 16bit rgb images:
		else if (extrasamples == 0
			&& samplesperpixel == 3
			&& (bitspersample == 8 || bitspersample == 16)
			&& photometric == PHOTOMETRIC_RGB
			&& planarconfig == 1
			&& (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
			&& tilewidth == w && tilelength == h
			) {
			ILubyte *strip, *dat;
			tsize_t stripsize;
			ILuint y;
			uint32 rowsperstrip, j, linesread;

			if (!Image) {
				int type = IL_UNSIGNED_BYTE;
				if (bitspersample == 16) type = IL_UNSIGNED_SHORT;
				if(!ilTexImage(context, w, h, 1, 3, IL_RGB, type, NULL)) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = context->impl->iCurImage;
			}
			else {
				Image->Next = ilNewImage(context, w, h, 1, 1, 1);
				if(Image->Next == NULL) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = Image->Next;
			}

			TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
			stripsize = TIFFStripSize(tif);

			strip = (ILubyte*)ialloc(context, stripsize);

			dat = Image->Data;
			for (y = 0; y < h; y += rowsperstrip) {
				//the last strip may contain less data if the image
				//height is not evenly divisible by rowsperstrip
				if (y + rowsperstrip > h) {
					stripsize = linesize*(h - y);
					linesread = h - y;
				}
				else
					linesread = rowsperstrip;

				if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, y, 0), strip, stripsize) == -1) {
					ilSetError(context, IL_LIB_TIFF_ERROR);
					ifree(strip);
					TIFFClose(tif);
					return IL_FALSE;
				}

				for(j = 0; j < linesread; ++j)
						memcpy(&Image->Data[(y + j)*Image->Bps], &strip[j*linesize], Image->Bps);
			}

			ifree(strip);
			
			if(orientation == ORIENTATION_TOPLEFT)
				Image->Origin = IL_ORIGIN_UPPER_LEFT;
			else if(orientation == ORIENTATION_BOTLEFT)
				Image->Origin = IL_ORIGIN_LOWER_LEFT;
		}/*
		else if (extrasamples == 0 && samplesperpixel == 3  //rgb
					&& (bitspersample == 8 || bitspersample == 1 || bitspersample == 16)
					&& photometric == PHOTOMETRIC_RGB
					&& (planarconfig == PLANARCONFIG_CONTIG || planarcon
						&& (orientation == ORIENTATION_TOPLEFT || orientation == ORIENTATION_BOTLEFT)
					) {
		}
				*/
		else {
				//not direclty supported format
			if(!Image) {
				if(!ilTexImage(context, w, h, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = context->impl->iCurImage;
			}
			else {
				Image->Next = ilNewImage(context, w, h, 1, 4, 1);
				if(Image->Next == NULL) {
					TIFFClose(tif);
					return IL_FALSE;
				}
				Image = Image->Next;
			}

			if (samplesperpixel == 4) {
				TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples, &sampleinfo);
				if (!sampleinfo || sampleinfo[0] == EXTRASAMPLE_UNSPECIFIED) {
					si = EXTRASAMPLE_ASSOCALPHA;
					TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, &si);
				}
			}
			/*
			 if (!ilResizeImage(Image, Image->Width, Image->Height, 1, 4, 1)) {
				 TIFFClose(tif);
				 return IL_FALSE;
			 }
			 */
			Image->Format = IL_RGBA;
			Image->Type = IL_UNSIGNED_BYTE;
	
			// Siigron: added u_long cast to shut up compiler warning
			//2003-08-31: changed flag from 1 (exit on error) to 0 (keep decoding)
			//this lets me view text.tif, but can give crashes with unsupported
			//tiffs...
			//2003-09-04: keep flag 1 for official version for now
			if (!TIFFReadRGBAImage(tif, Image->Width, Image->Height, (uint32*)Image->Data, 0)) {
				TIFFClose(tif);
				ilSetError(context, IL_LIB_TIFF_ERROR);
				return IL_FALSE;
			}
			Image->Origin = IL_ORIGIN_LOWER_LEFT;  // eiu...dunno if this is right

#ifdef __BIG_ENDIAN__ //TIFFReadRGBAImage reads abgr on big endian, convert to rgba
			EndianSwapData(Image);
#endif

			/*
			 The following switch() should not be needed,
			 because we take care of the special cases that needed
			 these conversions. But since not all special cases
			 are handled right now, keep it :)
			 */
			//TODO: put switch into the loop??
			TempImage = context->impl->iCurImage;
			context->impl->iCurImage = Image; //ilConvertImage() converts context->impl->iCurImage
			switch (samplesperpixel)
			{
				case 1:
					//added 2003-08-31 to keep palettized tiffs colored
					if(photometric != 3)
						ilConvertImage(context, IL_LUMINANCE, IL_UNSIGNED_BYTE);
					else //strip alpha as tiff supports no alpha palettes
						ilConvertImage(context, IL_RGB, IL_UNSIGNED_BYTE);
					break;
					
				case 3:
					//TODO: why the ifdef??
#ifdef __LITTLE_ENDIAN__
					ilConvertImage(context, IL_RGB, IL_UNSIGNED_BYTE);
#endif			
					break; 
					
				case 4:
					pImageData = Image->Data;
					//removed on 2003-08-26...why was this here? libtiff should and does
					//take care of these things???
					/*			
					//invert alpha
#ifdef __LITTLE_ENDIAN__
					pImageData += 3;
#endif			
					for (i = Image->Width * Image->Height; i > 0; i--) {
						*pImageData ^= 255;
						pImageData += 4;
					}
					*/
					break;
			}
			context->impl->iCurImage = TempImage;
			
		} //else not directly supported format

		if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &ProfileLen, &Buffer)) {
			if (Image->Profile && Image->ProfileSize)
				ifree(Image->Profile);
			Image->Profile = (ILubyte*)ialloc(context, ProfileLen);
			if (Image->Profile == NULL) {
				TIFFClose(tif);
				return IL_FALSE;
			}

			memcpy(Image->Profile, Buffer, ProfileLen);
			Image->ProfileSize = ProfileLen;

			//removed on 2003-08-24 as explained in bug 579574 on sourceforge
			//_TIFFfree(Buffer);
		}

                // dangelo: read offset from tiff tags.
                //If nothing is stored in these tags, then this must be an "uncropped" TIFF 
                //file, in which case, the "full size" width/height is the same as the 
                //"cropped" width and height
                //
                // the full size is currently not supported by DevIL
                //if (TIFFGetField(tif, TIFFTAG_PIXAR_IMAGEFULLWIDTH, &(c->full_width)) ==
                //        0)
                //    (c->full_width = c->cropped_width);
                //if (TIFFGetField(tif, TIFFTAG_PIXAR_IMAGEFULLLENGTH, &(c->full_height)) ==
                //        0)
                //    (c->full_height = c->cropped_height);

                if (TIFFGetField(tif, TIFFTAG_XPOSITION, &x_position) == 0)
                    x_position = 0;
                if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &x_resolution) == 0)
                    x_resolution = 0;
                if (TIFFGetField(tif, TIFFTAG_YPOSITION, &y_position) == 0)
                    y_position = 0;
                if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &y_resolution) == 0)
                    y_resolution = 0;

                //offset in pixels of "cropped" image from top left corner of 
                //full image (rounded to nearest integer)
                Image->OffX = (uint32) ((x_position * x_resolution) + 0.49);
                Image->OffY = (uint32) ((y_position * y_resolution) + 0.49);
                

		/*
		 Image = Image->Next;
		 if (Image == NULL)  // Should never happen except when we reach the end, but check anyway.
		 break;
		 */ 
	} //for tiff directories

	TIFFClose(tif);

	return ilFixImage(context);
}

/*----------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////////////////
// Extension to load tiff files from memory
// Marco Fabbricatore (fabbrica@ai-lab.fh-furtwangen.de)
/////////////////////////////////////////////////////////////////////////////////////////

static tsize_t 
_tiffFileReadProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	ILcontext* context = (ILcontext*)fd;
	return context->impl->iread(context, pData, 1, (ILuint)tSize);
}

/*----------------------------------------------------------------------------*/

// We have trouble sometimes reading when writing a file.  Specifically, the only time
//  I have seen libtiff call this is at the beginning of writing a tiff, before
//  anything is ever even written!  Also, we have to return 0, no matter what tSize
//  is.  If we return tSize like would be expected, then TIFFClientOpen fails.
static tsize_t 
_tiffFileReadProcW(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	fd;
	return 0;
}

/*----------------------------------------------------------------------------*/

static tsize_t 
_tiffFileWriteProc(thandle_t fd, tdata_t pData, tsize_t tSize)
{
	ILcontext* context = (ILcontext*)fd;
	return context->impl->iwrite(context, pData, 1, (ILuint)tSize);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProc(thandle_t fd, toff_t tOff, int whence)
{
	ILcontext* context = (ILcontext*)fd;

	/* we use this as a special code, so avoid accepting it */
	if (tOff == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	context->impl->iseek(context, (ILint)tOff, whence);
	return context->impl->itell(context);
	//return tOff;
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSeekProcW(thandle_t fd, toff_t tOff, int whence)
{
	/* we use this as a special code, so avoid accepting it */
	if (tOff == 0xFFFFFFFF)
		return 0xFFFFFFFF;

	ILcontext* context = (ILcontext*)fd;
	context->impl->iseekw(context, (ILint)tOff, whence);
	return context->impl->itellw(context);
	//return tOff;
}

/*----------------------------------------------------------------------------*/

static int
_tiffFileCloseProc(thandle_t fd)
{
	fd;
	return (0);
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProc(thandle_t fd)
{
	ILcontext* context = (ILcontext*)fd;

	ILint Offset, Size;
	Offset = context->impl->itell(context);
	context->impl->iseek(context, 0, IL_SEEK_END);
	Size = context->impl->itell(context);
	context->impl->iseek(context, Offset, IL_SEEK_SET);

	fd;

	return Size;
}

/*----------------------------------------------------------------------------*/

static toff_t
_tiffFileSizeProcW(thandle_t fd)
{
	ILcontext* context = (ILcontext*)fd;
	ILint Offset, Size;
	Offset = context->impl->itellw(context);
	context->impl->iseekw(context, 0, IL_SEEK_END);
	Size = context->impl->itellw(context);
	context->impl->iseekw(context, Offset, IL_SEEK_SET);

	return Size;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static int
_tiffDummyMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	fd; pbase; psize;
	return 0;
}

/*----------------------------------------------------------------------------*/

#ifdef __BORLANDC__
#pragma argsused
#endif
static void
_tiffDummyUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	fd; base; size;
	return;
}

/*----------------------------------------------------------------------------*/

TIFF *iTIFFOpen(ILcontext* context, char *Mode)
{
	TIFF *tif;

	if (Mode[0] == 'w')
		tif = TIFFClientOpen("TIFFMemFile", Mode,
							(thandle_t)context,
							_tiffFileReadProcW, _tiffFileWriteProc,
							_tiffFileSeekProcW, _tiffFileCloseProc,
							_tiffFileSizeProcW, _tiffDummyMapProc,
							_tiffDummyUnmapProc);
	else
		tif = TIFFClientOpen("TIFFMemFile", Mode,
							(thandle_t)context,
							_tiffFileReadProc, _tiffFileWriteProc,
							_tiffFileSeekProc, _tiffFileCloseProc,
							_tiffFileSizeProc, _tiffDummyMapProc,
							_tiffDummyUnmapProc);
	
	return tif;
}

/*----------------------------------------------------------------------------*/


//! Writes a Tiff file
ILboolean ilSaveTiff(ILcontext* context, const ILstring FileName)
{
	ILHANDLE	TiffFile;
	ILuint		TiffSize;

	if (ilGetBoolean(context, IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(context, IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	TiffFile = context->impl->iopenw(FileName);
	if (TiffFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

	TiffSize = ilSaveTiffF(context, TiffFile);
	context->impl->iclosew(TiffFile);

	if (TiffSize == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Writes a Tiff to an already-opened file
ILuint ilSaveTiffF(ILcontext* context, ILHANDLE File)
{
	ILuint Pos;
	iSetOutputFile(context, File);
	Pos = context->impl->itellw(context);
	if (iSaveTiffInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


//! Writes a Tiff to a memory "lump"
ILuint ilSaveTiffL(ILcontext* context, void *Lump, ILuint Size)
{
	ILuint Pos = context->impl->itellw(context);
	iSetOutputLump(context, Lump, Size);
	if (iSaveTiffInternal(context) == IL_FALSE)
		return 0;  // Error occurred
	return context->impl->itellw(context) - Pos;  // Return the number of bytes written.
}


// @TODO:  Accept palettes!

// Internal function used to save the Tiff.
ILboolean iSaveTiffInternal(ILcontext* context/*, ILconst_string Filename*/)
{
	ILenum	Format;
	ILenum	Compression;
	ILuint	ixLine;
	TIFF	*File;
	char	Description[512];
	ILimage *TempImage;
	const char	*str;
	ILboolean SwapColors;
	ILubyte *OldData;

	if(context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

#if 1
	TIFFSetWarningHandler (NULL);
	TIFFSetErrorHandler   (NULL);
#else
	//for debugging only
	TIFFSetWarningHandler(warningHandler);
	TIFFSetErrorHandler(errorHandler);
#endif
	if (iGetHint(context, IL_COMPRESSION_HINT) == IL_USE_COMPRESSION)
		Compression = COMPRESSION_LZW;
	else
		Compression = COMPRESSION_NONE;

	if (context->impl->iCurImage->Format == IL_COLOUR_INDEX) {
		if (ilGetBppPal(context->impl->iCurImage->Pal.PalType) == 4)  // Preserve the alpha.
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGBA, IL_UNSIGNED_BYTE);
		else
			TempImage = iConvertImage(context, context->impl->iCurImage, IL_RGB, IL_UNSIGNED_BYTE);
		
		if (TempImage == NULL) {
			return IL_FALSE;
		}
	}
	else {
		TempImage = context->impl->iCurImage;
	}

	/*#ifndef _UNICODE
		File = TIFFOpen(Filename, "w");
	#else
		File = TIFFOpenW(Filename, "w");
	#endif*/

	// Control writing functions ourself.
	File = iTIFFOpen(context, (char*)"w");
	if (File == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return IL_FALSE;
	}

#ifdef _UNICODE
	sprintf(Description, "Tiff generated by %Ls", ilGetString(IL_VERSION_NUM));
#else// _UNICODE
	sprintf(Description, "Tiff generated by %s", ilGetString(context, IL_VERSION_NUM));
#endif// _UNICODE

	TIFFSetField(File, TIFFTAG_IMAGEWIDTH, TempImage->Width);
	TIFFSetField(File, TIFFTAG_IMAGELENGTH, TempImage->Height);
	TIFFSetField(File, TIFFTAG_COMPRESSION, Compression);
	TIFFSetField(File, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(File, TIFFTAG_BITSPERSAMPLE, TempImage->Bpc << 3);
	TIFFSetField(File, TIFFTAG_SAMPLESPERPIXEL, TempImage->Bpp);
	if (TempImage->Bpp == 4) //TODO: LUMINANCE, LUMINANCE_ALPHA
		TIFFSetField(File, TIFFTAG_MATTEING, 1);
	TIFFSetField(File, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(File, TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(File, TIFFTAG_SOFTWARE, ilGetString(context, IL_VERSION_NUM));  //@TODO: Will probably not work properly under Windows if Unicode
	/*TIFFSetField(File, TIFFTAG_DOCUMENTNAME,
		iGetString(context, IL_TIF_DOCUMENTNAME_STRING) ?
		iGetString(context, IL_TIF_DOCUMENTNAME_STRING) : FileName);
*/
	str = iGetString(context, IL_TIF_DOCUMENTNAME_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_DOCUMENTNAME, str);
		ifree(str);
	}


	str = iGetString(context, IL_TIF_AUTHNAME_STRING);
	if (iGetString(context, IL_TIF_AUTHNAME_STRING)) {
		TIFFSetField(File, TIFFTAG_ARTIST, str);
		ifree(str);
	}

	str = iGetString(context, IL_TIF_HOSTCOMPUTER_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_HOSTCOMPUTER, str);
		ifree(str);
	}

	str = iGetString(context, IL_TIF_HOSTCOMPUTER_STRING);
	if (str) {
		TIFFSetField(File, TIFFTAG_IMAGEDESCRIPTION, str);
		ifree(str);
	}

	// Set the date and time string.
	TIFFSetField(File, TIFFTAG_DATETIME, iMakeString());

	// 24/4/2003
	// Orientation flag is not always supported (Photoshop, ...), orient the image data 
	// and set it always to normal view
	TIFFSetField(File, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	if (TempImage->Origin != IL_ORIGIN_UPPER_LEFT) {
		ILubyte* Data = iGetFlipped(context, TempImage);
		OldData = TempImage->Data;
		TempImage->Data = Data;
	}
	else
		OldData = TempImage->Data;

	/*
	 TIFFSetField(File, TIFFTAG_ORIENTATION,
				  TempImage->Origin == IL_ORIGIN_UPPER_LEFT ? ORIENTATION_TOPLEFT : ORIENTATION_BOTLEFT);
	 */

	Format = TempImage->Format;
	SwapColors = (Format == IL_BGR || Format == IL_BGRA);
	if (SwapColors)
 		ilSwapColours(context);

	for (ixLine = 0; ixLine < TempImage->Height; ++ixLine) {
		if (TIFFWriteScanline(File, TempImage->Data + ixLine * TempImage->Bps, ixLine, 0) < 0) {
			TIFFClose(File);
			ilSetError(context, IL_LIB_TIFF_ERROR);
			if (SwapColors)
				ilSwapColours(context);
			if (TempImage->Data != OldData) {
				ifree( TempImage->Data );
				TempImage->Data = OldData;
			}
			if (TempImage != context->impl->iCurImage)
				ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}

	if (SwapColors)
 		ilSwapColours(context);

	if (TempImage->Data != OldData) {
		ifree(TempImage->Data);
		TempImage->Data = OldData;
	}

	if (TempImage != context->impl->iCurImage)
		ilCloseImage(TempImage);

	TIFFClose(File);

	return IL_TRUE;
}

/*----------------------------------------------------------------------------*/
// Makes a neat date string for the date field.
// From http://www.awaresystems.be/imaging/tiff/tifftags/datetime.html :
// The format is: "YYYY:MM:DD HH:MM:SS", with hours like those on
// a 24-hour clock, and one space character between the date and the
// time. The length of the string, including the terminating NUL, is
// 20 bytes.)
char *iMakeString()
{
	static char TimeStr[20];
	time_t		Time;
	struct tm	*CurTime;

	imemclear(TimeStr, 20);

	time(&Time);
#ifdef _WIN32
	_tzset();
#endif
	CurTime = localtime(&Time);

	strftime(TimeStr, 20, "%Y:%m:%d %H:%M:%S", CurTime);
	
	return TimeStr;
}

/*----------------------------------------------------------------------------*/

#endif//IL_NO_TIF
