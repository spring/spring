/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all log records to the MSVC debug system.
 * Does nothing on compilers != MSVC.
 */

#ifdef _MSC_VER

#include "Backend.h"
#include "FramePrefixer.h"

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name logging_sink_outputDebugString
 * ILog.h sink implementation for the MSVC debug system.
 */
///@{

/// Records a log entry
static void log_sink_record_outputDebugString(int level, const char* section, const char* record)
{
	char framePrefix[128] = {'\0'};
	log_framePrefixer_createPrefix(framePrefix, sizeof(framePrefix));

	OutputDebugString(framePrefix);
	OutputDebugString(record);
	OutputDebugString("\n");
}

///@}


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct OutputDebugStringSinkRegistrator {
		OutputDebugStringSinkRegistrator() {
			log_backend_registerSink(&log_sink_record_outputDebugString);
		}
		~OutputDebugStringSinkRegistrator() {
			log_backend_unregisterSink(&log_sink_record_outputDebugString);
		}
	} outputDebugStringSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _MSC_VER

