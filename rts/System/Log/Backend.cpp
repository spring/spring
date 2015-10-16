/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Backend.h"

#include <cstdio>
#include <cstdarg>
#include <set>




namespace {
	std::set<log_sink_ptr>& log_formatter_getSinks() {
		static std::set<log_sink_ptr> sinks;
		return sinks;
	}

	std::set<log_cleanup_ptr>& log_formatter_getCleanupFuncs() {
		static std::set<log_cleanup_ptr> cleanupFuncs;
		return cleanupFuncs;
	}
}

#ifdef __cplusplus
extern "C" {
#endif

extern char* log_formatter_format(const char* section, int level, const char* fmt, va_list arguments);

void log_backend_registerSink(log_sink_ptr sink) { log_formatter_getSinks().insert(sink); }
void log_backend_unregisterSink(log_sink_ptr sink) { log_formatter_getSinks().erase(sink); }

void log_backend_registerCleanup(log_cleanup_ptr cleanupFunc) { log_formatter_getCleanupFuncs().insert(cleanupFunc); }
void log_backend_unregisterCleanup(log_cleanup_ptr cleanupFunc) { log_formatter_getCleanupFuncs().erase(cleanupFunc); }

/**
 * @name logging_backend
 * ILog.h backend implementation.
 */
///@{

/// Eventually formats and routes the record to all sinks
void log_backend_record(const char* section, int level, const char* fmt, va_list arguments)
{
	const std::set<log_sink_ptr>& sinks = log_formatter_getSinks();

	if (sinks.empty()) {
		static bool warned = false;

		if (!warned) {
			warned = true;

			fprintf(stderr,
				"\nWARNING: A log message was recorded, but no sink is registered."
				"\n         (there will be no further warnings)\n\n");
		}
	} else {
		// format the record
		const char* record = log_formatter_format(section, level, fmt, arguments);

		// sink the record into each registered sink
		for (auto si = sinks.begin(); si != sinks.end(); ++si) {
			(*si)(section, level, record);
		}

		delete[] record;
	}
}

/// Passes on a cleanup request to all sinks
void log_backend_cleanup() {
	const std::set<log_cleanup_ptr>& cleanupFuncs = log_formatter_getCleanupFuncs();

	for (auto si = cleanupFuncs.begin(); si != cleanupFuncs.end(); ++si) {
		(*si)();
	}
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

