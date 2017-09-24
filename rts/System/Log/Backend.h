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

typedef void (*log_sink_ptr)(int level, const char* section, const char* record);

/// Start routing log records to the supplied sink
void log_backend_registerSink(log_sink_ptr sink);

/// Stop routing log records to the supplied sink
void log_backend_unregisterSink(log_sink_ptr sink);


typedef void (*log_cleanup_ptr)();

/**
 * Registers a cleanup function, which will be called in certain exceptional
 * situations only, for example during the graceful handling of a crash.
 */
void log_backend_registerCleanup(log_cleanup_ptr cleanupFunc);

/**
 * Unregisters a cleanup function.
 */
void log_backend_unregisterCleanup(log_cleanup_ptr cleanupFunc);

///@}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_BACKEND_H

