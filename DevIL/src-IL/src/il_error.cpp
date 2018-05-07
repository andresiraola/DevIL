//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 06/02/2007
//
// Filename: src-IL/src/il_error.cpp
//
// Description: The error functions
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


// Sets the current error
//	If you go past the stack size for this, it cycles the errors, almost like a LRU algo.
ILAPI void ILAPIENTRY ilSetError(ILcontext* context, ILenum Error)
{
	ILuint i;

	context->impl->ilErrorPlace++;
	if (context->impl->ilErrorPlace >= IL_ERROR_STACK_SIZE) {
		for (i = 0; i < IL_ERROR_STACK_SIZE - 2; i++) {
			context->impl->ilErrorNum[i] = context->impl->ilErrorNum[i+1];
		}
		context->impl->ilErrorPlace = IL_ERROR_STACK_SIZE - 1;
	}
	context->impl->ilErrorNum[context->impl->ilErrorPlace] = Error;

	return;
}


//! Gets the last error on the error stack
ILenum ILAPIENTRY ilGetError(ILcontext* context)
{
	ILenum ilReturn;

	if (context->impl->ilErrorPlace >= 0) {
		ilReturn = context->impl->ilErrorNum[context->impl->ilErrorPlace];
		context->impl->ilErrorPlace--;
	}
	else
		ilReturn = IL_NO_ERROR;

	return ilReturn;
}
