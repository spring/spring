/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIMEUTIL_H
#define TIMEUTIL_H

#include "Util.h"
#include <string.h>
#include <time.h>

#ifdef __GNUC__
#define __time64_t time_t
#define _time64(x) time(x)
#define _localtime64(x) localtime(x)
#endif

class CTimeUtil
{
public:
	/**
	 * Returns the current time as a long integer
	 */
	static inline __time64_t GetCurrentTime()
	{
		__time64_t currTime;
		_time64(&currTime);
		return currTime;
	}

	/**
	 * Returns the current local time formatted as follows:
	 * "JJJJMMDD_HHmmSS", eg: "20091231_115959"
	 */
	static std::string GetCurrentTimeStr();
};

#endif // TIMEUTIL_H
