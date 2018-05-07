//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 12/17/2008
//
// Filename: src-IL/src/il_stack.cpp
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

// Credit goes to John Villar (johnny@reliaschev.com) for making the suggestion
//	of not letting the user use ILimage structs but instead binding images
//	like OpenGL.

#include "il_internal.h"

static ILboolean iEnlargeStack(ILcontext* context);

//! Creates Num images and puts their index in Images - similar to glGenTextures().
void ILAPIENTRY ilGenImages(ILcontext* context, ILsizei Num, ILuint *Images)
{
	ILsizei	Index = 0;
	iFree	*TempFree = context->impl->FreeNames;

	if (Num < 1 || Images == NULL) {
		ilSetError(context, IL_INVALID_VALUE);
		return;
	}

	// No images have been generated yet, so create the image stack.
	if (context->impl->ImageStack == NULL)
		if (!iEnlargeStack(context))
			return;

	do {
		if (context->impl->FreeNames != NULL) {  // If any have been deleted, then reuse their image names.
			TempFree = (iFree*)context->impl->FreeNames->Next;
			Images[Index] = context->impl->FreeNames->Name;
			context->impl->ImageStack[context->impl->FreeNames->Name] = ilNewImage(context, 1, 1, 1, 1, 1);
			ifree(context->impl->FreeNames);
			context->impl->FreeNames = TempFree;
		}
		else {
			if (context->impl->LastUsed >= context->impl->StackSize)
				if (!iEnlargeStack(context))
					return;
			Images[Index] = context->impl->LastUsed;
			// Must be all 1's instead of 0's, because some functions would divide by 0.
			context->impl->ImageStack[context->impl->LastUsed] = ilNewImage(context, 1, 1, 1, 1, 1);
			context->impl->LastUsed++;
		}
	} while (++Index < Num);

	return;
}

ILuint ILAPIENTRY ilGenImage(ILcontext* context)
{
    ILuint i;
    ilGenImages(context, 1,&i);
    return i;
}

//! Makes Image the current active image - similar to glBindTexture().
void ILAPIENTRY ilBindImage(ILcontext* context, ILuint Image)
{
	if (context->impl->ImageStack == NULL || context->impl->StackSize == 0) {
		if (!iEnlargeStack(context)) {
			return;
		}
	}

	// If the user requests a high image name.
	while (Image >= context->impl->StackSize) {
		if (!iEnlargeStack(context)) {
			return;
		}
	}

	if (context->impl->ImageStack[Image] == NULL) {
		context->impl->ImageStack[Image] = ilNewImage(context, 1, 1, 1, 1, 1);
		if (Image >= context->impl->LastUsed) // >= ?
			context->impl->LastUsed = Image + 1;
	}

	context->impl->iCurImage = context->impl->ImageStack[Image];
	context->impl->CurName = Image;

	context->impl->ParentImage = IL_TRUE;

	return;
}


//! Deletes Num images from the image stack - similar to glDeleteTextures().
void ILAPIENTRY ilDeleteImages(ILcontext* context, ILsizei Num, const ILuint *Images)
{
	iFree	*Temp = context->impl->FreeNames;
	ILuint	Index = 0;

	if (Num < 1) {
		//ilSetError(context, IL_INVALID_VALUE);
		return;
	}
	if (context->impl->StackSize == 0)
		return;

	do {
		if (Images[Index] > 0 && Images[Index] < context->impl->LastUsed) {  // <= ?
			/*if (context->impl->FreeNames != NULL) {  // Terribly inefficient
				Temp = context->impl->FreeNames;
				do {
					if (Temp->Name == Images[Index]) {
						continue;  // Sufficient?
					}
				} while ((Temp = Temp->Next));
			}*/

			// Already has been deleted or was never used.
			if (context->impl->ImageStack[Images[Index]] == NULL)
				continue;

			// Find out if current image - if so, set to default image zero.
			if (Images[Index] == context->impl->CurName || Images[Index] == 0) {
				context->impl->iCurImage = context->impl->ImageStack[0];
				context->impl->CurName = 0;
			}
			
			// Should *NOT* be NULL here!
			ilCloseImage(context->impl->ImageStack[Images[Index]]);
			context->impl->ImageStack[Images[Index]] = NULL;

			// Add to head of list - works for empty and non-empty lists
			Temp = (iFree*)ialloc(context, sizeof(iFree));
			if (!Temp) {
				return;
			}
			Temp->Name = Images[Index];
			Temp->Next = context->impl->FreeNames;
			context->impl->FreeNames = Temp;
		}
		/*else {  // Shouldn't set an error...just continue onward.
			ilSetError(context, IL_ILLEGAL_OPERATION);
		}*/
	} while (++Index < (ILuint)Num);
}


