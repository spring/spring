/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to the console (stdout & stderr).
 */

#include "Backend.h"
#include "FramePrefixer.h"
#include "Level.h" // for LOG_LEVEL_*
#include "System/maindefines.h"

#include <cstdio>


#ifdef __cplusplus
extern "C" {
#endif

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
 * @name logging_sink_console
 * ILog.h sink implementation.
 */
///@{

/// Records a log entry
static void log_sink_record_console(const char* section, int level,
		const char* record)
{
	char framePrefix[128] = {'\0'};
	log_framePrefixer_createPrefix(framePrefix, sizeof(framePrefix));

	FILE* outStream = log_chooseStream(level);
	if (level >= LOG_LEVEL_ERROR) {
		FPRINTF(outStream, "\033[90m%s\033[31m%s\033[39m\n", framePrefix, record);
	} else if (level >= LOG_LEVEL_WARNING) {
		FPRINTF(outStream, "\033[90m%s\033[33m%s\033[39m\n", framePrefix, record);
	} else {
		FPRINTF(outStream, "\033[90m%s\033[39m%s\n", framePrefix, record);
	}
	// *printf does not always flush after a newline
	// (eg. if stdout is being redirected to a file)
	fflush(outStream);
}

///@}


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct ConsoleSinkRegistrator {
		ConsoleSinkRegistrator() {
			log_backend_registerSink(&log_sink_record_console);
		}
	} consoleSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

