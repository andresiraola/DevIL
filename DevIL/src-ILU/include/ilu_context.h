#pragma once

#include "IL/il.h"

struct ILUcontext
{
    ILcontext* ilContext;

	// Resize variables
	ILuint		x1, x2;
	ILuint		NewY1, NewY2, NewX1, NewX2, NewZ1, NewZ2;
	ILuint		Size;
	ILuint		x, y, z, c;
	ILdouble	ScaleX, ScaleY, ScaleZ;
	ILdouble	t1, t2, t3, t4, f, ft, NewX;
	ILdouble	Table[2][4];  // Assumes we don't have larger than 32-bit images.
	ILuint		ImgBps, SclBps, ImgPlane, SclPlane;
	ILushort	*ShortPtr, *SShortPtr;
	ILuint		*IntPtr, *SIntPtr;
	ILfloat		*FloatPtr, *SFloatPtr;
};