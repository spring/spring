/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is the default formatter for the ILog.h logging API.
 * It formats the message and eventually adds a prefix.
 * The prefix will usually consist of the level, section and possibly a
 * timestamp.
 */

#include "LogUtil.h"
#include "Level.h"
#include "Section.h"
#include "System/maindefines.h"
#include "System/SafeCStrings.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>


#ifdef __cplusplus
extern "C" {
#endif

static const int SECTION_SIZE_MIN = 10;
static const int SECTION_SIZE_MAX = 20;

static int* frameNum = NULL;

void log_formatter_setFrameNumReference(int* frameNumReference)
{
	frameNum = frameNumReference;
}

static void log_formatter_createPrefix_xorgStyle(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	section = log_util_prepareSection(section);

	const char levelChar = log_util_levelToChar(level);

	SNPRINTF(prefix, prefixSize, "(%c%c) %*.*s - ", levelChar, levelChar,
			SECTION_SIZE_MIN, SECTION_SIZE_MAX, section);
}

static void log_formatter_createPrefix_testing(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	section = log_util_prepareSection(section);

	const char* levelStr = log_util_levelToString(level);

	SNPRINTF(prefix, prefixSize, "%s %s: ", levelStr, section);
}

static void log_formatter_createPrefix_default(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	prefix[0] = '\0';

	if (!LOG_SECTION_IS_DEFAULT(section)) {
		section = log_util_prepareSection(section);
		STRCAT_T(prefix, prefixSize, "[");
		STRCAT_T(prefix, prefixSize, section);
		STRCAT_T(prefix, prefixSize, "] ");
	}
	if (level != LOG_LEVEL_INFO) {
		const char* levelStr = log_util_levelToString(level);
		STRCAT_T(prefix, prefixSize, levelStr);
		STRCAT_T(prefix, prefixSize, ": ");
	}
}

static inline void log_formatter_createPrefix(char* prefix, size_t prefixSize,
		const char* section, int level)
{
	//log_formatter_createPrefix_xorgStyle(prefix, prefixSize, section, level);
	//log_formatter_createPrefix_testing(prefix, prefixSize, section, level);
	log_formatter_createPrefix_default(prefix, prefixSize, section, level);
}


/**
 * @name logging_formatter
 * ILog.h formatter implementation.
 */
///@{

char* log_formatter_getFrame_prefix()
{
	//FIXME using a static var may make it thread-unsafe!!!
	static char prefix[128] = {'\0'};
	if (frameNum != NULL) {
		SNPRINTF(prefix, sizeof(prefix), "[f=%07d] ", *frameNum);
	}
	return prefix;
}


/// Formats a log entry into its final string form
void log_formatter_format(char* record, size_t recordSize,
		const char* section, int level, const char* fmt, va_list arguments)
{
	char prefix[64];
	log_formatter_createPrefix(prefix, sizeof(prefix), section, level);

	char message[1024];
	VSNPRINTF(message, sizeof(message), fmt, arguments);

	SNPRINTF(record, recordSize, "%s%s", prefix, message);
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

