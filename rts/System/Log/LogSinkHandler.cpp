/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LogSinkHandler.h"

#include "Backend.h"

#include <string>
#include <vector>
#include <cassert>


LogSinkHandler logSinkHandler;

/// Records a log entry
static void log_sink_record_logSinkHandler(const char* section, int level,
		const char* record)
{
	logSinkHandler.RecordLogMessage((section == NULL) ? "" : section, level, record);
}


LogSinkHandler::LogSinkHandler()
	: sinking(true)
{
}

LogSinkHandler::~LogSinkHandler() {

	if (!sinks.empty()) {
		log_backend_unregisterSink(&log_sink_record_logSinkHandler);
	}
}

void LogSinkHandler::AddSink(ILogSink* logSink) {

	assert(logSink != NULL);
	sinks.push_back(logSink);

	if (sinks.size() == 1) {
		log_backend_registerSink(&log_sink_record_logSinkHandler);
	}
}

void LogSinkHandler::RemoveSink(ILogSink* logSink) {

	assert(logSink != NULL);
	std::vector<ILogSink*>::iterator lsi;
	for (lsi = sinks.begin(); lsi != sinks.end(); ++lsi) {
		if (*lsi == logSink) {
			sinks.erase(lsi);
			break;
		}
	}

	if (sinks.empty()) {
		log_backend_unregisterSink(&log_sink_record_logSinkHandler);
	}
}

void LogSinkHandler::SetSinking(bool enabled) {
	this->sinking = enabled;
}

bool LogSinkHandler::IsSinking() const {
	return sinking;
}

void LogSinkHandler::RecordLogMessage(const std::string& section, int level,
			const std::string& text) const
{
	if (!sinking) {
		return;
	}

	std::vector<ILogSink*>::const_iterator lsi;
	for (lsi = sinks.begin(); lsi != sinks.end(); ++lsi) {
		(*lsi)->RecordLogMessage(section, level, text);
	}
}
