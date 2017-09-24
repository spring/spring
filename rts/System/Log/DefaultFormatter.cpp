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
#include "System/MainDefines.h"
#include "System/SafeCStrings.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define va_copy(dst, src) ((dst) = (src))
#endif


static inline bool printf_append(log_record_t* log, va_list arguments)
{
	const size_t bufferPos = strlen(log->msg);
	const size_t bufferSize = sizeof(log->msg);

	if (bufferPos >= (bufferSize - 1))
		return false;

	// printf will move the internal pointer of va_list so we need to make a copy
	va_list arguments_;
	va_copy(arguments_, arguments); 
	const int writtenChars = VSNPRINTF(&log->msg[bufferPos], bufferSize - bufferPos, log->fmt, arguments_);
	va_end(arguments_);

	// since writtenChars excludes the null terminator (if any was written),
	// writtenChars >= freeBufferSize always means the buffer was too small
	// NOTE: earlier glibc versions and MSVC will return -1 in this case
	return (size_t(writtenChars) < (bufferSize - bufferPos));
}


// *******************************************************************************************

static void log_formatter_createPrefix(log_record_t* log) {
	char* bufEndPtr = &log->msg[0];

	if (!LOG_SECTION_IS_DEFAULT(log->sec)) {
		bufEndPtr = STRCAT_T(bufEndPtr, sizeof(log->msg), "[");
		bufEndPtr = STRCAT_T(bufEndPtr, sizeof(log->msg), log_util_prepareSection(log->sec));
		bufEndPtr = STRCAT_T(bufEndPtr, sizeof(log->msg), "] ");
	}
	if (log->lvl != LOG_LEVEL_INFO && log->lvl != LOG_LEVEL_NOTICE) {
		bufEndPtr = STRCAT_T(bufEndPtr, sizeof(log->msg), log_util_levelToString(log->lvl));
		bufEndPtr = STRCAT_T(bufEndPtr, sizeof(log->msg), ": ");
	}
}


// *******************************************************************************************

/**
 * @name logging_formatter
 * ILog.h formatter implementation.
 */
///@{

/**
 * Formats a log entry into its final string form.
 */
void log_formatter_format(log_record_t* log, va_list arguments)
{
	memset(&log->msg[0], 0, sizeof(log->msg));

	log_formatter_createPrefix(log);
	printf_append(log, arguments);
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

