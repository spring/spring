/*
LogOutput - global object to write log info to.
 Game UI elements that display log can subscribe to it to receive the log messages.
*/

#ifndef LOGOUTPUT_H
#define LOGOUTPUT_H

#include <stdarg.h>
#include <vector>
#include <string>

// format string error checking
#ifdef __GNUC__
#define FORMATSTRING(n) __attribute__((format(printf, n, n + 1)))
#else
#define FORMATSTRING(n)
#endif

class float3;

class ILogSubscriber
{
public:
	// Notification of log messages to subscriber
	virtual void NotifyLogMsg(int zone, const char *str) = 0;
	virtual void SetLastMsgPos(const float3& pos) {}
};

class CLogOutput
{
public:
	CLogOutput();
	~CLogOutput();

	void Print(int zone, const char *fmt, ...) FORMATSTRING(3);
	void Print(const char *fmt, ...) FORMATSTRING(2);
	void Print(const std::string& text);
	void Print(int zone, const std::string& text);
	void Printv(int zone, const char* fmt, va_list argp);

	CLogOutput& operator<<(const int i);
	CLogOutput& operator<<(const float f);
	CLogOutput& operator<<(const char* c);
	CLogOutput& operator<<(const float3& f);
	CLogOutput& operator<<(const std::string &f);	

	void SetLastMsgPos(const float3& pos);

	// In case the InfoConsole and other in game subscribers
	// should not be used anymore (SDL shutdown)
	void RemoveAllSubscribers();
	// Close the output file, so the crash reporter can copy it
	void End();

	void AddSubscriber(ILogSubscriber *ls);
	void RemoveSubscriber(ILogSubscriber *ls);

	void SetMirrorToStdout(bool);

protected:
	void Output(int zone, const char *str);

	std::vector<ILogSubscriber*> subscribers;
};


extern CLogOutput logOutput;

#undef FORMATSTRING

#endif // LOGOUTPUT_H
