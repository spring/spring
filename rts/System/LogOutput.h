/*
LogOutput - global object to write log info to. 
 Game UI elements that display log can subscribe to it to receive the log messages.
*/

#ifndef LOGOUTPUT_H
#define LOGOUTPUT_H

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
	virtual void NotifyLogMsg(int priority, const char *str) = 0;
	virtual void SetLastMsgPos(const float3& pos) {}
};

class CLogOutput
{
public:
	CLogOutput();
	~CLogOutput();

	void Print(int priority, const char *fmt, ...) FORMATSTRING(3);
	void Print(const char *fmt, ...) FORMATSTRING(2); // priority 0
	void Print(const std::string& text);
	void Print(int priority, const std::string& text);

	CLogOutput& operator<<(int i);
	CLogOutput& operator<<(float f);
	CLogOutput& operator<<(const char* c);

	void SetLastMsgPos(float3 pos);

	// In case the InfoConsole and other in game subscribers 
	// should not be used anymore (SDL shutdown)
	void RemoveAllSubscribers(); 
	// Close the output file, so the crash reporter can copy it to the 
	void End(); 

	void AddSubscriber(ILogSubscriber *ls);
	void RemoveSubscriber(ILogSubscriber *ls);

protected:
	void Output(int priority, const char *str);

	std::vector<ILogSubscriber*> subscribers;
};


extern CLogOutput logOutput;

#undef FORMATSTRING

#endif // LOGOUTPUT_H
