/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOGOUTPUT_H
#define LOGOUTPUT_H

#include <stdarg.h>
#include <string>
#include <vector>
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
	virtual void NotifyLogMsg(const CLogSubsystem& subsystem, const std::string& str) = 0;
	virtual void SetLastMsgPos(const float3& pos) {}
	bool wantLogInformationPrefix; //set it to true if you want extra information appended such as frame number, log type, etc
};


/**
 * @brief logging class
 * Global object to write log info to.
 * Game UI elements that display log can subscribe to it to receive the log messages.
 */
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

	void AddSubscriber(ILogSubscriber* ls);
	void RemoveSubscriber(ILogSubscriber* ls);
	/**
	 * @brief Enables/Disables notifying of subscribers
	 *
	 * The list of subscribers does not get altered when calling this function,
	 * just its use is enabled or disabled.
	 * Used, for example, in case the info console and other in game subscribers
	 * should not be notified anymore (eg. SDL shutdown).
	 */
	void SetSubscribersEnabled(bool enabled);
	/**
	 * @brief Indicates whether notifying of subscribers is enabled
	 *
	 * The initial value is true.
	 */
	bool IsSubscribersEnabled() const;

	/// Close the output file, so the crash reporter can copy it
	void End();

	/**
	 * @brief set the log file
	 *
	 * Relative paths are relative to the writeable data-dir.
	 * This method may only be called as long as the logger is not yet
	 * initialized.
	 * @see Initialize()
	 */
	void SetFileName(std::string fileName);
	/**
	 * @brief returns the log file name (without path)
	 *
	 * Relative paths are relative to the writeable data-dir.
	 */
	const std::string& GetFileName() const;
	/**
	 * @brief returns the absolute path to the log file
	 *
	 * Relative paths are relative to the writeable data-dir.
	 * This method may only be called after the logger got initialzized.
	 * @see Initialize()
	 */
	const std::string& GetFilePath() const;

	/**
	 * @brief initialize logOutput
	 *
	 * Only after calling this method, logOutput starts writing to disk.
	 * The log file is written in the current directory so this may only be called
	 * after the engine chdir'ed to the correct directory.
	 */
	void Initialize();
	void Flush();

protected:
	/**
	 * @brief initialize the log subsystems
	 *
	 * This writes list of all available and all enabled subsystems to the log.
	 *
	 * Log subsystems can be enabled using the configuration key "LogSubsystems",
	 * or the environment variable "SPRING_LOG_SUBSYSTEMS".
	 *
	 * Both specify a comma separated list of subsystems that should be enabled.
	 * The lists from both sources are combined, there is no overriding.
	 *
	 * A subsystem that is by default enabled, can not be disabled.
	 */
	void InitializeSubsystems();
	/**
	 * @brief core log output method, used by all others
	 *
	 * Note that, when logOutput isn't initialized yet, the logging is done to the
	 * global std::vector preInitLog(), and is only written to disk in the call to
	 * Initialize().
	 *
	 * This method notifies all registered ILogSubscribers, calls OutputDebugString
	 * (for MSVC builds) and prints the message to stdout and the file log.
	 */
	void Output(const CLogSubsystem& subsystem, const std::string& str);

	void ToStdout(const CLogSubsystem& subsystem, const std::string& message);
	void ToFile(const CLogSubsystem& subsystem, const std::string& message);

private:
	/**
	 * @brief creates an absolute file path from a file name
	 *
	 * Will use the CWD, whihc should be the writeable data-dir format
	 * absoluteification.
	 */
	static std::string CreateFilePath(const std::string& fileName);

	/**
	 * @brief enable/disable log file rotation
	 *
	 * The default is determined by the config setting RotateLogFiles and
	 * whether this is a DEBUG build or not.
	 * RotateLogFiles defaults to "auto", and could be set to "never"
	 * or "always". On "auto", it will rotate logs only for debug builds.
	 * You may only call this as long as the logger did not yet get initialized.
	 */
	void SetLogFileRotating(bool enabled);
	bool IsLogFileRotating() const;
	/**
	 * @brief ff enabled, moves the log file of the last run
	 *
	 * Moves the log file of the last run, to preserve it,
	 * if log file rotation is enabled.
	 *
	 * By default, this is enabled only for DEBUG builds;
	 * ... (describe config file value here)
	 */
	void RotateLogFile() const;


	std::vector<ILogSubscriber*> subscribers;
	std::string fileName;
	std::string filePath;
	bool rotateLogFiles;
	bool subscribersEnabled;
};


extern CLogOutput logOutput;

#undef FORMATSTRING

#endif // LOGOUTPUT_H

