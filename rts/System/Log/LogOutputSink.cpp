/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to logOutput.
 */

#include "Backend.h"
#include "System/LogOutput.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * ILog.h sink implementation.
 * @group logging_sink_logOutput
 * @{
 */

/// Records a log entry
static void log_sink_record_logOutput(const char* section, int level,
		const char* record)
{
	logOutput.Print("%s", record);
}

/** @} */ // group logging_sink_logOutput


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct SinkRegistrator {
		SinkRegistrator() {
			log_backend_registerSink(&log_sink_record_logOutput);
		}
	} sinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

