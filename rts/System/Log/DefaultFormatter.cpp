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

#include <cstdio>
#include <cstdarg>


#ifdef __cplusplus
extern "C" {
#endif

static const int SECTION_SIZE_MIN = 10;
static const int SECTION_SIZE_MAX = 20;

static void log_formatter_createPrefix_xorgStyle(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	section = log_prepareSection(section);

	const char levelChar = log_levelToChar(level);

	snprintf(prefix, prefixSize, "(%c%c) %*.*s - ", levelChar, levelChar,
			SECTION_SIZE_MIN, SECTION_SIZE_MAX, section);
}

static void log_formatter_createPrefix_testing(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	section = log_prepareSection(section);

	const char* levelStr = log_levelToString(level);

	snprintf(prefix, prefixSize, "%s %s: ", levelStr, section);
}

static void log_formatter_createPrefix_default(char* prefix,
		size_t prefixSize, const char* section, int level)
{
	prefix[0] = '\0';

	// HACK this stuff should be done later, closer to the point where it is written to a file or the console
	if (section != LOG_SECTION_DEFAULT) {
		section = log_prepareSection(section);
		snprintf(prefix, prefixSize, "%s[%s] ", prefix, section);
	}
	if (level != LOG_LEVEL_INFO) {
		const char* levelStr = log_levelToString(level);
		snprintf(prefix, prefixSize, "%s%s: ", prefix, levelStr);
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
 * ILog.h formatter implementation.
 * @group logging_formatter
 * @{
 */

/// Formats a log entry into its final string form
void log_formatter_format(char* record, size_t recordSize,
		const char* section, int level, const char* fmt, va_list arguments)
{
	char prefix[64];
	log_formatter_createPrefix(prefix, sizeof(prefix), section, level);

	char message[1024];
	vsnprintf(message, sizeof(message), fmt, arguments);

	snprintf(record, recordSize, "%s%s", prefix, message);
}

/** @} */ // group logging_formatter

#ifdef __cplusplus
} // extern "C"
#endif

