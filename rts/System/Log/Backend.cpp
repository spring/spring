/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <set>

#include "Backend.h"
#include "DefaultFilter.h"
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
static _threadlocal log_record_t cur_record = {{0}, "", "",  0, 0};
static _threadlocal log_record_t prv_record = {{0}, "", "",  0, 0};


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
	const auto& sinks = log_formatter_getSinks();

	if (sinks.empty())
		return;

	cur_record.sec = section;
	cur_record.fmt = fmt;
	cur_record.lvl = level;

	// format the record
	log_formatter_format(&cur_record, arguments);

	// check for duplicates after formatting; can not be
	// done in log_frontend_record or log_filter_record
	const int cmp = (prv_record.msg != NULL && STRCASECMP(cur_record.msg, prv_record.msg) == 0);

	cur_record.cnt += cmp;
	cur_record.cnt *= cmp;

	if (cur_record.cnt >= log_filter_getRepeatLimit())
		return;


	// sink the record into each registered sink
	for (log_sink_ptr fptr: sinks) {
		fptr(level, section, cur_record.msg);
	}

	if (cur_record.cnt > 0)
		return;

	memcpy(prv_record.msg, cur_record.msg, sizeof(cur_record.msg));
}

/// Passes on a cleanup request to all sinks
void log_backend_cleanup() {
	for (log_cleanup_ptr fptr: log_formatter_getCleanupFuncs()) {
		fptr();
	}
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

