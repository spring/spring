/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#ifdef	__cplusplus
extern "C" {
#endif


#if       defined WIN32
#ifndef _TIMEZONE_DEFINED
#define _TIMEZONE_DEFINED
struct timezone {
	int  tz_minuteswest; ///< minutes W of Greenwich
	int  tz_dsttime;     ///< type of dst correction
};
#endif
/// @link http://linux.die.net/man/2/gettimeofday
int gettimeofday(struct timeval* tv, struct timezone* tz);
#else  // defined WIN32
// gettimeofday() is POSIX standard -> already declared
#endif // defined WIN32


/**
 * Returns the current time in milli-seconds.
 * On POSIX, this uses gettimeofday(),
 * on Windows, GetSystemTimeAsFileTime(),
 * which is what Boost uses when HIGH_PRECISION_CLOCK is defined.
 * This function does not depend on any libraries (like Boost or SDL).
 */
unsigned long timeUtil_getCurrentTimeMillis();


#ifdef __cplusplus
} // extern "C"
#endif

#endif // TIME_UTIL_H
