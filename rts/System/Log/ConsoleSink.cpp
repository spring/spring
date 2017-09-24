/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to the console (stdout & stderr).
 */

#include "Backend.h"
#include "FramePrefixer.h"
#include "Level.h" // for LOG_LEVEL_*
#include "System/MainDefines.h"

#include <cstdio>


#ifdef __cplusplus
extern "C" {
#endif

static bool colorizedOutput = false;



/**
 * @name logging_sink_console
 * ILog.h sink implementation.
 */
///@{


void log_console_colorizedOutput(bool enable) {
	colorizedOutput = enable;
}


/// Records a log entry
static void log_sink_record_console(int level, const char* section, const char* record)
{
	char framePrefix[128] = {'\0'};
	log_framePrefixer_createPrefix(framePrefix, sizeof(framePrefix));

	FILE* outStream = (level >= LOG_LEVEL_WARNING)? stderr: stdout;

	const char* fstr = "%s%s\n";
	if (colorizedOutput) {
		if (level >= LOG_LEVEL_ERROR) {
			fstr = "\033[90m%s\033[31m%s\033[39m\n";
		} else if (level >= LOG_LEVEL_WARNING) {
			fstr = "\033[90m%s\033[33m%s\033[39m\n";
		} else {
			fstr = "\033[90m%s\033[39m%s\n";
		}
	}

	FPRINTF(outStream, fstr, framePrefix, record);

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
		~ConsoleSinkRegistrator() {
			log_backend_unregisterSink(&log_sink_record_console);
		}
	} consoleSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

