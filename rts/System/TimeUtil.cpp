/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TimeUtil.h"

#include "Exceptions.h"

std::string CTimeUtil::GetCurrentTimeStr()
{
	struct tm* lt;
	// Get time as long integer
	__time64_t long_time = GetCurrentTime();
	// Convert to local time
	lt = _localtime64(&long_time);

	// Don't see how this can happen (according to docs _localtime64 only returns
	// NULL if long_time is before 1/1/1970...) but a user's stacktrace indicated
	// NULL newtime in the snprintf line...
	if (lt == NULL) {
		throw content_error("error: _localtime64 returned NULL");
	}

	const size_t str_maxSize = 512;
	char str[str_maxSize];
	SNPRINTF(str, str_maxSize, "%04i%02i%02i_%02i%02i%02i",
		lt->tm_year + 1900,
		lt->tm_mon + 1,
		lt->tm_mday,
		lt->tm_hour,
		lt->tm_min,
		lt->tm_sec);

	return str;
}
