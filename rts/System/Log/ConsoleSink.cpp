/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to the console (stdout & stderr).
 */

#include "Level.h" // for LOG_LEVEL_*
#include "LogUtil.h"

#include <cstdio>
#include <cstdarg>


#ifdef __cplusplus
extern "C" {
#endif

static const int SECTION_SIZE_MIN = 10;
static const int SECTION_SIZE_MAX = 20;

static void log_createPrefix_xorgStyle(char* prefix, size_t prefixSize,
		const char* section, int level)
{
	section = log_prepareSection(section);

	const char levelChar = log_levelToChar(level);

	snprintf(prefix, prefixSize, "(%c%c) %*.*s - ", levelChar, levelChar,
			SECTION_SIZE_MIN, SECTION_SIZE_MAX, section);
}

static void log_createPrefix_classical(char* prefix, size_t prefixSize,
		const char* section, int level)
{
	section = log_prepareSection(section);

	const char* levelStr = log_levelToString(level);

	snprintf(prefix, prefixSize, "%s %s: ", levelStr, section);
}

static inline void log_createPrefix(char* prefix, size_t prefixSize,
		const char* section, int level)
{
	log_createPrefix_xorgStyle(prefix, prefixSize, section, level);
	//log_createPrefix_classical(prefix, prefixSize, section, level);
}

/// Choose the out-stream for logging
static inline FILE* log_chooseStream(int level) {

	// - stdout: levels less severe then WARNING
	// - stderr: WARNING and more severe
	FILE* outStream = stdout;
	if (level >= LOG_LEVEL_WARNING) {
		outStream = stderr;
	}

	return outStream;
}


/**
 * ILog.h sink implementation.
 * @group logging_sink_console
 * @{
 */

/// Records a log entry
void log_sink_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	char prefix[64];
	log_createPrefix(prefix, sizeof(prefix), section, level);

	char message[1024];
	vsnprintf(message, sizeof(message), fmt, arguments);

	FILE* outStream = log_chooseStream(level);
	fprintf(outStream, "%s%s\n", prefix, message);
}

/** @} */ // group logging_sink_console

#ifdef __cplusplus
} // extern "C"
#endif

