/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LogSinkHandler.h"
#include "Backend.h"

#include <string>
#include <cassert>



// forwards a log entry to all ILogSinks (e.g. InfoConsole) added to the handler
static void log_sink_record_logSinkHandler(int level, const char* section, const char* record)
{
	logSinkHandler.RecordLogMessage(level, (section == nullptr) ? "" : section, record);
}



void LogSinkHandler::AddSink(ILogSink* logSink) {
	assert(logSink != nullptr);

	if (sinks.empty())
		log_backend_registerSink(&log_sink_record_logSinkHandler);

	sinks.insert(logSink);
}

void LogSinkHandler::RemoveSink(ILogSink* logSink) {
	assert(logSink != nullptr);
	sinks.erase(logSink);

	if (!sinks.empty())
		return;

	log_backend_unregisterSink(&log_sink_record_logSinkHandler);
}

void LogSinkHandler::RecordLogMessage(
	int level,
	const std::string& section,
	const std::string& message
) {
	if (!sinking)
		return;

	// forward to clients (currently only InfoConsole)
	for (ILogSink* sink: sinks) {
		sink->RecordLogMessage(level, section, message);
	}
}

