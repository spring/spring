/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MESSAGEBOX_H_
#define _MESSAGEBOX_H_

#include "System/Platform/errorhandler.h" // MBF_OK, etc.

namespace Platform {
	/**
	 * Will pop up an message window
	 * @param  message the main text, describing the error
	 * @param  caption will appear in the title bar of the error window
	 * @param  flags   @see errorhandler.h
	 */
	void MsgBox(const char* message, const char* caption, unsigned int flags);
}

#endif // _MESSAGEBOX_H_