void ILAPIENTRY ilDeleteImage(ILcontext* context, const ILuint Num) {
    ilDeleteImages(context, 1,&Num);
}

//! Checks if Image is a valid ilGenImages-generated image (like glIsTexture()).
ILboolean ILAPIENTRY ilIsImage(ILcontext* context, ILuint Image)
{
	//iFree *Temp = context->impl->FreeNames;

	if (context->impl->ImageStack == NULL)
		return IL_FALSE;
	if (Image >= context->impl->LastUsed || Image == 0)
		return IL_FALSE;

	/*do {
		if (Temp->Name == Image)
			return IL_FALSE;
	} while ((Temp = Temp->Next));*/

	if (context->impl->ImageStack[Image] == NULL)  // Easier check.
		return IL_FALSE;

	return IL_TRUE;
}


//! Closes Image and frees all memory associated with it.
ILAPI void ILAPIENTRY ilCloseImage(ILimage *Image)
{
	if (Image == NULL)
		return;

	if (Image->Data != NULL) {
		ifree(Image->Data);
		Image->Data = NULL;
	}

	if (Image->Pal.Palette != NULL && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
	}

	if (Image->Next != NULL) {
		ilCloseImage(Image->Next);
		Image->Next = NULL;
	}

	if (Image->Faces != NULL) {
		ilCloseImage(Image->Faces);
		Image->Mipmaps = NULL;
	}

	if (Image->Mipmaps != NULL) {
		ilCloseImage(Image->Mipmaps);
		Image->Mipmaps = NULL;
	}

	if (Image->Layers != NULL) {
		ilCloseImage(Image->Layers);
		Image->Layers = NULL;
	}

	if (Image->AnimList != NULL && Image->AnimSize != 0) {
		ifree(Image->AnimList);
		Image->AnimList = NULL;
	}

	if (Image->Profile != NULL && Image->ProfileSize != 0) {
		ifree(Image->Profile);
		Image->Profile = NULL;
		Image->ProfileSize = 0;
	}

	if (Image->DxtcData != NULL && Image->DxtcFormat != IL_DXT_NO_COMP) {
		ifree(Image->DxtcData);
		Image->DxtcData = NULL;
		Image->DxtcFormat = IL_DXT_NO_COMP;
		Image->DxtcSize = 0;
	}

	ifree(Image);
	Image = NULL;

	return;
}


ILAPI ILboolean ILAPIENTRY ilIsValidPal(ILpal *Palette)
{
	if (Palette == NULL)
		return IL_FALSE;
	if (Palette->PalSize == 0 || Palette->Palette == NULL)
		return IL_FALSE;
	switch (Palette->PalType)
	{
		case IL_PAL_RGB24:
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR24:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			return IL_TRUE;
	}
	return IL_FALSE;
}


//! Closes Palette and frees all memory associated with it.
ILAPI void ILAPIENTRY ilClosePal(ILpal *Palette)
{
	if (Palette == NULL)
		return;
	if (!ilIsValidPal(Palette))
		return;
	ifree(Palette->Palette);
	ifree(Palette);
	return;
}


