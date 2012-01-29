/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <windows.h>

#include "System/Platform/errorhandler.h"

void X_MessageBox(const char *msg, const char *caption, unsigned int flags)
{
	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = MB_TOPMOST;
	// MB_OK is default (0)
	if (flags & MBF_EXCL)  winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)  winFlags |= MB_ICONINFORMATION;
	if (flags & MBF_CRASH) winFlags |= MB_ICONERROR;
	MessageBox(GetActiveWindow(), msg, caption, winFlags);
}

