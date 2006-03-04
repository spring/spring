
// Win32 support functions, this file contains general functions that need windows headers

#include "StdAfx.h"
#include "Platform/errorhandler.h"

#include <SDL.h>
#include <windows.h>

void ErrorMessageBox (const char *msg, const char *caption, unsigned int flags)
{
	SDL_Quit();

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = 0;		// MB_OK is default (0)
	
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;

	MessageBox (GetActiveWindow(), msg, caption, winFlags);
}
