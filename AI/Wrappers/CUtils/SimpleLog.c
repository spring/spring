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

#include <stdio.h>	// for file IO
#include <stdlib.h>	// calloc(), exit()
#include <string.h>	// strlen(), strcpy()
#include <time.h>	// for fetching current time
#include <stdarg.h>	// var-arg support

#define SIMPLELOG_OUTPUTBUFFER_SIZE 2048

static const char* logFileName = NULL;
static bool useTimeStamps;
static int logLevel;

static char* simpleLog_createTimeStamp() {

	time_t now;
	now = time(&now);
	struct tm* myTime = localtime(&now);
	unsigned int maxTimeStampSize = 32;
	char* timeStamp = (char*) calloc(maxTimeStampSize + 1, sizeof(char));
	strftime(timeStamp, maxTimeStampSize, "%c", myTime);

	return timeStamp;
}

static void simpleLog_out(int level, const char* msg) {

	if (level > logLevel) {
		return;
	}

	static char outBuffer[SIMPLELOG_OUTPUTBUFFER_SIZE];

	// format the message
	const char* logLevel_str = simpleLog_levelToStr(level);
	if (useTimeStamps) {
		char* timeStamp = simpleLog_createTimeStamp();
		SNPRINTF(outBuffer, SIMPLELOG_OUTPUTBUFFER_SIZE, "%s / %s(%i): %s\n",
				timeStamp, logLevel_str, level, msg);
		free(timeStamp);
		timeStamp = NULL;
	} else {
		SNPRINTF(outBuffer, SIMPLELOG_OUTPUTBUFFER_SIZE, "%s(%i): %s\n",
				logLevel_str, level, msg);
	}

	// try to open the log file
	FILE* file = NULL;
	if (logFileName != NULL) {
		file = FOPEN(logFileName, "a");
	}

	// print the message
	if (file != NULL) {
		FPRINTF(file, "%s", outBuffer);
		fclose(file);
		file = NULL;
	} else {
		// fallback method: write to stdout
		if (level > SIMPLELOG_LEVEL_WARNING || level < 0) {
			FPRINTF(stdout, "%s", outBuffer);
		} else {
			FPRINTF(stderr, "%s", outBuffer);
		}
	}
}

static void simpleLog_logv(int level, const char* fmt, va_list argp) {

	if (level > logLevel) {
		return;
	}

	static char outBuffer[SIMPLELOG_OUTPUTBUFFER_SIZE];

	VSNPRINTF(outBuffer, SIMPLELOG_OUTPUTBUFFER_SIZE, fmt, argp);
	simpleLog_out(level, outBuffer);
}

void simpleLog_init(const char* _logFileName, bool _useTimeStamps,
		int _logLevel, bool append) {

	if (_logFileName != NULL) {
		// NOTE: this causes a memory leack, as it is never freed.
		// but it is used till the end of the applications runtime anyway
		// -> no problem
		logFileName = util_allocStrCpy(_logFileName);
	
		// delete the logFile, and try writing to it
		FILE* file = NULL;
		if (logFileName != NULL) {
			if (append) {
				file = FOPEN(logFileName, "a");
			} else {
				file = FOPEN(logFileName, "w");
			}
		}
		if (file != NULL) {
			// make the file empty
			FPRINTF(file, "%s", "");
			fclose(file);
			file = NULL;
		} else {
			// report the error to stderr
			FPRINTF(stderr, "Failed writing to the log file \"%s\".\n%s",
					logFileName, "We will continue logging to stdout.");
		}

		// make sure the dir of the log file exists
		char* logFileDir = util_allocStrCpy(logFileName);
		if (!util_getParentDir(logFileDir)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to evaluate the parent dir of the config file: %s",
					logFileName);
		} else if (!util_makeDir(logFileDir, true)) {
			simpleLog_logL(SIMPLELOG_LEVEL_ERROR,
					"Failed to create the parent dir of the config file: %s",
					logFileDir);
		}
		free(logFileDir);
	} else {
		simpleLog_logL(-1, "No log file name supplied -> logging to stdout and stderr",
			useTimeStamps ? "yes" : "no", logLevel);
		logFileName = NULL;
	}

	useTimeStamps = _useTimeStamps;
	logLevel = _logLevel;

	simpleLog_logL(-1, "[logging started (time-stamps: %s / logLevel: %i)]",
			useTimeStamps ? "yes" : "no", logLevel);
}

void simpleLog_logL(int level, const char* fmt, ...) {

	if (level > logLevel) {
		return;
	}

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(level, fmt, argp);
	va_end(argp);
}

void simpleLog_log(const char* fmt, ...) {

	static const int level = SIMPLELOG_LEVEL_NORMAL;

	if (level > logLevel) {
		return;
	}

	va_list argp;

	va_start(argp, fmt);
	simpleLog_logv(level, fmt, argp);
	va_end(argp);
}

const char* simpleLog_levelToStr(int logLevel) {

	switch (logLevel) {
		case SIMPLELOG_LEVEL_ERROR:
			return "ERROR";
		case SIMPLELOG_LEVEL_WARNING:
			return "WARNING";
		case SIMPLELOG_LEVEL_NORMAL:
			return "NORMAL";
		case SIMPLELOG_LEVEL_FINE:
			return "FINE";
		case SIMPLELOG_LEVEL_FINER:
			return "FINER";
		case SIMPLELOG_LEVEL_FINEST:
			return "FINEST";
		default:
			return "CUSTOM";
	}
}
