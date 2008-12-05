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

#include "Log.h"

#include "InterfaceUtil.h"
#include "InterfaceDefines.h"

#include <stdio.h>	// for file IO
#include <stdlib.h>	// calloc(), exit()
#include <string.h>	// strlen(), strcpy()
#include <time.h>	// for fetching current time
#include <stdarg.h>	// var-arg support

static const unsigned int bufferSize = 2048;

const char* myLogFileName = NULL;
bool useTimeStamps;
bool isFineLog;

void simpleLog_init(const char* _logFileName, bool _useTimeStamps,
		bool _isFineLog) {

	// NOTE: causeing a memory leack, as it is never freed.
	// but it is used till the end of the applications runtime anyway
	// -> no problem
	char* logFileName = (char*) calloc(strlen(_logFileName) + 1, sizeof (char));
	STRCPY(logFileName, _logFileName);
	myLogFileName = logFileName;

	useTimeStamps = _useTimeStamps;
	isFineLog = _isFineLog;

	simpleLog_log("\n\n[logging started]");
}

char* simpleLog_createTimeStamp() {

	time_t now;
	now = time(&now);
	struct tm* myTime = localtime(&now);
	unsigned int maxTimeStampSize = 32;
	char* timeStamp = (char*) calloc(maxTimeStampSize + 1, sizeof (char));
	strftime(timeStamp, maxTimeStampSize, "%c", myTime);

	return timeStamp;
}

void simpleLog_out(const char* msg) {

	if (myLogFileName != NULL) {
		FILE* file = FOPEN(myLogFileName, "a");
		if (useTimeStamps) {
			char* timeStamp = simpleLog_createTimeStamp();
			FPRINTF(file, "%s: %s\n", timeStamp, msg);
			free(timeStamp);
		} else {
			FPRINTF(file, "%s\n", msg);
		}
		fclose(file);
	} else {
		// fallback method: write to stdout
		PRINTF(msg);
	}
}

void simpleLog_logv(const char* fmt, va_list argp) {
	
	char text[bufferSize];

	VSNPRINTF(text, sizeof(text), fmt, argp);
	simpleLog_out(text);
}

void simpleLog_log(const char* fmt, ...) {

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(fmt, argp);
	va_end(argp);
}

void simpleLog_fine(const char* fmt, ...) {

	if (isFineLog) {
		va_list argp;

		va_start(argp, fmt);
		simpleLog_logv(fmt, argp);
		va_end(argp);
	}
}

void simpleLog_error(int error, const char* fmt, ...) {

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(fmt, argp);
	va_end(argp);

	exit(error);
}
