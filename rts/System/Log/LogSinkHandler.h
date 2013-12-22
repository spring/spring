/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_SINK_HANDLER_H
#define LOG_SINK_HANDLER_H

#include <string>
#include <set>


/**
 * C++/OO API for log sinks.
 * @see MulticastLogSink
 */
class ILogSink {
public:
	virtual void RecordLogMessage(const std::string& section, int level,
			const std::string& text) = 0;
protected:
	~ILogSink() {}
};

/**
 * Sends log messages to ILogSink implementations.
 */
class LogSinkHandler {
public:
	LogSinkHandler(): sinking(true) {
	}

	static LogSinkHandler& GetInstance() {
		static LogSinkHandler lsh;
		return lsh;
	}

	void AddSink(ILogSink* logSink);
	void RemoveSink(ILogSink* logSink);

	/**
	 * @brief Enables/Disables sinking to registered sinks
	 *
	 * The list of registered sinks does not get altered when calling this
	 * function, just its use is enabled or disabled.
	 * Used, for example, in case the info-console and other in game
	 * subscribers should not be notified anymore (eg. SDL shutdown).
	 */
	void SetSinking(bool enabled) { sinking = enabled; }
	/**
	 * Indicates whether sinking to registered sinks is enabled
	 *
	 * The initial value is true.
	 */
	bool IsSinking() const { return sinking; }

	void RecordLogMessage(const std::string& section, int level,
			const std::string& text) const;

private:
	std::set<ILogSink*> sinks;
	/**
	 * Whether log records are passed on to registered sinks, or dismissed.
	 */
	bool sinking;
};

#define logSinkHandler (LogSinkHandler::GetInstance())

#endif // LOG_SINK_HANDLER_H

