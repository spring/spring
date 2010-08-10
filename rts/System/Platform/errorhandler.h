/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Platform specific error handling
 */

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <string>

#define MBF_OK    1
#define MBF_INFO  2
#define MBF_EXCL  4

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
void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags = MBF_OK) NO_RETURN;


#endif // ERRORHANDLER_H
