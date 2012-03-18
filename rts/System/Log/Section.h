/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_SECTION_H
#define LOG_SECTION_H

#include <string.h>

/*
 * see ILog.h for documentation
 */

#define LOG_SECTION_DEFAULT ""


/**
 * Helpers
 */
// first do a fasth-path and check if both c-strings share the same memory location
// if not check if both c-strings have the same content
#define LOG_SECTION_EQUAL(section1, section2) \
	(((void*)section1 == (void*)section2) \
	|| (((void*)section1 != NULL) && ((void*)section2 != NULL) \
		&& (strcmp(section1, section2) == 0)))

#define LOG_SECTION_COMPARE(section1, section2) \
	(((void*)section1 == NULL) \
		|| (((void*)section2 != NULL) && (strcmp(section1, section2) > 0)))

#define LOG_SECTION_IS_DEFAULT(section) \
	LOG_SECTION_EQUAL(section, LOG_SECTION_DEFAULT)


/**
 * Initialize the current log section to the default.
 * You might set a custom section by defining LOG_SECTION_CURRENT before
 * including this header, or by redefining LOG_SECTION_CURRENT
 * (@code #undef @endcode & @code #define @endcode).
 * Note: You may only use compile time strings (unique reference pointer)
 *       as section. reason:
 *       - allows for much faster and simpler comparison
 *       - allows for simplification in other matters, for example a registry of
 *         all sections
 */
#ifndef LOG_SECTION_CURRENT
	#define LOG_SECTION_CURRENT LOG_SECTION_DEFAULT
#endif

#endif // LOG_SECTION_H
