
// Win32 support functions, this file contains general functions that need windows headers

#include "StdAfx.h"
#include "Platform/errorhandler.h"

#include <windows.h>

void ErrorMessageBox (const char *msg, const char *caption, unsigned int flags)
{
	MessageBox (GetActiveWindow(), msg, caption, flags);
}
