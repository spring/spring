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

#ifndef _LOG_H
#define _LOG_H

#ifdef __cplusplus
extern "C" {
#endif

enum SimpleLog_Level {
	SIMPLELOG_LEVEL_ERROR       = 1,
	SIMPLELOG_LEVEL_WARNING     = 3,
	SIMPLELOG_LEVEL_NORMAL      = 5,
	SIMPLELOG_LEVEL_FINE        = 8,
	SIMPLELOG_LEVEL_FINER       = 9,
	SIMPLELOG_LEVEL_FINEST      = 10
};

#include <stdbool.h> // bool, true, false

#define EXTERNAL_LOGGER(msg)   log(msg);

/**
 * Initializes the log.
 * @param  logLevel   see enum SimpleLog_Level
 * @param  append     if true, previous content of the file is preserved
 */
void simpleLog_init(const char* logFileName, bool useTimeStamps,
		int logLevel, bool append);

/**
 * Logs a text message,
 * but only if level <= logLevel (see simpleLog_init()).
 * Works like printf(fmt, ...).
 * @param  level   see enum SimpleLog_Level
 */
void simpleLog_logL(int level, const char* fmt, ...);

/**
 * Logs a text message with SIMPLELOG_LEVEL_NORMAL.
 * Works like printf(fmt, ...).
 */
void simpleLog_log(const char* fmt, ...);

/**
 * Returns a string representation of a log level.
 * @param  logLevel   see enum SimpleLog_Level
 */
const char* simpleLog_levelToStr(int logLevel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _LOG_H
