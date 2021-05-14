/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeUtil.h"

#include "System/StringUtil.h"
#include "System/Exceptions.h"

#include <string>
#include <ctime>
#include <functional>

std::string CTimeUtil::GetCurrentTimeStr(bool utc)
{
	struct tm lt;
	// Get time as long integer
	__time64_t long_time = GetCurrentTime();

	// Convert to UTC or local time
#ifdef _WIN32
	static decltype(_localtime64_s)* convFunc[] = { &_localtime64_s, &_gmtime64_s };
	convFunc[utc](&lt, &long_time);
#else
	static decltype(localtime_r)* convFunc[] = { &localtime_r, &gmtime_r };
	convFunc[utc](&long_time, &lt);
#endif

	// Don't see how this can happen (according to docs _localtime64 only returns
	// nullptr if long_time is before 1/1/1970...) but a user's stacktrace indicated
	// nullptr newtime in the snprintf line...
	if (&lt == nullptr) {
		throw content_error("error: _localtime64 returned NULL");
	}

	const size_t str_maxSize = 512;
	char str[str_maxSize];
	SNPRINTF(str, str_maxSize, "%04i%02i%02i_%02i%02i%02i",
		lt.tm_year + 1900,
		lt.tm_mon + 1,
		lt.tm_mday,
		lt.tm_hour,
		lt.tm_min,
		lt.tm_sec);

	return str;
}
