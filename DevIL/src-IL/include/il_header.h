#pragma once

#include "il_internal.h"

class CHeaderHandler
{
protected:
	ILcontext* context;
	char* internalName;

public:
	CHeaderHandler(ILcontext* context, char* internalName);

	ILboolean	save(ILconst_string FileName);
};