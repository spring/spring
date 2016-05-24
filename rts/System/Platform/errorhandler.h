/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Platform specific error handling
 */

#ifndef _ERROR_HANDLER_H
#define _ERROR_HANDLER_H

#include <string>
#ifndef NO_CATCH_EXCEPTIONS
#include <sstream>
#endif
#include <boost/thread/exceptions.hpp>
#include <boost/system/system_error.hpp>
#include "System/Exceptions.h"

#define MBF_OK    1
#define MBF_INFO  2
#define MBF_EXCL  4
#define MBF_CRASH 8

// legacy support define
#define handleerror(o, m, c, f) ErrorMessageBox(m, c, f)

/**
 * Will pop up an error message window and exit (C version).
 * @param  msg     the main text, describing the error
 * @param  caption will appear in the title bar of the error window
 * @param  flags   one of:
 *                 - MBF_OK   : Error
 *                 - MBF_EXCL : Warning
 *                 - MBF_INFO : Info
 *                 - MBF_CRASH: Crash
 */
void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags = MBF_OK, bool fromMain = false);

/**
 * sets springs exit code
 * @param code that will be returned by the executable
 * TODO: add enum / use it
 */
void SetExitCode(int code);

/**
 * returns the exit code
 */
int GetExitCode();

/**
 * Spring's common exception handler.
 */
#define CATCH_SPRING_ERRORS_BASE                                                             \
	catch (const content_error& e) {                                                         \
		ErrorMessageBox(e.what(), "Spring: Incorrect/Missing content: ", MBF_OK | MBF_EXCL); \
	}                                                                                        \
	catch (const opengl_error& e) {                                                          \
		ErrorMessageBox(e.what(), "Spring: OpenGL content", MBF_OK | MBF_CRASH);             \
	}                                                                                        \
	catch (const user_error& e) {                                                            \
		ErrorMessageBox(e.what(), "Spring: Fatal Error (user)", MBF_OK | MBF_EXCL);          \
	}                                                                                        \
	catch (const unsupported_error& e) {                                                     \
		ErrorMessageBox(e.what(), "Spring: Hardware Problem: ", MBF_OK | MBF_CRASH);         \
	}                                                                                        \
	catch (const network_error& e) {                                                     \
		ErrorMessageBox(e.what(), "Spring: Network Error: ", MBF_OK | MBF_EXCL);         \
	}

/**
 * Spring's exception handler additions to BASE for non DEBUG builds.
 */
#define CATCH_SPRING_ERRORS_EXTENDED                                                        \
	catch (const boost::lock_error& e) {                                                    \
		std::ostringstream ss;                                                              \
		ss << e.native_error() << ": " << e.what();                                         \
		ErrorMessageBox(ss.str(), "Spring: Fatal Error (boost-lock)", MBF_OK | MBF_CRASH);  \
	}                                                                                       \
	catch (const boost::system::system_error& e) {                                          \
		std::ostringstream ss;                                                              \
		ss << e.code().value() << ": " << e.what();                                         \
		ErrorMessageBox(ss.str(), "Spring: Fatal Error (boost)", MBF_OK | MBF_CRASH);       \
	}                                                                                       \
	catch (const std::bad_alloc& e) {                                                       \
		ErrorMessageBox(e.what(), "Spring: Fatal Error (out of memory)", MBF_OK | MBF_CRASH);     \
	}                                                                                       \
	catch (const std::exception& e) {                                                       \
		ErrorMessageBox(e.what(), "Spring: Fatal Error (general)", MBF_OK | MBF_CRASH);     \
	}                                                                                       \
	catch (const char* e) {                                                                 \
		ErrorMessageBox(e, "Spring: Fatal Error (legacy)", MBF_OK | MBF_CRASH);             \
	}

/**
 * Spring's common error handler.
 */
#ifdef NO_CATCH_EXCEPTIONS
	#define CATCH_SPRING_ERRORS \
		CATCH_SPRING_ERRORS_BASE
#else
	#define CATCH_SPRING_ERRORS \
		CATCH_SPRING_ERRORS_BASE \
		CATCH_SPRING_ERRORS_EXTENDED
#endif

#endif // _ERROR_HANDLER_H
