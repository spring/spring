/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StreamSink.h"
#include "Backend.h"

#include <ostream>
#include <string>
#include <cstring>


static std::ostream* logStreamInt;

void log_sink_stream_setLogStream(std::ostream* logStream) {
	logStreamInt = logStream;
}


#ifdef __cplusplus
extern "C" {
#endif

/**
 * ILog.h sink implementation.
 * @group logging_sink_stream
 * @{
 */

/// Records a log entry
void log_sink_record_stream(const char* section, int level, const char* record)
{
	if (logStreamInt != NULL) {
		logStreamInt->write(record, strlen(record));
		(*logStreamInt) << std::endl;
	}
}

/** @} */ // group logging_sink_stream


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct StreamSinkRegistrator {
		StreamSinkRegistrator() {
			logStreamInt = NULL;
			log_backend_registerSink(&log_sink_record_stream);
		}
	} streamSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

