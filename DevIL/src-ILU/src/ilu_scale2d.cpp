//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001
//
// Filename: src-ILU/src/ilu_scale2d.cpp
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------

// NOTE:  Don't look at this file if you wish to preserve your sanity!

#include "ilu_context.h"
#include "ilu_internal.h"
#include "ilu_states.h"

ILimage *iluScale2DNear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DLinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);
ILimage *iluScale2DBilinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height);

ILimage *iluScale2D_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	if (Image == NULL) {
		ilSetError(context->ilContext, ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	context->ScaleX = (ILfloat)Width / Image->Width;
	context->ScaleY = (ILfloat)Height / Image->Height;

	if (iluFilter == ILU_NEAREST)
		return iluScale2DNear_(context, Image, Scaled, Width, Height);
	else if (iluFilter == ILU_LINEAR)
		return iluScale2DLinear_(context, Image, Scaled, Width, Height);
	// iluFilter == ILU_BILINEAR
	return iluScale2DBilinear_(context, Image, Scaled, Width, Height);
}

ILimage *iluScale2DNear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	context->ImgBps = Image->Bps / Image->Bpc;
	context->SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
	case 1:
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = context->y * context->SclBps;
			context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->NewX1 = context->x * Scaled->Bpp;
				context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					Scaled->Data[context->NewY1 + context->NewX1 + context->c] = Image->Data[context->NewY2 + context->NewX2 + context->c];
					context->x1 = 0;
				}
			}
		}
		break;

	case 2:
		context->ShortPtr = (ILushort*)Image->Data;
		context->SShortPtr = (ILushort*)Scaled->Data;
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = context->y * context->SclBps;
			context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->NewX1 = context->x * Scaled->Bpp;
				context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->SShortPtr[context->NewY1 + context->NewX1 + context->c] = context->ShortPtr[context->NewY2 + context->NewX2 + context->c];
					context->x1 = 0;
				}
			}
		}
		break;

	case 4:
		context->IntPtr = (ILuint*)Image->Data;
		context->SIntPtr = (ILuint*)Scaled->Data;
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = context->y * context->SclBps;
			context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->NewX1 = context->x * Scaled->Bpp;
				context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->SIntPtr[context->NewY1 + context->NewX1 + context->c] = context->IntPtr[context->NewY2 + context->NewX2 + context->c];
					context->x1 = 0;
				}
			}
		}
		break;
	}

	return Scaled;
}

ILimage *iluScale2DLinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	context->ImgBps = Image->Bps / Image->Bpc;
	context->SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
	case 1:
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->t2 = context->t4 - (ILuint)(context->t4);
				context->ft = context->t2 * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;
				context->NewX1 = ((ILuint)(context->t4 / context->ScaleX)) * Image->Bpp;
				context->NewX2 = ((ILuint)(context->t4 / context->ScaleX) + 1) * Image->Bpp;

				context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->x1 = Image->Data[context->NewY1 + context->NewX1 + context->c];
					context->x2 = Image->Data[context->NewY1 + context->NewX2 + context->c];
					Scaled->Data[context->Size + context->c] = (ILubyte)((1.0 - context->f) * context->x1 + context->f * context->x2);
				}
			}
		}
		break;

	case 2:
		context->ShortPtr = (ILushort*)Image->Data;
		context->SShortPtr = (ILushort*)Scaled->Data;
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->t2 = context->t4 - (ILuint)(context->t4);
				context->ft = context->t2 * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;
				context->NewX1 = ((ILuint)(context->t4 / context->ScaleX)) * Image->Bpp;
				context->NewX2 = ((ILuint)(context->t4 / context->ScaleX) + 1) * Image->Bpp;

				context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->x1 = context->ShortPtr[context->NewY1 + context->NewX1 + context->c];
					context->x2 = context->ShortPtr[context->NewY1 + context->NewX2 + context->c];
					context->SShortPtr[context->Size + context->c] = (ILushort)((1.0 - context->f) * context->x1 + context->f * context->x2);
				}
			}
		}
		break;

	case 4:
		context->IntPtr = (ILuint*)Image->Data;
		context->SIntPtr = (ILuint*)Scaled->Data;
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->t2 = context->t4 - (ILuint)(context->t4);
				context->ft = context->t2 * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;
				context->NewX1 = ((ILuint)(context->t4 / context->ScaleX)) * Image->Bpp;
				context->NewX2 = ((ILuint)(context->t4 / context->ScaleX) + 1) * Image->Bpp;

				context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->x1 = context->IntPtr[context->NewY1 + context->NewX1 + context->c];
					context->x2 = context->IntPtr[context->NewY1 + context->NewX2 + context->c];
					context->SIntPtr[context->Size + context->c] = (ILuint)((1.0 - context->f) * context->x1 + context->f * context->x2);
				}
			}
		}
		break;
	}

	return Scaled;
}

