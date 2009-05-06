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
