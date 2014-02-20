/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LogSinkHandler.h"
#include "Backend.h"

#include <string>
#include <cassert>



/// Records a log entry
static void log_sink_record_logSinkHandler(const char* section, int level,
		const char* record)
{
	logSinkHandler.RecordLogMessage((section == NULL) ? "" : section, level, record);
}



void LogSinkHandler::AddSink(ILogSink* logSink) {
	assert(logSink != NULL);

	if (sinks.empty()) {
		log_backend_registerSink(&log_sink_record_logSinkHandler);
	}

	sinks.insert(logSink);
}

void LogSinkHandler::RemoveSink(ILogSink* logSink) {
	assert(logSink != NULL);
	sinks.erase(logSink);

	if (sinks.empty()) {
		log_backend_unregisterSink(&log_sink_record_logSinkHandler);
	}
}

void LogSinkHandler::RecordLogMessage(const std::string& section, int level,
			const std::string& text) const
{
	if (!sinking)
		return;

	for (auto lsi = sinks.begin(); lsi != sinks.end(); ++lsi) {
		(*lsi)->RecordLogMessage(section, level, text);
	}
}
