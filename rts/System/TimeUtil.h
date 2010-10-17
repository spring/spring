/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include <string>
#include <ctime>

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

#endif // TIME_UTIL_H
