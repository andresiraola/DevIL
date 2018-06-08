//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 01/04/2009
//
// Filename: src-IL/src/il_xpm.cpp
//
// Description: Reads from an .xpm file.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"

#ifndef IL_NO_XPM

#include <ctype.h>

#include "il_xpm.h"

//If this is defined, only xpm files with 1 char/pixel
//can be loaded. They load somewhat faster then, though
//(not much).
//#define XPM_DONT_USE_HASHTABLE

ILint XpmGetsInternal(ILcontext* context, ILubyte *Buffer, ILint MaxLen);

XpmHandler::XpmHandler(ILcontext* context) :
	context(context)
{

}

//! Checks if the file specified in FileName is a valid XPM file.
ILboolean XpmHandler::isValid(ILconst_string FileName)
{
	ILHANDLE	XpmFile;
	ILboolean	bXpm = IL_FALSE;
	
	if (!iCheckExtension(FileName, IL_TEXT("xpm"))) {
		ilSetError(context, IL_INVALID_EXTENSION);
		return bXpm;
	}
	
	XpmFile = context->impl->iopenr(FileName);
	if (XpmFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bXpm;
	}
	
	bXpm = isValidF(XpmFile);
	context->impl->icloser(XpmFile);
	
	return bXpm;
}

//! Checks if the ILHANDLE contains a valid XPM file at the current position.
ILboolean XpmHandler::isValidF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;
	
	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = isValidInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);
	
	return bRet;
}

//! Checks if Lump is a valid XPM lump.
ILboolean XpmHandler::isValidL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return isValidInternal();
}

// Internal function to get the header and check it.
ILboolean XpmHandler::isValidInternal()
{
	ILubyte	Buffer[10];
	ILuint	Pos = context->impl->itell(context);

	XpmGetsInternal(context, Buffer, 10);
	context->impl->iseek(context, Pos, IL_SEEK_SET);  // Restore position

	if (strncmp("/* XPM */", (char*)Buffer, strlen("/* XPM */")))
		return IL_FALSE;
	return IL_TRUE;
}

// Reads an .xpm file
ILboolean XpmHandler::load(ILconst_string FileName)
{
	ILHANDLE	XpmFile;
	ILboolean	bXpm = IL_FALSE;

	XpmFile = context->impl->iopenr(FileName);
	if (XpmFile == NULL) {
		ilSetError(context, IL_COULD_NOT_OPEN_FILE);
		return bXpm;
	}

	iSetInputFile(context, XpmFile);
	bXpm = loadF(XpmFile);
	context->impl->icloser(XpmFile);

	return bXpm;
}

//! Reads an already-opened .xpm file
ILboolean XpmHandler::loadF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(context, File);
	FirstPos = context->impl->itell(context);
	bRet = loadInternal();
	context->impl->iseek(context, FirstPos, IL_SEEK_SET);

	return bRet;
}

//! Reads from a memory "lump" that contains an .xpm
ILboolean XpmHandler::loadL(const void *Lump, ILuint Size)
{
	iSetInputLump(context, Lump, Size);
	return loadInternal();
}

typedef ILubyte XpmPixel[4];

#define XPM_MAX_CHAR_PER_PIXEL 2

#ifndef XPM_DONT_USE_HASHTABLE

//The following hash table code was inspired by the xpm
//loading code of xv, one of the best image viewers of X11

//For xpm files with more than one character/pixel, it is
//impractical to use a simple lookup table for the
//character-to-color mapping (because the table requires
//2^(chars/pixel) entries, this is quite big).
//Because of that, a hash table is used for the mapping.
//The hash table has 257 entries, and collisions are
//resolved by chaining.

//257 is the smallest prime > 256
#define XPM_HASH_LEN 257

typedef struct XPMHASHENTRY
{
	ILubyte ColourName[XPM_MAX_CHAR_PER_PIXEL];
	XpmPixel ColourValue;
	struct XPMHASHENTRY *Next;
} XPMHASHENTRY;

static ILuint XpmHash(const ILubyte* name, int len)
{
	ILint i, sum;
	for (sum = i = 0; i < len; ++i)
		sum += name[i];
	return sum % XPM_HASH_LEN;
}

XPMHASHENTRY** XpmCreateHashTable(ILcontext* context)
{
	XPMHASHENTRY** Table =
		(XPMHASHENTRY**)ialloc(context, XPM_HASH_LEN*sizeof(XPMHASHENTRY*));
	if (Table != NULL)
		memset(Table, 0, XPM_HASH_LEN*sizeof(XPMHASHENTRY*));
	return Table;
}

