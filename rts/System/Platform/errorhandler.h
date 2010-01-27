/**
 * @file errorhandler.h
 * @brief error messages
 * @author Christopher Han <xiphux@gmail.com>
 * @author Tobi Vollebregt <tobivollebregt@gmail.com>
 * @author Robin Vobruba <hoijui.quaero@gmail.com>
 *
 * Error handling based on platform
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <string>

#define MBF_OK		1
#define MBF_INFO	2
#define MBF_EXCL	4

#if defined __GNUC__ && (__GNUC__ >= 4)
	#define NO_RETURN __attribute__ ((noreturn))
#else
	#define NO_RETURN
#endif

// legacy support define
#define handleerror(o, m, c, f) ErrorMessageBox(m, c, f)

/**
 * Will pop up an error message window and exit (C version).
 * @param  msg     the main text, describing the error
 * @param  caption will appear in the title bar of the error window
 * @param  flags   one of:
 *                 - MB_OK   : Error
 *                 - MB_EXCL : Warning
 *                 - MB_INFO : Info
 */
void ErrorMessageBox(const char* msg, const char* caption, unsigned int flags) NO_RETURN;

/**
 * Will pop up an error message window and exit (C++ version).
 * @see ErrorMessageBox(const char* msg, const char* caption, unsigned int flags)
 */
inline void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags = MBF_OK)
{
	ErrorMessageBox(msg.c_str(), caption.c_str(), flags);
};

#endif // ERRORHANDLER_H
