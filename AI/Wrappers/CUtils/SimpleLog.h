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

#include "System/Log/Level.h"
#include "System/ExportDefines.h"

#include <stdbool.h> /* bool, true, false */

typedef void (CALLING_CONV *logfunction)(int id, const char* section, int level, const char* msg);

void simpleLog_initcallback(int, const char* section, logfunction func, int loglevel);

/**
 * Logs a text message,
 * but only if level <= logLevel (see simpleLog_init()).
 * Works like printf(fmt, ...).
 * @param level see enum SimpleLog_Level
 * @param fmt   format of the logmessage, see man 3 printf
 */
void simpleLog_logL(int level, const char* fmt, ...);

/**
 * Logs a text message with SIMPLELOG_LEVEL_NORMAL.
 * Works like printf(fmt, ...).
 */
void simpleLog_log(const char* fmt, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _LOG_H */
