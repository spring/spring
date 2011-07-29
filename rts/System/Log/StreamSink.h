/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOG_STREAM_SINK_H
#define _LOG_STREAM_SINK_H

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to a C++ output stream.
 * This might be useful for unit tests.
 */

#include <ostream>

/**
 * ILog.h stream-sink control interface.
 * @group logging_sink_stream_control
 * @{
 */

/**
 * Set which tream to log to.
 * @param logStream where to log to, NULL for no logging.
 */
void log_sink_stream_setLogStream(std::ostream* logStream);


/** @} */ // group logging_sink_stream_control

#endif // _LOG_STREAM_SINK_H

