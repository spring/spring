/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Backend.h"

#include <cstdio>
#include <cstdarg>
#include <vector>


#ifdef __cplusplus
extern "C" {
#endif

extern void log_formatter_format(char* record, size_t recordSize,
		const char* section, int level, const char* fmt, va_list arguments);

namespace {
	std::vector<log_sink_ptr>* sinks;

	struct SinkVectorInitializer {
		SinkVectorInitializer() {
			sinks = new std::vector<log_sink_ptr>();
		}
		~SinkVectorInitializer() {
			std::vector<log_sink_ptr>* tmpSinks = sinks;
			sinks = NULL;
			tmpSinks->clear();
			delete tmpSinks;
		}
	};
}


void log_backend_registerSink(log_sink_ptr sink) {

	// NOTE This is required for the vector to be initialized
	//      when used before main() was called.
	static const SinkVectorInitializer sinkVectorInitializer;

	if (sinks != NULL) { // might be cleaned-up already
		sinks->push_back(sink);
	}
}

void log_backend_unregisterSink(log_sink_ptr sink) {

	if (sinks != NULL) { // might be cleaned-up already
		std::vector<log_sink_ptr>::iterator si;
		for (si = sinks->begin(); si != sinks->end(); ++si) {
			if (*si == sink) {
				sinks->erase(si);
			}
		}
	}
}

/**
 * ILog.h backend implementation.
 * @group logging_backend
 * @{
 */

/// Eventually formats and routes the record to all sinks
void log_backend_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	if ((sinks != NULL) && !sinks->empty()) {
		char record[1024 + 64];
		log_formatter_format(record, sizeof(record), section, level, fmt,
				arguments);

		std::vector<log_sink_ptr>::iterator si;
		for (si = sinks->begin(); si != sinks->end(); ++si) {
			(*si)(section, level, record);
		}
	}
}

/** @} */ // group logging_backend

#ifdef __cplusplus
} // extern "C"
#endif

