/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * see ILog.h for documentation
 */

#ifndef LOG_SECTION_H
#define LOG_SECTION_H

// logging should be C, use string.h
#include <string.h>

#define LOG_SECTION_DEFAULT ""


/**
 * Helpers
 */
//FIXME move all 3 funcs into a .c to not enforce including string.h?

// first check if both c-strings share the same memory location
// if not check if both c-strings have the same content
static inline int LOG_SECTION_EQUAL(const char* s1, const char* s2) {
	if (s1 == s2)
		return 1;
	if (s1 == NULL || s2 == NULL)
		return 0;

	return (strcmp(s1, s2) == 0);
}
static inline int LOG_SECTION_COMPARE_LESS(const char* s1, const char* s2) {
	if (s1 == NULL)
		return 1;
	if (s2 == NULL)
		return 1;

	return (strcmp(s1, s2) < 0);
}
static inline int LOG_SECTION_IS_DEFAULT(const char* s) {
	return (LOG_SECTION_EQUAL(s, LOG_SECTION_DEFAULT));
}


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
