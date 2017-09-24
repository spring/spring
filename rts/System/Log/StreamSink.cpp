/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StreamSink.h"
#include "Backend.h"

#include <ostream>
#include <string>
#include <cstring>


static std::ostream* logStreamInt = NULL;

void log_sink_stream_setLogStream(std::ostream* logStream) {
	logStreamInt = logStream;
}


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name logging_sink_stream
 * ILog.h sink implementation.
 */
///@{

/// Records a log entry
void log_sink_record_stream(int level, const char* section, const char* record)
{
	if (logStreamInt != NULL) {
		logStreamInt->write(record, strlen(record));
		(*logStreamInt) << std::endl;
	}
}

///@}


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct StreamSinkRegistrator {
		StreamSinkRegistrator() {
			log_backend_registerSink(&log_sink_record_stream);
		}
		~StreamSinkRegistrator() {
			log_backend_unregisterSink(&log_sink_record_stream);
		}
	} streamSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

