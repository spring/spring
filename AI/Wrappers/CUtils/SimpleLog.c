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

#include "SimpleLog.h"

#include "Util.h"

#include "System/MainDefines.h"
#include "System/SafeCStrings.h"

#include <stdio.h>	// for file IO
#include <stdlib.h>	// calloc(), exit()
#include <string.h>	// strlen(), strcpy()
#include <stdarg.h>	// var-arg support

#define SIMPLELOG_OUTPUTBUFFER_SIZE 2048

static int logLevel = LOG_LEVEL_INFO;
static logfunction logFunction = NULL;
static char logSection[32] = "";
static int interfaceid = 0;

static void simpleLog_out(int level, const char* msg) {

	if (level < logLevel) {
		return;
	}

	if (logFunction != NULL) {
		logFunction(interfaceid, logSection, level, msg);
		return;
	}

	// fallback method: write to stdout
	if (level < LOG_LEVEL_WARNING || level == LOG_LEVEL_NONE) {
		FPRINTF(stdout, "%s", msg);
	} else {
		FPRINTF(stderr, "%s", msg);
	}
}

static void simpleLog_logv(int level, const char* fmt, va_list argp) {

	if (level < logLevel) {
		return;
	}

	static char outBuffer[SIMPLELOG_OUTPUTBUFFER_SIZE];

	VSNPRINTF(outBuffer, SIMPLELOG_OUTPUTBUFFER_SIZE, fmt, argp);
	simpleLog_out(level, outBuffer);
}

void simpleLog_logL(int level, const char* fmt, ...) {

	if (level < logLevel) {
		return;
	}

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(level, fmt, argp);
	va_end(argp);
}

void simpleLog_log(const char* fmt, ...) {

	static const int level = LOG_LEVEL_INFO;

	if (level < logLevel) {
		return;
	}

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(level, fmt, argp);
	va_end(argp);
}

void simpleLog_initcallback(int id, const char* section, logfunction func, int _logLevel)
{
	strncpy(logSection, section, 32);
	logFunction = func;
	interfaceid = id;
	logLevel = _logLevel;
}

