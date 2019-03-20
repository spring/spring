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
void ErrorMessageBox(const char* msg, const char* caption, unsigned int flags = MBF_OK);


/**
 * Spring's common exception handler.
 */
#define CATCH_SPRING_ERRORS_BASE                                                             \
	catch (const content_error& e) {                                                         \
		ErrorMessageBox(e.what(), "Spring: caught content_error: ", MBF_OK | MBF_EXCL);      \
	}                                                                                        \
	catch (const opengl_error& e) {                                                          \
		ErrorMessageBox(e.what(), "Spring: caught opengl_error: ", MBF_OK | MBF_CRASH);      \
	}                                                                                        \
	catch (const user_error& e) {                                                            \
		ErrorMessageBox(e.what(), "Spring: caught user_error: ", MBF_OK | MBF_EXCL);         \
	}                                                                                        \
	catch (const unsupported_error& e) {                                                     \
		ErrorMessageBox(e.what(), "Spring: caught unsupported_error: ", MBF_OK | MBF_CRASH); \
	}                                                                                        \
	catch (const network_error& e) {                                                         \
		ErrorMessageBox(e.what(), "Spring: caught network_error: ", MBF_OK | MBF_EXCL);      \
	}

/**
 * Spring's exception handler additions to BASE for non DEBUG builds.
 */
#define CATCH_SPRING_ERRORS_EXTENDED                                                          \
	catch (const std::logic_error& e) {                                                       \
		ErrorMessageBox(e.what(), "Spring: caught std::logic_error", MBF_OK | MBF_CRASH);     \
	}                                                                                         \
	catch (const std::bad_alloc& e) {                                                         \
		ErrorMessageBox(e.what(), "Spring: caught std::bad_alloc", MBF_OK | MBF_CRASH);       \
	}                                                                                         \
	catch (const std::system_error& e) {                                                      \
		ErrorMessageBox(e.what(), "Spring: caught std::system_error", MBF_OK | MBF_CRASH);    \
	}                                                                                         \
	catch (const std::runtime_error& e) {                                                     \
		ErrorMessageBox(e.what(), "Spring: caught std::runtime_error", MBF_OK | MBF_CRASH);   \
	}                                                                                         \
	catch (const std::exception& e) {                                                         \
		ErrorMessageBox(e.what(), "Spring: caught std::exception", MBF_OK | MBF_CRASH);       \
	}                                                                                         \
	catch (const char* e) {                                                                   \
		ErrorMessageBox(e, "Spring: caught legacy exception", MBF_OK | MBF_CRASH);            \
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
