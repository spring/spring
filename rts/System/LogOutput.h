/*
LogOutput - global object to write log info to.
 Game UI elements that display log can subscribe to it to receive the log messages.
*/

#ifndef LOGOUTPUT_H
#define LOGOUTPUT_H

#include <stdarg.h>
#include <string>
#include <sstream>

// format string error checking
#ifdef __GNUC__
#define FORMATSTRING(n) __attribute__((format(printf, n, n + 1)))
#else
#define FORMATSTRING(n)
#endif

class float3;


/**
 * @brief defines a logging subsystem
 *
 * Each logging subsystem can be independently enabled/disabled, this allows
 * for adding e.g. very detailed logging that's by default off, but can be
 * turned on when troubleshooting.  (example: the virtual file system)
 *
 * A logging subsystem should be defined as a global variable, so it can be
 * used as argument to logOutput.Print similarly to a simple enum constant:
 *
 *	static CLogSubsystem LOG_MYSUBSYS("mysubsystem");
 *
 * ...then, in the actual code of your engine subsystem, use:
 *
 *		logOutput.Print(LOG_MYSUBSYS, "blah");
 *
 * All subsystems are linked together in a global list, allowing CLogOutput
 * to enable/disable subsystems based on env var and configuration key.
 */
class CLogSubsystem
{
public:
	static CLogSubsystem* GetList() { return linkedList; }
	CLogSubsystem* GetNext() { return next; }

	CLogSubsystem(const char* name, bool enabled = false);

	const char* const name;
	CLogSubsystem* const next;

	bool enabled;

private:
	static CLogSubsystem* linkedList;
};

/**
 * @brief ostringstream-derivate for simple logging
 *
 * usage:
 * LogObject() << "Important message with value: " << myint;
 * LogObject(mySubsys) << "Message";
 */
class LogObject
{
public:
	LogObject(const CLogSubsystem& subsys);
	LogObject();

	~LogObject();

	template <typename T>
	LogObject& operator<<(const T& t)
	{
		str << t;
		return *this;
	};

private:
	const CLogSubsystem& subsys;
	std::ostringstream str;
};

/** @brief implement this interface to be able to observe CLogOutput */
class ILogSubscriber
{
public:
	// Notification of log messages to subscriber
	virtual void NotifyLogMsg(const CLogSubsystem& subsystem, const char* str) = 0;
	virtual void SetLastMsgPos(const float3& pos) {}
};


/** @brief logging class */
class CLogOutput
{
public:
	CLogOutput();
	~CLogOutput();

	void Print(CLogSubsystem& subsystem, const char* fmt, ...) FORMATSTRING(3);
	void Print(const char* fmt, ...) FORMATSTRING(2);
	void Print(const std::string& text);
	void Prints(const CLogSubsystem& subsystem, const std::string& text); // canno be named Print, would be  not unique
	void Printv(CLogSubsystem& subsystem, const char* fmt, va_list argp);
	static CLogSubsystem& GetDefaultLogSubsystem();

	void SetLastMsgPos(const float3& pos);

	// In case the InfoConsole and other in game subscribers
	// should not be used anymore (SDL shutdown)
	void RemoveAllSubscribers();
	// Close the output file, so the crash reporter can copy it
	void End();

	void AddSubscriber(ILogSubscriber* ls);
	void RemoveSubscriber(ILogSubscriber* ls);

	void SetFilename(const char* filename);
	void Initialize();

protected:
	void InitializeSubsystems();
	void Output(const CLogSubsystem& subsystem, const std::string& str);

	void ToStdout(const CLogSubsystem& subsystem, const std::string message);
	void ToFile(const CLogSubsystem& subsystem, const std::string message);
};


extern CLogOutput logOutput;

#undef FORMATSTRING

#endif // LOGOUTPUT_H
