/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LogUtil.h"

#include "Level.h"
#include <stddef.h> // for NULL

const char* log_util_levelToString(int level) {

	switch (level) {
		case LOG_LEVEL_DEBUG:   return "Debug";
		case LOG_LEVEL_INFO:    return "Info";
		case LOG_LEVEL_WARNING: return "Warning";
		case LOG_LEVEL_ERROR:   return "Error";
		case LOG_LEVEL_FATAL:   return "Fatal";
		default:                return "<unknown>";
	}
}

char log_util_levelToChar(int level) {

	switch (level) {
		case LOG_LEVEL_DEBUG:   return 'D';
		case LOG_LEVEL_INFO:    return 'I';
		case LOG_LEVEL_WARNING: return 'W';
		case LOG_LEVEL_ERROR:   return 'E';
		case LOG_LEVEL_FATAL:   return 'F';
		default:                return '?';
	}
}

const char* log_util_prepareSection(const char* section) {

	// make sure we always have a string for printing
	if (section == NULL) {
		section = "<default>";
	}

	return section;
}

