/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_SECTION_H
#define LOG_SECTION_H

/*
 * see ILog.h for documentation
 */

#define LOG_SECTION_DEFAULT NULL

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
