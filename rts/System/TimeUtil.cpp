/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeUtil.h"

#include "System/StringUtil.h"
#include "System/Exceptions.h"

#include <string>
#include <ctime>
#include <functional>

#include "lib/fmt/format.h"
#include "lib/fmt/printf.h"

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

	return fmt::sprintf("%04i%02i%02i_%02i%02i%02i",
		lt.tm_year + 1900,
		lt.tm_mon + 1,
		lt.tm_mday,
		lt.tm_hour,
		lt.tm_min,
		lt.tm_sec
	);
}