void XpmDestroyHashTable(XPMHASHENTRY **Table)
{
	ILint i;
	XPMHASHENTRY* Entry;

	for (i = 0; i < XPM_HASH_LEN; ++i) {
		while (Table[i] != NULL) {
			Entry = Table[i]->Next;
			ifree(Table[i]);
			Table[i] = Entry;
		}
	}

	ifree(Table);
}

void XpmInsertEntry(ILcontext* context, XPMHASHENTRY **Table, const ILubyte* Name, int Len, XpmPixel Colour)
{
	XPMHASHENTRY* NewEntry;
	ILuint Index;
	Index = XpmHash(Name, Len);

	NewEntry = (XPMHASHENTRY*)ialloc(context, sizeof(XPMHASHENTRY));
	if (NewEntry != NULL) {
		NewEntry->Next = Table[Index];
		memcpy(NewEntry->ColourName, Name, Len);
		memcpy(NewEntry->ColourValue, Colour, sizeof(XpmPixel));
		Table[Index] = NewEntry;
	}
}

void XpmGetEntry(XPMHASHENTRY **Table, const ILubyte* Name, int Len, XpmPixel Colour)
{
	XPMHASHENTRY* Entry;
	ILuint Index;

	Index = XpmHash(Name, Len);
	Entry = Table[Index];
	while (Entry != NULL && strncmp((char*)(Entry->ColourName), (char*)Name, Len) != 0)
		Entry = Entry->Next;

	if (Entry != NULL)
		memcpy(Colour, Entry->ColourValue, sizeof(XpmPixel));
}

#endif //XPM_DONT_USE_HASHTABLE

ILint XpmGetsInternal(ILcontext* context, ILubyte *Buffer, ILint MaxLen)
{
	ILint	i = 0, Current;

	if (context->impl->ieof(context))
		return IL_EOF;

	while ((Current = context->impl->igetc(context)) != IL_EOF && i < MaxLen - 1) {
		if (Current == IL_EOF)
			return 0;
		if (Current == '\n') //unix line ending
			break;

		if (Current == '\r') { //dos/mac line ending
			Current = context->impl->igetc(context);
			if (Current == '\n') //dos line ending
				break;

			if (Current == IL_EOF)
				break;

			Buffer[i++] = (ILubyte)Current;
			continue;
		}
		Buffer[i++] = (ILubyte)Current;
	}

	Buffer[i++] = 0;

	return i;
}

ILint XpmGets(ILcontext* context, ILubyte *Buffer, ILint MaxLen)
{
	ILint		Size, i, j;
	ILboolean	NotComment = IL_FALSE, InsideComment = IL_FALSE;

	do {
		Size = XpmGetsInternal(context, Buffer, MaxLen);
		if (Size == IL_EOF)
			return IL_EOF;

		//skip leading whitespace (sometimes there's whitespace
		//before a comment or before the pixel data)

		for(i = 0; i < Size && isspace(Buffer[i]); ++i) ;
		Size = Size - i;
		for(j = 0; j < Size; ++j)
			Buffer[j] = Buffer[j + i];

		if (Size == 0)
			continue;

		if (Buffer[0] == '/' && Buffer[1] == '*') {
			for (i = 2; i < Size; i++) {
				if (Buffer[i] == '*' && Buffer[i+1] == '/') {
					break;
				}
			}
			if (i >= Size)
				InsideComment = IL_TRUE;
		}
		else if (InsideComment) {
			for (i = 0; i < Size; i++) {
				if (Buffer[i] == '*' && Buffer[i+1] == '/') {
					break;
				}
			}
			if (i < Size)
				InsideComment = IL_FALSE;
		}
		else {
			NotComment = IL_TRUE;
		}
	} while (!NotComment);

	return Size;
}

ILint XpmGetInt(ILubyte *Buffer, ILint Size, ILint *Position)
{
	char		Buff[1024];
	ILint		i, j;
	ILboolean	IsInNum = IL_FALSE;

	for (i = *Position, j = 0; i < Size; i++) {
		if (isdigit(Buffer[i])) {
			IsInNum = IL_TRUE;
			Buff[j++] = Buffer[i];
		}
		else {
			if (IsInNum) {
				Buff[j] = 0;
				*Position = i;
				return atoi(Buff);
			}
		}
	}

	return -1;
}