ILimage *iGetBaseImage(ILcontext* context)
{
	return context->impl->ImageStack[ilGetCurName(context)];
}


//! Sets the current mipmap level
ILboolean ILAPIENTRY ilActiveMipmap(ILcontext* context, ILuint Number)
{
	ILuint Current;
    ILimage *iTempImage;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Number == 0) {
		return IL_TRUE;
	}

    iTempImage = context->impl->iCurImage;
	context->impl->iCurImage = context->impl->iCurImage->Mipmaps;
	if (context->impl->iCurImage == NULL) {
		context->impl->iCurImage = iTempImage;
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	for (Current = 1; Current < Number; Current++) {
		context->impl->iCurImage = context->impl->iCurImage->Mipmaps;
		if (context->impl->iCurImage == NULL) {
			ilSetError(context, IL_ILLEGAL_OPERATION);
			context->impl->iCurImage = iTempImage;
			return IL_FALSE;
		}
	}

	context->impl->ParentImage = IL_FALSE;

	return IL_TRUE;
}


//! Used for setting the current image if it is an animation.
ILboolean ILAPIENTRY ilActiveImage(ILcontext* context, ILuint Number)
{
	ILuint Current;
    ILimage *iTempImage;
    
	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Number == 0) {
		return IL_TRUE;
	}

    iTempImage = context->impl->iCurImage;
	context->impl->iCurImage = context->impl->iCurImage->Next;
	if (context->impl->iCurImage == NULL) {
		context->impl->iCurImage = iTempImage;
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Number--;  // Skip 0 (parent image)
	for (Current = 0; Current < Number; Current++) {
		context->impl->iCurImage = context->impl->iCurImage->Next;
		if (context->impl->iCurImage == NULL) {
			ilSetError(context, IL_ILLEGAL_OPERATION);
			context->impl->iCurImage = iTempImage;
			return IL_FALSE;
		}
	}

	context->impl->ParentImage = IL_FALSE;

	return IL_TRUE;
}


//! Used for setting the current face if it is a cubemap.
ILboolean ILAPIENTRY ilActiveFace(ILcontext* context, ILuint Number)
{
	ILuint Current;
    ILimage *iTempImage;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Number == 0) {
		return IL_TRUE;
	}

    iTempImage = context->impl->iCurImage;
	context->impl->iCurImage = context->impl->iCurImage->Faces;
	if (context->impl->iCurImage == NULL) {
		context->impl->iCurImage = iTempImage;
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//Number--;  // Skip 0 (parent image)
	for (Current = 1; Current < Number; Current++) {
		context->impl->iCurImage = context->impl->iCurImage->Faces;
		if (context->impl->iCurImage == NULL) {
			ilSetError(context, IL_ILLEGAL_OPERATION);
			context->impl->iCurImage = iTempImage;
			return IL_FALSE;
		}
	}

	context->impl->ParentImage = IL_FALSE;

	return IL_TRUE;
}



