//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 11/07/2008
//
// Filename: src-IL/src/il_register.cpp
//
// Description: Allows the caller to specify user-defined callback functions
//				 to open files DevIL does not support, to parse files
//				 differently, or anything else a person can think up.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include "il_register.h"
#include <string.h>


// Linked lists of registered formats
iFormatL *LoadProcs = NULL;
iFormatS *SaveProcs = NULL;


ILboolean ILAPIENTRY ilRegisterLoad(ILcontext* context, ILconst_string Ext, IL_LOADPROC Load) {
	iFormatL *TempNode, *NewNode;

	TempNode = LoadProcs;
	if (TempNode != NULL) {
		while (TempNode->Next != NULL) {
			TempNode = TempNode->Next;
			if (!iStrCmp(TempNode->Ext, Ext)) {  // already registered
				return IL_TRUE;
			}
		}
	}

	NewNode = (iFormatL*)ialloc(context, sizeof(iFormatL));
	if (NewNode == NULL) {
		return IL_FALSE;
	}

	if (LoadProcs == NULL) {
		LoadProcs = NewNode;
	}
	else {
		TempNode->Next = NewNode;
	}

	NewNode->Ext = ilStrDup(context, Ext);
	NewNode->Load = Load;
	NewNode->Next = NULL;

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilRegisterSave(ILcontext* context, ILconst_string Ext, IL_SAVEPROC Save)
{
	iFormatS *TempNode, *NewNode;

	TempNode = SaveProcs;
	if (TempNode != NULL) {
		while (TempNode->Next != NULL) {
			TempNode = TempNode->Next;
			if (!iStrCmp(TempNode->Ext, Ext)) {  // already registered
				return IL_TRUE;
			}
		}
	}

	NewNode = (iFormatS*)ialloc(context, sizeof(iFormatL));
	if (NewNode == NULL) {
		return IL_FALSE;
	}

	if (SaveProcs == NULL) {
		SaveProcs = NewNode;
	}
	else {
		TempNode->Next = NewNode;
	}

	NewNode->Ext = ilStrDup(context, Ext);
	NewNode->Save = Save;
	NewNode->Next = NULL;

	return IL_TRUE;
}


//! Unregisters a load extension - doesn't have to be called.
ILboolean ILAPIENTRY ilRemoveLoad(ILconst_string Ext)
{
	iFormatL *TempNode = LoadProcs, *PrevNode = NULL;

	while (TempNode != NULL) {
		if (!iStrCmp(Ext, TempNode->Ext)) {
			if (PrevNode == NULL) {  // first node in the list
				LoadProcs = TempNode->Next;
				ifree((void*)TempNode->Ext);
				ifree(TempNode);
			}
			else {
				PrevNode->Next = TempNode->Next;
				ifree((void*)TempNode->Ext);
				ifree(TempNode);
			}

			return IL_TRUE;
		}

		PrevNode = TempNode;
		TempNode = TempNode->Next;
	}

	return IL_FALSE;
}


//! Unregisters a save extension - doesn't have to be called.
ILboolean ILAPIENTRY ilRemoveSave(ILconst_string Ext)
{
	iFormatS *TempNode = SaveProcs, *PrevNode = NULL;

	while (TempNode != NULL) {
		if (!iStrCmp(Ext, TempNode->Ext)) {
			if (PrevNode == NULL) {  // first node in the list
				SaveProcs = TempNode->Next;
				ifree((void*)TempNode->Ext);
				ifree(TempNode);
			}
			else {
				PrevNode->Next = TempNode->Next;
				ifree((void*)TempNode->Ext);
				ifree(TempNode);
			}

			return IL_TRUE;
		}

		PrevNode = TempNode;
		TempNode = TempNode->Next;
	}

	return IL_FALSE;
}


// Automatically removes all registered formats.
void ilRemoveRegistered()
{
	iFormatL *TempNodeL = LoadProcs;
	iFormatS *TempNodeS = SaveProcs;

	while (LoadProcs != NULL) {
		TempNodeL = LoadProcs->Next;
		ifree((void*)LoadProcs->Ext);
		ifree(LoadProcs);
		LoadProcs = TempNodeL;
	}

	while (SaveProcs != NULL) {
		TempNodeS = SaveProcs->Next;
		ifree((void*)SaveProcs->Ext);
		ifree(SaveProcs);
		SaveProcs = TempNodeS;
	}

	return;
}


ILboolean iRegisterLoad(ILcontext* context, ILconst_string FileName)
{
	iFormatL	*TempNode = LoadProcs;
	ILstring	Ext = iGetExtension(FileName);
	ILenum		Error;

	if (!Ext)
		return IL_FALSE;

	while (TempNode != NULL) {
		if (!iStrCmp(Ext, TempNode->Ext)) {
			Error = TempNode->Load(FileName);
			if (Error == IL_NO_ERROR || Error == 0) {  // 0 and IL_NO_ERROR are both valid.
				return IL_TRUE;
			}
			else {
				ilSetError(context, Error);
				return IL_FALSE;
			}
		}
		TempNode = TempNode->Next;
	}

	return IL_FALSE;
}


ILboolean iRegisterSave(ILcontext* context, ILconst_string FileName)
{
	iFormatS	*TempNode = SaveProcs;
	ILstring	Ext = iGetExtension(FileName);
	ILenum		Error;

	if (!Ext)
		return IL_FALSE;

	while (TempNode != NULL) {
		if (!iStrCmp(Ext, TempNode->Ext)) {
			Error = TempNode->Save(FileName);
			if (Error == IL_NO_ERROR || Error == 0) {  // 0 and IL_NO_ERROR are both valid.
				return IL_TRUE;
			}
			else {
				ilSetError(context, Error);
				return IL_FALSE;
			}
		}
		TempNode = TempNode->Next;
	}

	return IL_FALSE;
}


//
// "Reporting" functions
//

void ILAPIENTRY ilRegisterOrigin(ILcontext* context, ILenum Origin)
{
	switch (Origin)
	{
		case IL_ORIGIN_LOWER_LEFT:
		case IL_ORIGIN_UPPER_LEFT:
			context->impl->iCurImage->Origin = Origin;
			break;
		default:
			ilSetError(context, IL_INVALID_ENUM);
	}
	return;
}


void ILAPIENTRY ilRegisterFormat(ILcontext* context, ILenum Format)
{
	switch (Format)
	{
		case IL_COLOUR_INDEX:
		case IL_RGB:
		case IL_RGBA:
		case IL_BGR:
		case IL_BGRA:
		case IL_LUMINANCE:
                case IL_LUMINANCE_ALPHA:
			context->impl->iCurImage->Format = Format;
			break;
		default:
			ilSetError(context, IL_INVALID_ENUM);
	}
	return;
}


ILboolean ILAPIENTRY ilRegisterNumFaces(ILcontext* context, ILuint Num)
{
	ILimage *Next, *Prev;

	ilBindImage(context, ilGetCurName(context));  // Make sure the current image is actually bound.
	ilCloseImage(context->impl->iCurImage->Faces);  // Close any current mipmaps.

	context->impl->iCurImage->Faces = NULL;
	if (Num == 0)  // Just gets rid of all the mipmaps.
		return IL_TRUE;

	context->impl->iCurImage->Faces = ilNewImage(context, 1, 1, 1, 1, 1);
	if (context->impl->iCurImage->Faces == NULL)
		return IL_FALSE;
	Next = context->impl->iCurImage->Faces;
	Num--;

	while (Num) {
		Next->Faces = ilNewImage(context, 1, 1, 1, 1, 1);
		if (Next->Faces == NULL) {
			// Clean up before we error out.
			Prev = context->impl->iCurImage->Faces;
			while (Prev) {
				Next = Prev->Faces;
				ilCloseImage(Prev);
				Prev = Next;
			}
			return IL_FALSE;
		}
		Next = Next->Faces;
		Num--;
	}

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilRegisterMipNum(ILcontext* context, ILuint Num)
{
	ILimage *Next, *Prev;

	ilBindImage(context, ilGetCurName(context));  // Make sure the current image is actually bound.
	ilCloseImage(context->impl->iCurImage->Mipmaps);  // Close any current mipmaps.

	context->impl->iCurImage->Mipmaps = NULL;
	if (Num == 0)  // Just gets rid of all the mipmaps.
		return IL_TRUE;

	context->impl->iCurImage->Mipmaps = ilNewImage(context, 1, 1, 1, 1, 1);
	if (context->impl->iCurImage->Mipmaps == NULL)
		return IL_FALSE;
	Next = context->impl->iCurImage->Mipmaps;
	Num--;

	while (Num) {
		Next->Next = ilNewImage(context, 1, 1, 1, 1, 1);
		if (Next->Next == NULL) {
			// Clean up before we error out.
			Prev = context->impl->iCurImage->Mipmaps;
			while (Prev) {
				Next = Prev->Next;
				ilCloseImage(Prev);
				Prev = Next;
			}
			return IL_FALSE;
		}
		Next = Next->Next;
		Num--;
	}

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilRegisterNumImages(ILcontext* context, ILuint Num)
{
	ILimage *Next, *Prev;

	ilBindImage(context, ilGetCurName(context));  // Make sure the current image is actually bound.
	ilCloseImage(context->impl->iCurImage->Next);  // Close any current "next" images.

	context->impl->iCurImage->Next = NULL;
	if (Num == 0)  // Just gets rid of all the "next" images.
		return IL_TRUE;

	context->impl->iCurImage->Next = ilNewImage(context, 1, 1, 1, 1, 1);
	if (context->impl->iCurImage->Next == NULL)
		return IL_FALSE;
	Next = context->impl->iCurImage->Next;
	Num--;

	while (Num) {
		Next->Next = ilNewImage(context, 1, 1, 1, 1, 1);
		if (Next->Next == NULL) {
			// Clean up before we error out.
			Prev = context->impl->iCurImage->Next;
			while (Prev) {
				Next = Prev->Next;
				ilCloseImage(Prev);
				Prev = Next;
			}
			return IL_FALSE;
		}
		Next = Next->Next;
		Num--;
	}

	return IL_TRUE;
}


void ILAPIENTRY ilRegisterType(ILcontext* context, ILenum Type)
{
	switch (Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
		case IL_DOUBLE:
			context->impl->iCurImage->Type = Type;
			break;
		default:
			ilSetError(context, IL_INVALID_ENUM);
	}

	return;
}


void ILAPIENTRY ilRegisterPal(ILcontext* context, void *Pal, ILuint Size, ILenum Type)
{
	if (!context->impl->iCurImage->Pal.Palette || !context->impl->iCurImage->Pal.PalSize || context->impl->iCurImage->Pal.PalType != IL_PAL_NONE) {
		ifree(context->impl->iCurImage->Pal.Palette);
	}

	context->impl->iCurImage->Pal.PalSize = Size;
	context->impl->iCurImage->Pal.PalType = Type;
	context->impl->iCurImage->Pal.Palette = (ILubyte*)ialloc(context, Size);
	if (context->impl->iCurImage->Pal.Palette == NULL)
		return;

	if (Pal != NULL) {
		memcpy(context->impl->iCurImage->Pal.Palette, Pal, Size);
	}
	else {
		ilSetError(context, IL_INVALID_PARAM);
	}
	
	return;
}


ILboolean ILAPIENTRY ilSetDuration(ILcontext* context, ILuint Duration)
{
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	context->impl->iCurImage->Duration = Duration;

	return IL_TRUE;
}
