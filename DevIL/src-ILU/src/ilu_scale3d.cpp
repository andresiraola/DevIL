//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 05/25/2001
//
// Filename: src-ILU/src/ilu_scale.cpp
//
// Description: Scales an image.
//
//-----------------------------------------------------------------------------

// NOTE:  Don't look at this file if you wish to preserve your sanity!

#include "ilu_context.h"
#include "ilu_internal.h"
#include "ilu_states.h"

ILimage *iluScale3DNear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
ILimage *iluScale3DLinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
//ILimage *iluScale3DBilinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);

ILimage *iluScale3D_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	if (Image == NULL) {
		ilSetError(context->ilContext, ILU_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	context->ScaleX = (ILfloat)Width / Image->Width;
	context->ScaleY = (ILfloat)Height / Image->Height;
	context->ScaleZ = (ILfloat)Depth / Image->Depth;

	//if (iluFilter == ILU_NEAREST)
		return iluScale3DNear_(context, Image, Scaled, Width, Height, Depth);
	//else if (iluFilter == ILU_LINEAR)
		//return iluScale3DLinear_(Image, Scaled, Width, Height, Depth);
	
	// iluFilter == ILU_BILINEAR
	//return iluScale3DBilinear_(Image, Scaled, Width, Height, Depth);
}

ILimage *iluScale3DNear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	context->ImgBps = Image->Bps / Image->Bpc;
	context->SclBps = Scaled->Bps / Scaled->Bpc;
	context->ImgPlane = Image->SizeOfPlane / Image->Bpc;
	context->SclPlane = Scaled->SizeOfPlane / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = context->z * context->SclPlane;
				context->NewZ2 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
				for (context->y = 0; context->y < Height; context->y++) {
					context->NewY1 = context->y * context->SclBps;
					context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
					for (context->x = 0; context->x < Width; context->x++) {
						context->NewX1 = context->x * Scaled->Bpp;
						context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							Scaled->Data[context->NewZ1 + context->NewY1 + context->NewX1 + context->c] =
								Image->Data[context->NewZ2 + context->NewY2 + context->NewX2 + context->c];
						}
					}
				}
			}
			break;

		case 2:
			context->ShortPtr = (ILushort*)Image->Data;
			context->SShortPtr = (ILushort*)Scaled->Data;
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = context->z * context->SclPlane;
				context->NewZ2 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
				for (context->y = 0; context->y < Height; context->y++) {
					context->NewY1 = context->y * context->SclBps;
					context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
					for (context->x = 0; context->x < Width; context->x++) {
						context->NewX1 = context->x * Scaled->Bpp;
						context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							context->SShortPtr[context->NewZ1 + context->NewY1 + context->NewX1 + context->c] =
								context->ShortPtr[context->NewZ2 + context->NewY2 + context->NewX2 + context->c];
						}
					}
				}
			}
			break;

		case 4:
			context->IntPtr = (ILuint*)Image->Data;
			context->SIntPtr = (ILuint*)Scaled->Data;
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = context->z * context->SclPlane;
				context->NewZ2 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
				for (context->y = 0; context->y < Height; context->y++) {
					context->NewY1 = context->y * context->SclBps;
					context->NewY2 = (ILuint)(context->y / context->ScaleY) * context->ImgBps;
					for (context->x = 0; context->x < Width; context->x++) {
						context->NewX1 = context->x * Scaled->Bpp;
						context->NewX2 = (ILuint)(context->x / context->ScaleX) * Image->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							context->SIntPtr[context->NewZ1 + context->NewY1 + context->NewX1 + context->c] =
								context->IntPtr[context->NewZ2 + context->NewY2 + context->NewX2 + context->c];
						}
					}
				}
			}
			break;
	}

	return Scaled;
}

