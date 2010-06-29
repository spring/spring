/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Error handling based on platform.
 */

#include <StdAfx.h>
#include "errorhandler.h"

#include "Game/GameServer.h"
#ifndef DEDICATED
#include "Sound/ISound.h"
#endif

#include <SDL.h>

#if       !defined(DEDICATED) && !defined(HEADLESS)
#if       defined(WIN32)
#include <windows.h>
#endif // defined(WIN32)

// from X_MessageBox.cpp:
void X_MessageBox(const char *msg, const char *caption, unsigned int flags);
#else  // !defined(DEDICATED) && !defined(HEADLESS)
#include "LogOutput.h"
#endif // !defined(DEDICATED) && !defined(HEADLESS)

void ErrorMessageBox (const char *msg, const char *caption, unsigned int flags)
{
	// Platform independent cleanup.
	SDL_Quit();
	// not exiting threads causes another exception
	delete gameServer; gameServer = NULL;
#ifndef DEDICATED
	ISound::Shutdown();
#endif


#if       !defined(DEDICATED) && !defined(HEADLESS)
#if       defined(WIN32)
	// Windows implementation, using MessageBox.

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = 0;		// MB_OK is default (0)
	
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;

	MessageBox (GetActiveWindow(), msg, caption, winFlags);
#else  // defined(WIN32)
	// X implementation
// TODO: write Mac OS X specific message box

	X_MessageBox(msg, caption, flags);
#endif // defined(WIN32)
#else  // !defined(DEDICATED) && !defined(HEADLESS)
	LogObject() << msg;
#endif // !defined(DEDICATED) && !defined(HEADLESS)

	exit(-1); // continuing execution when SDL_Quit has already been run will result in a crash
}

