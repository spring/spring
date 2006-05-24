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
#include "Platform/errorhandler.h"

#include <SDL.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <iostream>
#endif

void ErrorMessageBox (const char *msg, const char *caption, unsigned int flags)
{
	// Platform independent cleanup.
	SDL_Quit();

#ifdef _WIN32
	// Windows implementation, using MessageBox.

	// Translate spring flags to corresponding win32 dialog flags
	unsigned int winFlags = 0;		// MB_OK is default (0)
	
	if (flags & MBF_EXCL)
		winFlags |= MB_ICONEXCLAMATION;
	if (flags & MBF_INFO)
		winFlags |= MB_ICONINFORMATION;

	MessageBox (GetActiveWindow(), msg, caption, winFlags);

#else
	// Platform independent implementation, using std::cerr.

#ifdef DEBUG
#ifdef __GNUC__
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << std::endl;
#else
#define DEBUGSTRING std::cerr << "  " << __FILE__ << ":" << std::dec << __LINE__ << std::endl;
#endif
#else
#define DEBUGSTRING
#endif

	if (flags & MBF_INFO)
		std::cerr << "Info: ";
	else if (flags & MBF_EXCL)
		std::cerr << "Warning: ";
	else
		std::cerr << "ERROR: ";
	std::cerr << caption << std::endl;
	std::cerr << "  " << msg << std::endl;
	if (!(flags & (MBF_INFO | MBF_EXCL))) {
		DEBUGSTRING
	}

#endif
}
