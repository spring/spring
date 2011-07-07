/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all logging messages to logOutput.
 */

#include "ILog.h" // for LOG_LEVEL_*
#include "LogUtil.h" // for log_levelToString
#include "System/LogOutput.h"


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
	if (level != LOG_LEVEL_INFO) {
		// HACK this should be done later, closer to the point where it is written to a file or the console
		const char* levelStr = log_levelToString(level);
		const std::string prefixedFmt = std::string(levelStr) + ": " + fmt;
		logOutput.Printv(logOutput.GetDefaultLogSubsystem(),
				prefixedFmt.c_str(), arguments);
	} else {
		logOutput.Printv(logOutput.GetDefaultLogSubsystem(), fmt, arguments);
	}
}

/** @} */ // group logging_sink_logOutput

#ifdef __cplusplus
} // extern "C"
#endif

