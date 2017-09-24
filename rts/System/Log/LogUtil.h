/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_UTIL_H
#define LOG_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

const char* log_util_levelToString(int level);

char log_util_levelToChar(int level);

int log_util_getNearestLevel(int level);

/**
 * Ensure we have a non-NULL section.
 */
const char* log_util_prepareSection(const char* section);


struct log_record_t {
	char msg[32768];

	const char* sec;
	const char* fmt;

	int lvl;
	int cnt; // number of repeats
};


#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_UTIL_H

