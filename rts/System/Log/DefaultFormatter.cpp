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

#ifdef _MSC_VER
#define va_copy(dst, src) ((dst) = (src))
#endif

static const int SECTION_SIZE_MIN = 10;
static const int SECTION_SIZE_MAX = 20;


// *******************************************************************************************
// Helpers
static inline void resize_buffer(log_record_t* log, const bool copy = false)
{
	char** buffer = &log->msg;
	char* old = *buffer;
	int* bufferSize = &log->len;

	*bufferSize <<= 2; // `2` to increase it faster
	*buffer = new char[*bufferSize];

	if (copy)
		memcpy(*buffer, old, (*bufferSize) >> 2);

	delete[] old;
}


static inline void printf_append(log_record_t* log, va_list arguments)
{
	// dynamically adjust the buffer size until VSNPRINTF returns fine
	size_t bufferPos = strlen(log->msg);

	do {
		const size_t freeBufferSize = log->len - bufferPos;
		char* bufAppendPos = &log->msg[bufferPos];

		// printf will move the internal pointer of va_list.
		// So we need to make a copy, if want to run it again.
		va_list arguments_;
		va_copy(arguments_, arguments); 
		const int writtenChars = VSNPRINTF(bufAppendPos, freeBufferSize, log->fmt, arguments_);
		va_end(arguments_);

		// since writtenChars excludes the null terminator (if any was written),
		// writtenChars >= freeBufferSize always means buffer was too small
		// NOTE: earlier glibc versions and MSVC will return -1 when buffer is too small
		// const bool bufferTooSmall = writtenChars >= freeBufferSize || writtenChars < 0;

		if (writtenChars >= 0 && writtenChars < freeBufferSize)
			break;

		resize_buffer(log, true);
	} while (true);
}


// *******************************************************************************************

static void log_formatter_createPrefix_default(log_record_t* log) {
	log->msg[0] = '\0';

	if (!LOG_SECTION_IS_DEFAULT(log->sec)) {
		const char* prepSection = log_util_prepareSection(log->sec);
		STRCAT_T(log->msg, log->len, "[");
		STRCAT_T(log->msg, log->len, prepSection);
		STRCAT_T(log->msg, log->len, "] ");
	}
	if (log->lvl != LOG_LEVEL_INFO && log->lvl != LOG_LEVEL_NOTICE) {
		const char* levelStr = log_util_levelToString(log->lvl);
		STRCAT_T(log->msg, log->len, levelStr);
		STRCAT_T(log->msg, log->len, ": ");
	}
}


static inline void log_formatter_createPrefix(log_record_t* log) {
	log_formatter_createPrefix_default(log);

	// check if the buffer was large enough, if not resize it and try again
	if ((strlen(log->msg) + 1) < log->len)
		return;

	resize_buffer(log);
	log_formatter_createPrefix(log);
}

// *******************************************************************************************

/**
 * @name logging_formatter
 * ILog.h formatter implementation.
 */
///@{

/**
 * Formats a log entry into its final string form.
 * @return a string buffer, allocated with new[] -> you have to delete[] it
 */
void log_formatter_format(log_record_t* log, va_list arguments)
{
	if (log->len == 0)
		log->msg = new char[log->len = 1024];

	memset(&log->msg[0], 0, log->len);

	log_formatter_createPrefix(log);
	printf_append(log, arguments);
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

