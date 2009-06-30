/**
 * @file errorhandler.cpp
 * @brief error messages
 * @author Tobi Vollebregt <tobivollebregt@gmail.com>
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Error handling based on platform
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include <StdAfx.h>
#include "errorhandler.h"

#include "Game/GameServer.h"
#include "Sound/Sound.h"

#include <SDL.h>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include "Mac/MacUtils.h"
#endif

// from X_MessageBox.cpp:
void X_MessageBox(const char *msg, const char *caption, unsigned int flags);

void ErrorMessageBox (const char *msg, const char *caption, unsigned int flags)
{
	// Platform independent cleanup.
	SDL_Quit();
	// not exiting threads causes another exception
	delete gameServer; gameServer = NULL;
	delete sound; sound = NULL;

#ifdef _WIN32
	// Windows implementation, using MessageBox.

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = 0;		// MB_OK is default (0)
	
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;

	MessageBox (GetActiveWindow(), msg, caption, winFlags);

// TODO: write Mac OS X specific message box
 #elif defined(__APPLE__)
	MacMessageBox(msg, caption, flags);
#else
	// X implementation

	X_MessageBox(msg, caption, flags);

#endif

	exit(-1); // continuing execution when SDL_Quit has already been run will result in a crash
}