ILimage *iluScale3DLinear_(ILUcontext* context, ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth)
{
	context->ImgBps = Image->Bps / Image->Bpc;
	context->SclBps = Scaled->Bps / Scaled->Bpc;
	context->ImgPlane = Image->SizeOfPlane / Image->Bpc;
	context->SclPlane = Scaled->SizeOfPlane / Scaled->Bpc;

	switch (Image->Bpc)
	{
		case 1:
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
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

						context->Size = context->z * context->SclPlane + context->y * context->SclBps + context->x * Scaled->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							context->x1 = Image->Data[context->NewZ1 + context->NewY1 + context->NewX1 + context->c];
							context->x2 = Image->Data[context->NewZ1 + context->NewY1 + context->NewX2 + context->c];
							Scaled->Data[context->Size + context->c] = (ILubyte)((1.0 - context->f) * context->x1 + context->f * context->x2);
						}
					}
				}
			}
			break;

		case 2:
			context->ShortPtr = (ILushort*)Image->Data;
			context->SShortPtr = (ILushort*)Scaled->Data;
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
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

						context->Size = context->z * context->SclPlane + context->y * context->SclBps + context->x * Scaled->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							context->x1 = context->ShortPtr[context->NewZ1 + context->NewY1 + context->NewX1 + context->c];
							context->x2 = context->ShortPtr[context->NewZ1 + context->NewY1 + context->NewX2 + context->c];
							context->SShortPtr[context->Size + context->c] = (ILubyte)((1.0 - context->f) * context->x1 + context->f * context->x2);
						}
					}
				}
			}
			break;

		case 4:
			context->IntPtr = (ILuint*)Image->Data;
			context->SIntPtr = (ILuint*)Scaled->Data;
			for (context->z = 0; context->z < Depth; context->z++) {
				context->NewZ1 = (ILuint)(context->z / context->ScaleZ) * context->ImgPlane;
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

						context->Size = context->z * context->SclPlane + context->y * context->SclBps + context->x * Scaled->Bpp;
						for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
							context->x1 = context->IntPtr[context->NewZ1 + context->NewY1 + context->NewX1 + context->c];
							context->x2 = context->IntPtr[context->NewZ1 + context->NewY1 + context->NewX2 + context->c];
							context->SIntPtr[context->Size + context->c] = (ILubyte)((1.0 - context->f) * context->x1 + context->f * context->x2);
						}
					}
				}
			}
			break;
	}

	return Scaled;
}

