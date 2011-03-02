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
void ErrorMessageBox(const std::string& msg, const std::string& caption, unsigned int flags = MBF_OK);


/**
 * Spring's common error handler.
 */
#ifdef NO_CATCH_EXCEPTIONS
	#define CATCH_SPRING_ERRORS \
		catch (const content_error& e) {                                                        \
			ErrorMessageBox(e.what(), "Spring: Incorrect/Missing content: ", MBF_OK | MBF_EXCL);     \
		}                                                                                       \
		catch (const opengl_error& e) {                                                         \
			ErrorMessageBox(e.what(), "Spring: OpenGL content", MBF_OK | MBF_CRASH);        \
		}
#else
	#define CATCH_SPRING_ERRORS \
		catch (const content_error& e) {                                                        \
			ErrorMessageBox(e.what(), "Spring: Incorrect/Missing content: ", MBF_OK | MBF_EXCL);     \
		}                                                                                       \
		catch (const opengl_error& e) {                                                         \
			ErrorMessageBox(e.what(), "Spring: OpenGL content", MBF_OK | MBF_CRASH);        \
		}                                                                                       \
		catch (const boost::system::system_error& e) {                                          \
			std::ostringstream ss;                                                          \
			ss << e.code().value() << ": " << e.what();                                     \
			ErrorMessageBox(ss.str(), "Spring: Fatal Error", MBF_OK | MBF_CRASH);           \
		}                                                                                       \
		catch (const std::exception& e) {                                                       \
			ErrorMessageBox(e.what(), "Spring: Fatal Error", MBF_OK | MBF_CRASH);           \
		}                                                                                       \
		catch (const char* e) {                                                                 \
			ErrorMessageBox(e, "Spring: Fatal Error", MBF_OK | MBF_CRASH);                  \
		}
#endif

#endif // ERRORHANDLER_H