ILboolean XpmPredefCol(char *Buff, XpmPixel *Colour)
{
	ILint len;
	ILint val = 128;

	if (!stricmp(Buff, "none")) {
		(*Colour)[0] = 0;
		(*Colour)[1] = 0;
		(*Colour)[2] = 0;
		(*Colour)[3] = 0;
		return IL_TRUE;
	}

	(*Colour)[3] = 255;

	if (!stricmp(Buff, "black")) {
		(*Colour)[0] = 0;
		(*Colour)[1] = 0;
		(*Colour)[2] = 0;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "white")) {
		(*Colour)[0] = 255;
		(*Colour)[1] = 255;
		(*Colour)[2] = 255;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "red")) {
		(*Colour)[0] = 255;
		(*Colour)[1] = 0;
		(*Colour)[2] = 0;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "green")) {
		(*Colour)[0] = 0;
		(*Colour)[1] = 255;
		(*Colour)[2] = 0;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "blue")) {
		(*Colour)[0] = 0;
		(*Colour)[1] = 0;
		(*Colour)[2] = 255;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "yellow")) {
		(*Colour)[0] = 255;
		(*Colour)[1] = 255;
		(*Colour)[2] = 0;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "cyan")) {
		(*Colour)[0] = 0;
		(*Colour)[1] = 255;
		(*Colour)[2] = 255;
		return IL_TRUE;
	}
	if (!stricmp(Buff, "gray")) {
		(*Colour)[0] = 128;
		(*Colour)[1] = 128;
		(*Colour)[2] = 128;
		return IL_TRUE;
	}

	//check for grayXXX codes (added 20040218)
	len = ilCharStrLen(Buff);
	if (len >= 4) {
		if (Buff[0] == 'g' || Buff[0] == 'G'
			|| Buff[1] == 'r' || Buff[1] == 'R'
			|| Buff[2] == 'a' || Buff[2] == 'A'
			|| Buff[3] == 'y' || Buff[3] == 'Y') {
			if (isdigit(Buff[4])) { // isdigit returns false on '\0'
				val = Buff[4] - '0';
				if (isdigit(Buff[5])) {
					val = val*10 + Buff[5] - '0';
					if (isdigit(Buff[6]))
						val = val*10 + Buff[6] - '0';
				}
				val = (255*val)/100;
			}
			(*Colour)[0] = (ILubyte)val;
			(*Colour)[1] = (ILubyte)val;
			(*Colour)[2] = (ILubyte)val;
			return IL_TRUE;
		}
	}


	// Unknown colour string, so use black
	// (changed 20040218)
	(*Colour)[0] = 0;
	(*Colour)[1] = 0;
	(*Colour)[2] = 0;

	return IL_FALSE;
}

#ifndef XPM_DONT_USE_HASHTABLE
ILboolean XpmGetColour(ILcontext* context, ILubyte *Buffer, ILint Size, int Len, XPMHASHENTRY **Table)
#else
ILboolean XpmGetColour(ILcontext* context, ILubyte *Buffer, ILint Size, int Len, XpmPixel* Colours)
#endif
{
	ILint		i = 0, j, strLen = 0;
	ILubyte		ColBuff[3];
	char		Buff[1024];

	XpmPixel	Colour;
	ILubyte		Name[XPM_MAX_CHAR_PER_PIXEL];

	for ( ; i < Size; i++) {
		if (Buffer[i] == '\"')
			break;
	}
	i++;  // Skip the quotes.

	if (i >= Size)
		return IL_FALSE;

	// Get the characters.
	for (j = 0; j < Len; ++j) {
		Name[j] = Buffer[i++];
	}

	// Skip to the colour definition.
	for ( ; i < Size; i++) {
		if (Buffer[i] == 'c')
			break;
	}
	i++;  // Skip the 'c'.

	if (i >= Size || Buffer[i] != ' ') { // no 'c' found...assume black
#ifndef XPM_DONT_USE_HASHTABLE
		memset(Colour, 0, sizeof(Colour));
		Colour[3] = 255;
		XpmInsertEntry(context, Table, Name, Len, Colour);
#else
		memset(Colours[Name[0]], 0, sizeof(Colour));
		Colours[Name[0]][3] = 255;
#endif
		return IL_TRUE;
	}

	for ( ; i < Size; i++) {
		if (Buffer[i] != ' ')
			break;
	}

	if (i >= Size)
		return IL_FALSE;

	if (Buffer[i] == '#') {
		// colour string may 4 digits/color or 1 digit/color
		// (added 20040218) TODO: is isxdigit() ANSI???
		++i;
		while (i + strLen < Size && isxdigit(Buffer[i + strLen]))
			++strLen;

		for (j = 0; j < 3; j++) {
			if (strLen >= 10) { // 4 digits
				ColBuff[0] = Buffer[i + j*4];
				ColBuff[1] = Buffer[i + j*4 + 1];
			}
			else if (strLen >= 8) { // 3 digits
				ColBuff[0] = Buffer[i + j*3];
				ColBuff[1] = Buffer[i + j*3 + 1];
			}
			else if (strLen >= 6) { // 2 digits
				ColBuff[0] = Buffer[i + j*2];
				ColBuff[1] = Buffer[i + j*2 + 1];
			}
			else if(j < strLen) { // 1 digit, strLen >= 1
				ColBuff[0] = Buffer[i + j];
				ColBuff[1] = 0;
			}

			ColBuff[2] = 0; // add terminating '\0' char
			Colour[j] = (ILubyte)strtol((char*)ColBuff, NULL, 16);
		}
		Colour[3] = 255;  // Full alpha.
	}
	else {
		for (j = 0; i < Size; i++) {
			if (!isalnum(Buffer[i]))
				break;
			Buff[j++] = Buffer[i];
		}
		Buff[j] = 0;

		if (i >= Size)
			return IL_FALSE;

		if (!XpmPredefCol(Buff, &Colour))

			return IL_FALSE;
	}


#ifndef XPM_DONT_USE_HASHTABLE
	XpmInsertEntry(context, Table, Name, Len, Colour);
#else
	memcpy(Colours[Name[0]], Colour, sizeof(Colour));
#endif
	return IL_TRUE;
}