//! Used for setting the current layer if layers exist.
ILboolean ILAPIENTRY ilActiveLayer(ILcontext* context, ILuint Number)
{
	ILuint Current;
    ILimage *iTempImage;

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (Number == 0) {
		return IL_TRUE;
	}

    iTempImage = context->impl->iCurImage;
	context->impl->iCurImage = context->impl->iCurImage->Layers;
	if (context->impl->iCurImage == NULL) {
		context->impl->iCurImage = iTempImage;
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	//Number--;  // Skip 0 (parent image)
	for (Current = 1; Current < Number; Current++) {
		context->impl->iCurImage = context->impl->iCurImage->Layers;
		if (context->impl->iCurImage == NULL) {
			ilSetError(context, IL_ILLEGAL_OPERATION);
			context->impl->iCurImage = iTempImage;
			return IL_FALSE;
		}
	}

	context->impl->ParentImage = IL_FALSE;

	return IL_TRUE;
}


ILuint ILAPIENTRY ilCreateSubImage(ILcontext* context, ILenum Type, ILuint Num)
{
	ILimage	*SubImage;
	ILuint	Count ;  // Create one before we go in the loop.

	if (context->impl->iCurImage == NULL) {
		ilSetError(context, IL_ILLEGAL_OPERATION);
		return 0;
	}
	if (Num == 0)  {
		return 0;
	}

	switch (Type)
	{
		case IL_SUB_NEXT:
			if (context->impl->iCurImage->Next)
				ilCloseImage(context->impl->iCurImage->Next);
			context->impl->iCurImage->Next = ilNewImage(context, 1, 1, 1, 1, 1);
			SubImage = context->impl->iCurImage->Next;
			break;

		case IL_SUB_MIPMAP:
			if (context->impl->iCurImage->Mipmaps)
				ilCloseImage(context->impl->iCurImage->Mipmaps);
			context->impl->iCurImage->Mipmaps = ilNewImage(context, 1, 1, 1, 1, 1);
			SubImage = context->impl->iCurImage->Mipmaps;
			break;

		case IL_SUB_LAYER:
			if (context->impl->iCurImage->Layers)
				ilCloseImage(context->impl->iCurImage->Layers);
			context->impl->iCurImage->Layers = ilNewImage(context, 1, 1, 1, 1, 1);
			SubImage = context->impl->iCurImage->Layers;
			break;

		default:
			ilSetError(context, IL_INVALID_ENUM);
			return IL_FALSE;
	}

	if (SubImage == NULL) {
		return 0;
	}

	for (Count = 1; Count < Num; Count++) {
		SubImage->Next = ilNewImage(context, 1, 1, 1, 1, 1);
		SubImage = SubImage->Next;
		if (SubImage == NULL)
			return Count;
	}

	return Count;
}


// Returns the current index.
ILAPI ILuint ILAPIENTRY ilGetCurName(ILcontext* context)
{
	if (context->impl->iCurImage == NULL || context->impl->ImageStack == NULL || context->impl->StackSize == 0)
		return 0;
	return context->impl->CurName;
}


// Returns the current image.
ILAPI ILimage* ILAPIENTRY ilGetCurImage(ILcontext* context)
{
	return context->impl->iCurImage;
}


// To be only used when the original image is going to be set back almost immediately.
ILAPI void ILAPIENTRY ilSetCurImage(ILcontext* context, ILimage *Image)
{
	context->impl->iCurImage = Image;
	return;
}


// Completely replaces the current image and the version in the image stack.
ILAPI void ILAPIENTRY ilReplaceCurImage(ILcontext* context, ILimage *Image)
{
	if (context->impl->iCurImage) {
		ilActiveImage(context, 0);
		ilCloseImage(context->impl->iCurImage);
	}
	context->impl->ImageStack[ilGetCurName(context)] = Image;
	context->impl->iCurImage = Image;
	context->impl->ParentImage = IL_TRUE;
	return;
}


// Like realloc but sets new memory to 0.
void* ILAPIENTRY ilRecalloc(ILcontext* context, void *Ptr, ILuint OldSize, ILuint NewSize)
{
	void *Temp = ialloc(context, NewSize);
	ILuint CopySize = (OldSize < NewSize) ? OldSize : NewSize;

	if (Temp != NULL) {
		if (Ptr != NULL) {
			memcpy(Temp, Ptr, CopySize);
			ifree(Ptr);
		}

		Ptr = Temp;

		if (OldSize < NewSize)
			imemclear((ILubyte*)Temp + OldSize, NewSize - OldSize);
	}

	return Temp;
}


// To keep Visual Studio happy with its unhappiness with ILAPIENTRY for atexit
void ilShutDownInternal()
{
	//ilShutDown(context);
}

// Internal function to enlarge the image stack by I_STACK_INCREMENT members.
ILboolean iEnlargeStack(ILcontext* context)
{
	// 02-05-2001:  Moved from ilGenImages().
	// Puts the cleanup function on the exit handler once.
	if (!context->impl->OnExit) {
		#ifdef _MEM_DEBUG
			AddToAtexit();  // So iFreeMem doesn't get called after unfreed information.
		#endif//_MEM_DEBUG
#if (!defined(_WIN32_WCE)) && (!defined(IL_STATIC_LIB))
			atexit(ilShutDownInternal);
#endif
		context->impl->OnExit = IL_TRUE;
	}

	if (!(context->impl->ImageStack = (ILimage**)ilRecalloc(context, context->impl->ImageStack, context->impl->StackSize * sizeof(ILimage*), (context->impl->StackSize + I_STACK_INCREMENT) * sizeof(ILimage*)))) {
		return IL_FALSE;
	}
	context->impl->StackSize += I_STACK_INCREMENT;
	return IL_TRUE;
}

// To keep Visual Studio happy with its unhappiness with ILAPIENTRY for atexit
void ilRemoveRegisteredInternal()
{
	ilRemoveRegistered();
}

// ONLY call at startup.
ILcontext* ILAPIENTRY ilInit()
{
	ILcontext* context = new ILcontext();
	
	//ilSetMemory(NULL, NULL);  Now useless 3/4/2006 (due to modification in il_alloc.c)
	ilSetError(context, IL_NO_ERROR);
	ilDefaultStates(context);  // Set states to their defaults.
	// Sets default file-reading callbacks.
	ilResetRead(context);
	ilResetWrite(context);
#if (!defined(_WIN32_WCE)) && (!defined(IL_STATIC_LIB))
	atexit(ilRemoveRegisteredInternal);
#endif
	//_WIN32_WCE
	//ilShutDown();
	iSetImage0(context);  // Beware!  Clears all existing textures!
	iBindImageTemp(context);  // Go ahead and create the temporary image.
	
	return context;
}


// Frees any extra memory in the stack.
//	- Called on exit
void ILAPIENTRY ilShutDown(ILcontext* context)
{
	// if it is not initialized do not shutdown
	iFree* TempFree = (iFree*)context->impl->FreeNames;
	ILuint i;
	
	while (TempFree != NULL) {
		context->impl->FreeNames = (iFree*)TempFree->Next;
		ifree(TempFree);
		TempFree = context->impl->FreeNames;
	}

	//for (i = 0; i < context->impl->LastUsed; i++) {
	for (i = 0; i < context->impl->StackSize; i++) {
		if (context->impl->ImageStack[i] != NULL)
			ilCloseImage(context->impl->ImageStack[i]);
	}

	if (context->impl->ImageStack)
		ifree(context->impl->ImageStack);
	context->impl->ImageStack = NULL;
	context->impl->LastUsed = 0;
	context->impl->StackSize = 0;

	return;
}


// Initializes the image stack's first entry (default image) -- ONLY CALL ONCE!
void iSetImage0(ILcontext* context)
{
	if (context->impl->ImageStack == NULL)
		if (!iEnlargeStack(context))
			return;

	context->impl->LastUsed = 1;
	context->impl->CurName = 0;
	context->impl->ParentImage = IL_TRUE;
	if (!context->impl->ImageStack[0])
		context->impl->ImageStack[0] = ilNewImage(context, 1, 1, 1, 1, 1);
	context->impl->iCurImage = context->impl->ImageStack[0];
	ilDefaultImage(context);

	return;
}


ILAPI void ILAPIENTRY iBindImageTemp(ILcontext* context)
{
	if (context->impl->ImageStack == NULL || context->impl->StackSize <= 1)
		if (!iEnlargeStack(context))
			return;

	if (context->impl->LastUsed < 2)
		context->impl->LastUsed = 2;
	context->impl->CurName = 1;
	context->impl->ParentImage = IL_TRUE;
	if (!context->impl->ImageStack[1])
		context->impl->ImageStack[1] = ilNewImage(context, 1, 1, 1, 1, 1);
	context->impl->iCurImage = context->impl->ImageStack[1];

	return;
}
