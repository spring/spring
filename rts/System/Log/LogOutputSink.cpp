/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to logOutput.
 */

#include "LogOutput.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * ILog.h sink implementation.
 * @group logging_sink_logOutput
 * @{
 */

/// Records a log entry
void log_sink_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	logOutput.Printv(logOutput.GetDefaultLogSubsystem(), fmt, arguments);
}

/** @} */ // group logging_sink_logOutput

#ifdef __cplusplus
} // extern "C"
#endif