// Rewrote using an algorithm described by Paul Nettle at
//  http://www.gamedev.net/reference/articles/article669.asp.
ILimage *iluScale2DBilinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height)
{
	ILfloat	ul, ll, ur, lr;
	ILfloat	FracX, FracY;
	ILfloat	SrcX, SrcY;
	ILuint	iSrcX, iSrcY, iSrcXPlus1, iSrcYPlus1, ulOff, llOff, urOff, lrOff;

	context->ImgBps = Image->Bps / Image->Bpc;
	context->SclBps = Scaled->Bps / Scaled->Bpc;

	switch (Image->Bpc)
	{
	case 1:
		for (context->y = 0; context->y < Height; context->y++) {
			for (context->x = 0; context->x < Width; context->x++) {
				// Calculate where we want to choose pixels from in our source image.
				SrcX = (ILfloat)context->x / (ILfloat)context->ScaleX;
				SrcY = (ILfloat)context->y / (ILfloat)context->ScaleY;
				// Integer part of SrcX and SrcY
				iSrcX = (ILuint)floor(SrcX);
				iSrcY = (ILuint)floor(SrcY);
				// Fractional part of SrcX and SrcY
				FracX = SrcX - (ILfloat)(iSrcX);
				FracY = SrcY - (ILfloat)(iSrcY);

				// We do not want to go past the right edge of the image or past the last line in the image,
				//  so this takes care of that.  Normally, iSrcXPlus1 is iSrcX + 1, but if this is past the
				//  right side, we have to bring it back to iSrcX.  The same goes for iSrcYPlus1.
				if (iSrcX < Image->Width - 1)
					iSrcXPlus1 = iSrcX + 1;
				else
					iSrcXPlus1 = iSrcX;
				if (iSrcY < Image->Height - 1)
					iSrcYPlus1 = iSrcY + 1;
				else
					iSrcYPlus1 = iSrcY;

				// Find out how much we want each of the four pixels contributing to the final values.
				ul = (1.0f - FracX) * (1.0f - FracY);
				ll = (1.0f - FracX) * FracY;
				ur = FracX * (1.0f - FracY);
				lr = FracX * FracY;

				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					// We just calculate the offsets for each pixel here...
					ulOff = iSrcY * Image->Bps + iSrcX * Image->Bpp + context->c;
					llOff = iSrcYPlus1 * Image->Bps + iSrcX * Image->Bpp + context->c;
					urOff = iSrcY * Image->Bps + iSrcXPlus1 * Image->Bpp + context->c;
					lrOff = iSrcYPlus1 * Image->Bps + iSrcXPlus1 * Image->Bpp + context->c;

					// ...and then we do the actual interpolation here.
					Scaled->Data[context->y * Scaled->Bps + context->x * Scaled->Bpp + context->c] = (ILubyte)(
						ul * Image->Data[ulOff] + ll * Image->Data[llOff] + ur * Image->Data[urOff] + lr * Image->Data[lrOff]);
				}
			}
		}
		break;

	case 2:
		context->ShortPtr = (ILushort*)Image->Data;
		context->SShortPtr = (ILushort*)Scaled->Data;
		Height--;  // Only use regular Height once in the following loop.
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
			context->NewY2 = (ILuint)((context->y + 1) / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->NewX = Width / context->ScaleX;
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->t2 = context->t4 - (ILuint)(context->t4);
				context->t3 = (1.0 - context->t2);
				context->t4 = context->t1 * context->NewX;
				context->NewX1 = (ILuint)(context->t4)* Image->Bpp;
				context->NewX2 = (ILuint)(context->t4 + 1) * Image->Bpp;

				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->Table[0][context->c] = context->t3 * context->ShortPtr[context->NewY1 + context->NewX1 + context->c] +
						context->t2 * context->ShortPtr[context->NewY1 + context->NewX2 + context->c];

					context->Table[1][context->c] = context->t3 * context->ShortPtr[context->NewY2 + context->NewX1 + context->c] +
						context->t2 * context->ShortPtr[context->NewY2 + context->NewX2 + context->c];
				}

				// Linearly interpolate between the table values.
				context->t1 = context->y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
				context->t3 = (1.0 - context->t1);
				context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->SShortPtr[context->Size + context->c] =
						(ILushort)(context->t3 * context->Table[0][context->c] + context->t1 * context->Table[1][context->c]);
				}
			}
		}

		// Calculate the last row.
		context->NewY1 = (ILuint)(Height / context->ScaleY) * context->ImgBps;
		for (context->x = 0; context->x < Width; context->x++) {
			context->NewX = Width / context->ScaleX;
			context->t1 = context->x / (ILdouble)Width;
			context->t4 = context->t1 * Width;
			context->ft = (context->t4 - (ILuint)(context->t4)) * IL_PI;
			context->f = (1.0 - cos(context->ft)) * .5;  // Cosine interpolation
			context->NewX1 = (ILuint)(context->t1 * context->NewX) * Image->Bpp;
			context->NewX2 = (ILuint)(context->t1 * context->NewX + 1) * Image->Bpp;

			context->Size = Height * context->SclBps + context->x * Image->Bpp;
			for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
				context->SShortPtr[context->Size + context->c] = (ILushort)((1.0 - context->f) * context->ShortPtr[context->NewY1 + context->NewX1 + context->c] +
					context->f * context->ShortPtr[context->NewY1 + context->NewX2 + context->c]);
			}
		}
		break;

	case 4:
		if (Image->Type != IL_FLOAT) {
			context->IntPtr = (ILuint*)Image->Data;
			context->SIntPtr = (ILuint*)Scaled->Data;
			Height--;  // Only use regular Height once in the following loop.
			for (context->y = 0; context->y < Height; context->y++) {
				context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
				context->NewY2 = (ILuint)((context->y + 1) / context->ScaleY) * context->ImgBps;
				for (context->x = 0; context->x < Width; context->x++) {
					context->NewX = Width / context->ScaleX;
					context->t1 = context->x / (ILdouble)Width;
					context->t4 = context->t1 * Width;
					context->t2 = context->t4 - (ILuint)(context->t4);
					context->t3 = (1.0 - context->t2);
					context->t4 = context->t1 * context->NewX;
					context->NewX1 = (ILuint)(context->t4)* Image->Bpp;
					context->NewX2 = (ILuint)(context->t4 + 1) * Image->Bpp;

					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
						context->Table[0][context->c] = context->t3 * context->IntPtr[context->NewY1 + context->NewX1 + context->c] +
							context->t2 * context->IntPtr[context->NewY1 + context->NewX2 + context->c];

						context->Table[1][context->c] = context->t3 * context->IntPtr[context->NewY2 + context->NewX1 + context->c] +
							context->t2 * context->IntPtr[context->NewY2 + context->NewX2 + context->c];
					}

					// Linearly interpolate between the table values.
					context->t1 = context->y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					context->t3 = (1.0 - context->t1);
					context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;
					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
						context->SIntPtr[context->Size + context->c] =
							(ILuint)(context->t3 * context->Table[0][context->c] + context->t1 * context->Table[1][context->c]);
					}
				}
			}

			// Calculate the last row.
			context->NewY1 = (ILuint)(Height / context->ScaleY) * context->ImgBps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->NewX = Width / context->ScaleX;
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->ft = (context->t4 - (ILuint)(context->t4)) * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;  // Cosine interpolation
				context->NewX1 = (ILuint)(context->t1 * context->NewX) * Image->Bpp;
				context->NewX2 = (ILuint)(context->t1 * context->NewX + 1) * Image->Bpp;

				context->Size = Height * context->SclBps + context->x * Image->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->SIntPtr[context->Size + context->c] = (ILuint)((1.0 - context->f) * context->IntPtr[context->NewY1 + context->NewX1 + context->c] +
						context->f * context->IntPtr[context->NewY1 + context->NewX2 + context->c]);
				}
			}

		}

		else {  // IL_FLOAT
			context->FloatPtr = (ILfloat*)Image->Data;
			context->SFloatPtr = (ILfloat*)Scaled->Data;
			Height--;  // Only use regular Height once in the following loop.

			for (context->y = 0; context->y < Height; context->y++) {

				context->NewY1 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;

				context->NewY2 = (ILuint)((context->y + 1) / context->ScaleY) * context->ImgBps;

				for (context->x = 0; context->x < Width; context->x++) {

					context->NewX = Width / context->ScaleX;

					context->t1 = context->x / (ILdouble)Width;

					context->t4 = context->t1 * Width;

					context->t2 = context->t4 - (ILuint)(context->t4);

					context->t3 = (1.0 - context->t2);

					context->t4 = context->t1 * context->NewX;

					context->NewX1 = (ILuint)(context->t4)* Image->Bpp;

					context->NewX2 = (ILuint)(context->t4 + 1) * Image->Bpp;



					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {

						context->Table[0][context->c] = context->t3 * context->FloatPtr[context->NewY1 + context->NewX1 + context->c] +

							context->t2 * context->FloatPtr[context->NewY1 + context->NewX2 + context->c];



						context->Table[1][context->c] = context->t3 * context->FloatPtr[context->NewY2 + context->NewX1 + context->c] +

							context->t2 * context->FloatPtr[context->NewY2 + context->NewX2 + context->c];

					}



					// Linearly interpolate between the table values.

					context->t1 = context->y / (ILdouble)(Height + 1);  // Height+1 is the real height now.

					context->t3 = (1.0 - context->t1);

					context->Size = context->y * context->SclBps + context->x * Scaled->Bpp;

					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {

						context->SFloatPtr[context->Size + context->c] =

							(ILfloat)(context->t3 * context->Table[0][context->c] + context->t1 * context->Table[1][context->c]);

					}

				}

			}



			// Calculate the last row.

			context->NewY1 = (ILuint)(Height / context->ScaleY) * context->ImgBps;

			for (context->x = 0; context->x < Width; context->x++) {

				context->NewX = Width / context->ScaleX;

				context->t1 = context->x / (ILdouble)Width;

				context->t4 = context->t1 * Width;

				context->ft = (context->t4 - (ILuint)(context->t4)) * IL_PI;

				context->f = (1.0 - cos(context->ft)) * .5;  // Cosine interpolation

				context->NewX1 = (ILuint)(context->t1 * context->NewX) * Image->Bpp;

				context->NewX2 = (ILuint)(context->t1 * context->NewX + 1) * Image->Bpp;



				context->Size = Height * context->SclBps + context->x * Image->Bpp;

				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {

					context->SFloatPtr[context->Size + context->c] = (ILfloat)((1.0 - context->f) * context->FloatPtr[context->NewY1 + context->NewX1 + context->c] +

						context->f * context->FloatPtr[context->NewY1 + context->NewX2 + context->c]);

				}

			}

		}

		break;
	}

	return Scaled;
}