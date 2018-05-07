//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/29/2009
//
// Filename: src-IL/include/il_endian.h
//
// Description: Handles Endian-ness
//
//-----------------------------------------------------------------------------

#ifndef IL_ENDIAN_H
#define IL_ENDIAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "il_internal.h"

#ifdef WORDS_BIGENDIAN  // This is defined by ./configure.
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__ 1
#endif
#endif

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __BIG_ENDIAN__) \
  || (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#undef __LITTLE_ENDIAN__
#define Short(s) iSwapShort(s)
#define UShort(s) iSwapUShort(s)
#define Int(i) iSwapInt(i)
#define UInt(i) iSwapUInt(i)
#define Float(f) iSwapFloat(f)
#define Double(d) iSwapDouble(d)

#define BigShort(s)  
#define BigUShort(s)  
#define BigInt(i)  
#define BigUInt(i)  
#define BigFloat(f)  
#define BigDouble(d)  
#else
#undef __BIG_ENDIAN__
#if !defined(__LITTLE_ENDIAN__) 
#undef __LITTLE_ENDIAN__  // Not sure if it's defined by any compiler...
#define __LITTLE_ENDIAN__
#endif
#define Short(s)  
#define UShort(s)  
#define Int(i)  
#define UInt(i)  
#define Float(f)  
#define Double(d)  

#define BigShort(s) iSwapShort(s)
#define BigUShort(s) iSwapUShort(s)
#define BigInt(i) iSwapInt(i)
#define BigUInt(i) iSwapUInt(i)
#define BigFloat(f) iSwapFloat(f)
#define BigDouble(d) iSwapDouble(d)
#endif

#ifdef IL_ENDIAN_C
#undef NOINLINE
#undef INLINE
#define INLINE
#endif

void   iSwapUShort(ILushort *s);
void   iSwapShort(ILshort *s);
void   iSwapUInt(ILuint *i);
void   iSwapInt(ILint *i);
void   iSwapFloat(ILfloat *f);
void   iSwapDouble(ILdouble *d);
ILushort GetLittleUShort(ILcontext* context);
ILshort  GetLittleShort(ILcontext* context);
ILuint   GetLittleUInt(ILcontext* context);
ILint    GetLittleInt(ILcontext* context);
ILfloat  GetLittleFloat(ILcontext* context);
ILdouble GetLittleDouble(ILcontext* context);
ILushort GetBigUShort(ILcontext* context);
ILshort  GetBigShort(ILcontext* context);
ILuint   GetBigUInt(ILcontext* context);
ILint    GetBigInt(ILcontext* context);
ILfloat  GetBigFloat(ILcontext* context);
ILdouble GetBigDouble(ILcontext* context);
ILubyte SaveLittleUShort(ILcontext* context, ILushort s);
ILubyte SaveLittleShort(ILcontext* context, ILshort s);
ILubyte SaveLittleUInt(ILcontext* context, ILuint i);
ILubyte SaveLittleInt(ILcontext* context, ILint i);
ILubyte SaveLittleFloat(ILcontext* context, ILfloat f);
ILubyte SaveLittleDouble(ILcontext* context, ILdouble d);
ILubyte SaveBigUShort(ILcontext* context, ILushort s);
ILubyte SaveBigShort(ILcontext* context, ILshort s);
ILubyte SaveBigUInt(ILcontext* context, ILuint i);
ILubyte SaveBigInt(ILcontext* context, ILint i);
ILubyte SaveBigFloat(ILcontext* context, ILfloat f);
ILubyte SaveBigDouble(ILcontext* context, ILdouble d);

void EndianSwapData(void *_Image);

#ifdef __cplusplus
}
#endif

#endif//ENDIAN_H