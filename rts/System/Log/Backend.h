/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOG_BACKEND_H
#define _LOG_BACKEND_H

/**
 * This is the universal, global backend for the ILog.h logging API.
 * It may format log records, and routes them to all the registered sinks.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ILog.h backend control interface.
 * @group logging_backend_control
 * @{
 */

typedef void (*log_sink_ptr)(const char* section, int level,
		const char* record);

/// Start routing log records to the supplied sink
void log_backend_registerSink(log_sink_ptr sink);

/// Stop routing log records to the supplied sink
void log_backend_unregisterSink(log_sink_ptr sink);

/** @} */ // group logging_backend_control

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _LOG_BACKEND_H

