/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_SINK_HANDLER_H
#define LOG_SINK_HANDLER_H

#include <string>
#include <vector>


/**
 * C++/OO API for log sinks.
 * @see MulticastLogSink
 */
class ILogSink {
public:
	virtual void RecordLogMessage(const std::string& section, int level,
			const std::string& text) = 0;
};

/**
 * Sends log messages to ILogSink implementations.
 */
class LogSinkHandler {
public:
	~LogSinkHandler();

	void AddSink(ILogSink* logSink);
	void RemoveSink(ILogSink* logSink);

	void RecordLogMessage(const std::string& section, int level,
			const std::string& text) const;

private:
	std::vector<ILogSink*> sinks;
};

extern LogSinkHandler logSinkHandler;

#endif // LOG_SINK_HANDLER_H

