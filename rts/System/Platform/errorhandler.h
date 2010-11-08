/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief error messages
 * Platform specific error handling
 */

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <string>
#include <boost/system/system_error.hpp>
#include "Exceptions.h"

#define MBF_OK    1
#define MBF_INFO  2
#define MBF_EXCL  4
#define MBF_MAIN  8

#if defined __GNUC__ && (__GNUC__ >= 4)
	#define NO_RETURN_POSTFIX __attribute__ ((noreturn))
#else
	#define NO_RETURN_POSTFIX
#endif

#if defined _MSC_VER
	#define NO_RETURN_PREFIX __declspec(noreturn)
#else
	#define NO_RETURN_PREFIX
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
NO_RETURN_PREFIX void ErrorMessageBox(const std::string msg, const std::string caption, unsigned int flags = MBF_OK) NO_RETURN_POSTFIX;


/**
 * Spring's common error handler.
 */
#ifdef NO_CATCH_EXCEPTIONS
	#define CATCH_SPRING_ERRORS \
		catch (const content_error& e) {                                                        \
			ErrorMessageBox(e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);     \
		}                                                                                       \
		catch (const opengl_error& e) {                                                         \
			ErrorMessageBox(e.what(), "OpenGL content:", MBF_OK | MBF_EXCL);                \
		}
#else
	#define CATCH_SPRING_ERRORS \
		catch (const content_error& e) {                                                        \
			ErrorMessageBox(e.what(), "Incorrect/Missing content:", MBF_OK | MBF_EXCL);     \
		}                                                                                       \
		catch (const opengl_error& e) {                                                         \
			ErrorMessageBox(e.what(), "OpenGL content:", MBF_OK | MBF_EXCL);                \
		}                                                                                       \
		catch (const boost::system::system_error& e) {                                          \
			std::stringstream ss;                                                           \
			ss << e.code().value() << ": " << e.what();                                     \
			std::string tmp = ss.str();                                                     \
			ErrorMessageBox(tmp, "Fatal Error", MBF_OK | MBF_EXCL);                         \
		}                                                                                       \
		catch (const std::exception& e) {                                                       \
			ErrorMessageBox(e.what(), "Fatal Error", MBF_OK | MBF_EXCL);                    \
		}                                                                                       \
		catch (const char* e) {                                                                 \
			ErrorMessageBox(e, "Fatal Error", MBF_OK | MBF_EXCL);                           \
		}
#endif

#endif // ERRORHANDLER_H
