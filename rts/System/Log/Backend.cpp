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
	std::vector<log_sink_ptr>& log_formatter_getSinks() {
		static std::vector<log_sink_ptr> sinks;
		return sinks;
	}
}


void log_backend_registerSink(log_sink_ptr sink) {
	log_formatter_getSinks().push_back(sink);
}

void log_backend_unregisterSink(log_sink_ptr sink) {

	std::vector<log_sink_ptr>& sinks = log_formatter_getSinks();
	std::vector<log_sink_ptr>::iterator si;
	for (si = sinks.begin(); si != sinks.end(); ++si) {
		if (*si == sink) {
			sinks.erase(si);
			break;
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
	const std::vector<log_sink_ptr>& sinks = log_formatter_getSinks();
	if (!sinks.empty()) {
		char record[1024 + 64];
		log_formatter_format(record, sizeof(record), section, level, fmt,
				arguments);

		std::vector<log_sink_ptr>::const_iterator si;
		for (si = sinks.begin(); si != sinks.end(); ++si) {
			(*si)(section, level, record);
		}
	}
}

/** @} */ // group logging_backend

#ifdef __cplusplus
} // extern "C"
#endif

