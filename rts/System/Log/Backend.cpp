/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
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
	static std::array<log_sink_ptr, MAX_LOG_SINKS> sinks = {{nullptr}};
	static std::array<log_cleanup_ptr, MAX_LOG_SINKS> cleanupFuncs = {{nullptr}};

	static size_t numSinks = 0;
	static size_t numFuncs = 0;

	template<typename T, size_t S> bool array_insert(std::array<T, S>& array, T value, size_t& count) {
		const auto iter = std::find(array.begin(), array.end(), nullptr);

		// too many elems
		if (iter == array.end())
			return false;
		// check for duplicates
		// NOLINTNEXTLINE{readability-simplify-boolean-expr}
		if (false && std::find(array.begin(), array.end(), value) != array.end())
			return false;

		return (*iter = value, ++count);
	}
	template<typename T, size_t S> bool array_remove(std::array<T, S>& array, T value, size_t& count) {
		const auto iter = std::find(array.begin(), array.end(), value);

		if (iter == array.end())
			return false;

		// remove without leaving holes
		for (size_t i = iter - array.begin(), j = array.size() - 1; i < j; i++) {
			array[i] = array[i + 1];
		}

		return (array[--count] = nullptr, true);
	}


	bool insert_sink(log_sink_ptr sink) {
		return (array_insert(sinks, sink, numSinks));
	}
	bool remove_sink(log_sink_ptr sink) {
		return (array_remove(sinks, sink, numSinks));
	}

	bool insert_func(log_cleanup_ptr func) {
		return (array_insert(cleanupFuncs, func, numFuncs));
	}
	bool remove_func(log_cleanup_ptr func) {
		return (array_remove(cleanupFuncs, func, numFuncs));
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

	if (log_formatter::numSinks == 0)
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
	for (size_t i = 0; i < log_formatter::numSinks; i++) {
		assert(sinks[i] != nullptr);
		sinks[i](level, section, cur_record.msg);
	}

	if (cur_record.cnt > 0)
		return;

	memcpy(prv_record.msg, cur_record.msg, sizeof(cur_record.msg));
}

/// Passes on a cleanup request to all sinks
void log_backend_cleanup() {
	const auto& funcs = log_formatter::cleanupFuncs;

	for (size_t i = 0; i < log_formatter::numFuncs; i++) {
		assert(funcs[i] != nullptr);
		funcs[i]();
	}
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif

