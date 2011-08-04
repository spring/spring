/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_BACKEND_H
#define LOG_BACKEND_H

/**
 * This is the universal, global backend for the ILog.h logging API.
 * It may format log records, and routes them to all the registered sinks.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name logging_backend_control
 * ILog.h backend control interface.
 */
///@{

typedef void (*log_sink_ptr)(const char* section, int level,
		const char* record);

/// Start routing log records to the supplied sink
void log_backend_registerSink(log_sink_ptr sink);

/// Stop routing log records to the supplied sink
void log_backend_unregisterSink(log_sink_ptr sink);

///@}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_BACKEND_H