/*ILimage *iluScale3DBilinear_(ILimage *Image, ILimage *Scaled, ILuint Width, ILuint Height, ILuint Depth);
{
		Depth--;  // Only use regular Depth once in the following loop.
		Height--;  // Only use regular Height once in the following loop.
		for (context->z = 0; context->z < Depth; context->z++) {
			context->NewZ1 = (ILuint)(context->z / context->ScaleZ) * Image->SizeOfPlane;
			context->NewZ2 = (ILuint)((context->z+1) / context->ScaleZ) * Image->SizeOfPlane;
			for (context->y = 0; context->y < Height; context->y++) {
				context->NewY1 = (ILuint)(context->y / context->ScaleY) * Image->Bps;
				context->NewY2 = (ILuint)((context->y+1) / context->ScaleY) * Image->Bps;
				for (context->x = 0; context->x < Width; context->x++) {
					NewX = Width / context->ScaleX;
					context->t1 = context->x / (ILdouble)Width;
					context->t4 = context->t1 * Width;
					context->t2 = context->t4 - (ILuint)(context->t4);
					t3 = (1.0 - context->t2);
					context->t4 = context->t1 * NewX;
					context->NewX1 = (ILuint)(context->t4) * Image->Bpp;
					context->NewX2 = (ILuint)(context->t4 + 1) * Image->Bpp;

					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
						Table[0][0][context->c] = t3 * Image->Data[context->NewZ1 + context->NewY1 + context->NewX1 + context->c] +
							context->t2 * Image->Data[context->NewZ1 + context->NewY1 + context->NewX2 + context->c];

						Table[0][1][context->c] = t3 * Image->Data[context->NewZ1 + context->NewY2 + context->NewX1 + context->c] +
							context->t2 * Image->Data[context->NewZ1 + context->NewY2 + context->NewX2 + context->c];

						Table[1][0][context->c] = t3 * Image->Data[context->NewZ2 + context->NewY1 + context->NewX1 + context->c] +
							context->t2 * Image->Data[context->NewZ2 + context->NewY1 + context->NewX2 + context->c];

						Table[1][1][context->c] = t3 * Image->Data[context->NewZ2 + context->NewY2 + context->NewX1 + context->c] +
							context->t2 * Image->Data[context->NewZ2 + context->NewY2 + context->NewX2 + context->c];
					}

					// Linearly interpolate between the table values.
					context->t1 = context->y / (ILdouble)(Height + 1);  // Height+1 is the real height now.
					context->t2 = context->z / (ILdouble)(Depth + 1);   // Depth+1 is the real depth now.
					t3 = (1.0 - context->t1);
					context->Size = context->z * Scaled->SizeOfPlane + context->y * Scaled->Bps + context->x * Scaled->Bpp;
					for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
						context->x1 = t3 * Table[0][0][context->c] + context->t1 * Table[0][1][context->c];
						context->x2 = t3 * Table[1][0][context->c] + context->t1 * Table[1][1][context->c];
						Scaled->Data[context->Size + context->c] = (ILubyte)((1.0 - context->t2) * context->x1 + context->t2 * context->x2);
					}
				}
			}

			// Calculate the last row.
			context->NewY1 = (ILuint)(Height / context->ScaleY) * Image->Bps;
			for (context->x = 0; context->x < Width; context->x++) {
				NewX = Width / context->ScaleX;
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->ft = (context->t4 - (ILuint)(context->t4)) * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;  // Cosine interpolation
				context->NewX1 = (ILuint)(context->t1 * NewX) * Image->Bpp;
				context->NewX2 = (ILuint)(context->t1 * NewX + 1) * Image->Bpp;

				context->Size = Height * Scaled->Bps + context->x * Image->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					Scaled->Data[context->Size + context->c] = (ILubyte)((1.0 - context->f) * Image->Data[context->NewY1 + context->NewX1 + context->c] +
						context->f * Image->Data[context->NewY1 + context->NewX2 + context->c]);
				}
			}
		}

		context->NewZ1 = (ILuint)(Depth / context->ScaleZ) * Image->SizeOfPlane;
		for (context->y = 0; context->y < Height; context->y++) {
			context->NewY1 = (ILuint)(context->y / context->ScaleY) * Image->Bps;
			for (context->x = 0; context->x < Width; context->x++) {
				context->t1 = context->x / (ILdouble)Width;
				context->t4 = context->t1 * Width;
				context->t2 = context->t4 - (ILuint)(context->t4);
				context->ft = context->t2 * IL_PI;
				context->f = (1.0 - cos(context->ft)) * .5;
				context->NewX1 = ((ILuint)(context->t4 / context->ScaleX)) * Image->Bpp;
				context->NewX2 = ((ILuint)(context->t4 / context->ScaleX) + 1) * Image->Bpp;

				context->Size = (Depth) * Scaled->SizeOfPlane + context->y * Scaled->Bps + context->x * Scaled->Bpp;
				for (context->c = 0; context->c < Scaled->Bpp; context->c++) {
					context->x1 = Image->Data[context->NewZ1 + context->NewY1 + context->NewX1 + context->c];
					context->x2 = Image->Data[context->NewZ1 + context->NewY1 + context->NewX2 + context->c];
					Scaled->Data[context->Size + context->c] = (ILubyte)((1.0 - context->f) * context->x1 + context->f * context->x2);
				}
			}
		}
	}

	return Scaled;
}
*/