ILboolean XpmHandler::loadInternal()
{
#define BUFFER_SIZE 2000
	ILubyte			Buffer[BUFFER_SIZE], *Data;
	ILint			Size, Pos, Width, Height, NumColours, i, x, y;

	ILint			CharsPerPixel;

#ifndef XPM_DONT_USE_HASHTABLE
	XPMHASHENTRY	**HashTable;
#else
	XpmPixel	*Colours;
	ILint		Offset;
#endif

	Size = XpmGetsInternal(context, Buffer, BUFFER_SIZE);
	if (strncmp("/* XPM */", (char*)Buffer, strlen("/* XPM */"))) {
		ilSetError(context, IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	Size = XpmGets(context, Buffer, BUFFER_SIZE);
	// @TODO:  Actually check the variable name here.

	Size = XpmGets(context, Buffer, BUFFER_SIZE);
	Pos = 0;
	Width = XpmGetInt(Buffer, Size, &Pos);
	Height = XpmGetInt(Buffer, Size, &Pos);
	NumColours = XpmGetInt(Buffer, Size, &Pos);

	CharsPerPixel = XpmGetInt(Buffer, Size, &Pos);

#ifdef XPM_DONT_USE_HASHTABLE
	if (CharsPerPixel != 1) {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
#endif

	if (CharsPerPixel > XPM_MAX_CHAR_PER_PIXEL
		|| Width*CharsPerPixel > BUFFER_SIZE) {
		ilSetError(context, IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

#ifndef XPM_DONT_USE_HASHTABLE
	HashTable = XpmCreateHashTable(context);
	if (HashTable == NULL)
		return IL_FALSE;
#else
	Colours = ialloc(context, 256 * sizeof(XpmPixel));
	if (Colours == NULL)
		return IL_FALSE;
#endif

	for (i = 0; i < NumColours; i++) {
		Size = XpmGets(context, Buffer, BUFFER_SIZE);
#ifndef XPM_DONT_USE_HASHTABLE
		if (!XpmGetColour(context, Buffer, Size, CharsPerPixel, HashTable)) {
			XpmDestroyHashTable(HashTable);
#else
		if (!XpmGetColour(Buffer, Size, CharsPerPixel, Colours)) {
			ifree(Colours);
#endif
			return IL_FALSE;
		}
	}
	
	if (!ilTexImage(context, Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL)) {
#ifndef XPM_DONT_USE_HASHTABLE
		XpmDestroyHashTable(HashTable);
#else
		ifree(Colours);
#endif
		return IL_FALSE;
	}

	Data = context->impl->iCurImage->Data;

	for (y = 0; y < Height; y++) {
		Size = XpmGets(context, Buffer, BUFFER_SIZE);
		for (x = 0; x < Width; x++) {
#ifndef XPM_DONT_USE_HASHTABLE
			XpmGetEntry(HashTable, &Buffer[1 + x*CharsPerPixel], CharsPerPixel, &Data[(x << 2)]);
#else
			Offset = (x << 2);
			Data[Offset + 0] = Colours[Buffer[x + 1]][0];
			Data[Offset + 1] = Colours[Buffer[x + 1]][1];
			Data[Offset + 2] = Colours[Buffer[x + 1]][2];
			Data[Offset + 3] = Colours[Buffer[x + 1]][3];
#endif
		}

		Data += context->impl->iCurImage->Bps;
	}

	//added 20040218
	context->impl->iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;


#ifndef XPM_DONT_USE_HASHTABLE
	XpmDestroyHashTable(HashTable);
#else
	ifree(Colours);
#endif
	return IL_TRUE;

#undef BUFFER_SIZE
}

#endif//IL_NO_XPM