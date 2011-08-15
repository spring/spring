/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_STREAM_SINK_H
#define LOG_STREAM_SINK_H

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to a C++ output stream.
 * This might be useful for unit tests.
 */

#include <ostream>

/**
 * @name logging_sink_stream_control
 * ILog.h stream-sink control interface.
 */
///@{

/**
 * Set which tream to log to.
 * @param logStream where to log to, NULL for no logging.
 */
void log_sink_stream_setLogStream(std::ostream* logStream);

///@}

#endif // LOG_STREAM_SINK_H

