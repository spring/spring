/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TimeUtil.h"

#include <time.h>


#if       defined WIN32

/*
 * This emulates the POSIX gettimeofday(timeval, timezone) function on Windows.
 * @see http://linux.die.net/man/2/gettimeofday
 * @see http://www.suacommunity.com/dictionary/gettimeofday-entry.php
 */

#include <windows.h>
#include <iostream>

using namespace std;
 
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
	#include <winsock2.h>
#else
	#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

/// Definition of a gettimeofday function
int gettimeofday(struct timeval* tv, struct timezone* tz) {
	// Define a structure to receive the current Windows filetime
	FILETIME ft;

	if (NULL != tv) {
		// Initialize the present time to 0
		unsigned __int64 tmpres = 0;

		GetSystemTimeAsFileTime(&ft);

		// The GetSystemTimeAsFileTime returns the number of 100 nanosecond 
		// intervals since Jan 1, 1601 in a structure. Copy the high bits to 
		// the 64 bit tmpres, shift it left by 32 then or in the low 32 bits.
		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		// Convert to microseconds by dividing by 10
		tmpres /= 10;

		// The Unix epoch starts on Jan 1 1970.  Need to subtract the difference 
		// in seconds from Jan 1 1601.
		tmpres -= DELTA_EPOCH_IN_MICROSECS;

		// Finally change microseconds to seconds and place in the seconds value. 
		// The modulus picks up the microseconds.
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (tz != NULL) {
		// Initialize the timezone to UTC
		static int tzflag = 0;

		if (!tzflag) {
			_tzset();
			tzflag++;
		}

		// Adjust for the timezone west of Greenwich
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

#else  // defined WIN32
#include <sys/time.h>
#endif // defined WIN32


unsigned long timeUtil_getCurrentTimeMillis() {

	struct timeval timediff;
	gettimeofday(&timediff, NULL);
	const unsigned long milliSeconds = (timediff.tv_sec * 1000UL) + ((timediff.tv_usec + 500UL) / 1000UL);
	return milliSeconds;
}
