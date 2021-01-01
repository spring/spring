/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/MessageBox.h"
#include <windows.h>


namespace Platform {

/**
 * @brief message box function
 */
void MsgBox(const char* message, const char* caption, unsigned int flags)
{
	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = MB_TOPMOST | MB_OK;
	if (flags & MBF_EXCL)  winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)  winFlags |= MB_ICONINFORMATION;
	if (flags & MBF_CRASH) winFlags |= MB_ICONERROR;

	::MessageBox(GetActiveWindow(), message, caption, winFlags);
}

}; //namespace Platform
