/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <cstring>

#include <algorithm>
#include <array>

#include "Backend.h"
#include "DefaultFilter.h"
#include "LogUtil.h"
#include "System/MainDefines.h"

#define MAX_LOG_SINKS 8

namespace log_formatter {
	static size_t numSinks = 0;

	static std::array<log_sink_ptr, MAX_LOG_SINKS> sinks = {nullptr};
	static std::array<log_cleanup_ptr, MAX_LOG_SINKS> cleanupFuncs = {nullptr};

	size_t getNumSinks() { return numSinks; }

	bool insert_sink(log_sink_ptr sink) {
		const auto iter = std::find(sinks.begin(), sinks.end(), nullptr);

		// too many sinks
		if (iter == sinks.end())
			return false;
		// check for duplicates
		if (false && std::find(sinks.begin(), sinks.end(), sink) != sinks.end())
			return false;

		numSinks += 1;
		return (*iter = sink, true);
	}
	bool remove_sink(log_sink_ptr sink) {
		const auto iter = std::find(sinks.begin(), sinks.end(), sink);

		if (iter == sinks.end())
			return false;

		numSinks -= 1;
		return (*iter = nullptr, true);
	}

	bool insert_func(log_cleanup_ptr func) {
		const auto iter = std::find(cleanupFuncs.begin(), cleanupFuncs.end(), nullptr);

		// too many funcs (same maximum as sinks)
		if (iter == cleanupFuncs.end())
			return false;
		// check for duplicates
		if (false && std::find(cleanupFuncs.begin(), cleanupFuncs.end(), func) != cleanupFuncs.end())
			return false;

		return (*iter = func, true);
	}
	bool remove_func(log_cleanup_ptr func) {
		const auto iter = std::find(cleanupFuncs.begin(), cleanupFuncs.end(), func);

		if (iter == cleanupFuncs.end())
			return false;

		return (*iter = nullptr, true);
	}
}


#ifdef __cplusplus
extern "C" {
#endif


// note: no real point to TLS, sinks themselves are not thread-safe
static _threadlocal log_record_t cur_record = {{0}, "", "",  0, 0};
static _threadlocal log_record_t prv_record = {{0}, "", "",  0, 0};


extern void log_formatter_format(log_record_t* log, va_list arguments);

void log_backend_registerSink(log_sink_ptr sink) { log_formatter::insert_sink(sink); }
void log_backend_unregisterSink(log_sink_ptr sink) { log_formatter::remove_sink(sink); }

void log_backend_registerCleanup(log_cleanup_ptr cleanupFunc) { log_formatter::insert_func(cleanupFunc); }
void log_backend_unregisterCleanup(log_cleanup_ptr cleanupFunc) { log_formatter::remove_func(cleanupFunc); }


/**
 * @name logging_backend
 * ILog.h backend implementation.
 */
///@{

// formats and routes the record to all sinks
void log_backend_record(int level, const char* section, const char* fmt, va_list arguments)
{
	const auto& sinks = log_formatter::sinks;

	if (log_formatter::getNumSinks() == 0)
		return;

	cur_record.sec = section;
	cur_record.fmt = fmt;
	cur_record.lvl = level;

	// format the record
	log_formatter_format(&cur_record, arguments);

	// check for duplicates after formatting; can not be
	// done in log_frontend_record or log_filter_record
	const int cmp = (prv_record.msg[0] != 0 && STRCASECMP(cur_record.msg, prv_record.msg) == 0);

	cur_record.cnt += cmp;
	cur_record.cnt *= cmp;

	if (cur_record.cnt >= log_filter_getRepeatLimit())
		return;


	// sink the record into each registered sink
	for (log_sink_ptr fptr: sinks) {
		if (fptr == nullptr)
			continue;

		fptr(level, section, cur_record.msg);
	}

	if (cur_record.cnt > 0)
		return;

	memcpy(prv_record.msg, cur_record.msg, sizeof(cur_record.msg));
}

/// Passes on a cleanup request to all sinks
void log_backend_cleanup() {
	for (log_cleanup_ptr fptr: log_formatter::cleanupFuncs) {
		if (fptr == nullptr)
			continue;

		fptr();
	}
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

