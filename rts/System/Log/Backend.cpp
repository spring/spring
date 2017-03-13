/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <set>

#include "Backend.h"
#include "LogUtil.h"
#include "System/maindefines.h"


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


// note: no real point to TLS, sinks themselves are not thread-safe
static _threadlocal log_record_t cur_record = {NULL, "", "", 0, 0};
static _threadlocal log_record_t prv_record = {NULL, "", "", 0, 0};


extern void log_formatter_format(log_record_t* log, va_list arguments);

void log_backend_registerSink(log_sink_ptr sink) { log_formatter_getSinks().insert(sink); }
void log_backend_unregisterSink(log_sink_ptr sink) { log_formatter_getSinks().erase(sink); }

void log_backend_registerCleanup(log_cleanup_ptr cleanupFunc) { log_formatter_getCleanupFuncs().insert(cleanupFunc); }
void log_backend_unregisterCleanup(log_cleanup_ptr cleanupFunc) { log_formatter_getCleanupFuncs().erase(cleanupFunc); }


/**
 * @name logging_backend
 * ILog.h backend implementation.
 */
///@{

// formats and routes the record to all sinks
void log_backend_record(int level, const char* section, const char* fmt, va_list arguments)
{
	const std::set<log_sink_ptr>& sinks = log_formatter_getSinks();

	if (sinks.empty())
		return;

	cur_record.sec = section;
	cur_record.fmt = fmt;
	cur_record.lvl = level;

	// format the record
	log_formatter_format(&cur_record, arguments);

	// check for duplicates after formatting; can not be
	// done in log_frontend_record or log_filter_record
	if (prv_record.msg != NULL && strcmp(cur_record.msg, prv_record.msg) == 0)
		return;

	// sink the record into each registered sink
	for (auto si = sinks.begin(); si != sinks.end(); ++si) {
		(*si)(level, section, cur_record.msg);
	}

	// reallocate buffer for copy if current is larger
	if (cur_record.len > prv_record.len) {
		delete[] prv_record.msg;
		prv_record.msg = NULL;
		prv_record.msg = new char[prv_record.len = cur_record.len];
	}

	strncpy(prv_record.msg, cur_record.msg, cur_record.len);
}

/// Passes on a cleanup request to all sinks
void log_backend_cleanup() {
	const std::set<log_cleanup_ptr>& cleanupFuncs = log_formatter_getCleanupFuncs();

	for (auto si = cleanupFuncs.begin(); si != cleanupFuncs.end(); ++si) {
		(*si)();
	}

	delete[] prv_record.msg; prv_record.msg = NULL; prv_record.len = 0;
	delete[] cur_record.msg; cur_record.msg = NULL; cur_record.len = 0;
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

