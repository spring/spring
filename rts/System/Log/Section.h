/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_SECTION_H
#define LOG_SECTION_H

/*
 * see ILog.h for documentation
 */

#define LOG_SECTION_DEFAULT ""


/**
 * As all sections are required to be defined as compile-time constants,
 * we may use an address comparison, if the compiler merges constants.
 * MinGW does not merge constants, therefore we have to use !defined(_WIN32).
 */
#if       defined(__GNUC__) && !defined(_WIN32) && !defined(DEBUG)
	/**
	 * GCC does merged constants on -O2+:
	 * -fmerge-constants
	 *   Attempt to merge identical constants (string constants and floating
	 *   point constants) across compilation units. 
	 *   This option is the default for optimized compilation if the assembler
	 *   and linker support it. Use -fno-merge-constants to inhibit this
	 *   behavior. 
	 *   Enabled at levels -O, -O2, -O3, -Os.
	 */
	#define LOG_SECTION_EQUAL(section1, section2) \
		(section1 == section2)
	#define LOG_SECTION_COMPARE(section1, section2) \
		(section1 < section2)
#else  // defined(__GNUC__) && !defined(_WIN32) && !defined(DEBUG)
	#include <string.h>
	#define LOG_SECTION_EQUAL(section1, section2) \
		(((void*)section1 == (void*)section2) \
		|| (((void*)section1 != NULL) && ((void*)section2 != NULL) \
			&& (strcmp(section1, section2) == 0)))
	#define LOG_SECTION_COMPARE(section1, section2) \
		(((void*)section1 == NULL) \
			|| (((void*)section2 != NULL) && (strcmp(section1, section2) > 0)))
#endif // defined(__GNUC__) && !defined(_WIN32) && !defined(DEBUG)

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
