/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread *mainThread = NULL;
	static boost::thread::id mainThreadID;
	static std::runtime_error *threadError = NULL;

	void SetMainThread(boost::thread *mt) {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThread = mt;
			mainThreadID = mainThread->get_id();
		}
	}

	boost::thread *GetMainThread() {
		return mainThread;
	}

	void SetThreadError(const std::string &s) {
		threadError = new std::runtime_error(s);
	}
	std::runtime_error GetThreadError() {
		return threadError ? *threadError : std::runtime_error("Unknown error");
	}

	bool IsMainThread() {
		return (boost::this_thread::get_id() == Threading::mainThreadID);
	}
};

//FIXME: use TLS? maybe extract the GML code?
//NOTE: I already tried to use __thread, but it didn't worked, maybe it is limited to newer CPUs?

/*
#ifdef WIN32
#include <windows.h>

DWORD mainthread_id;

class CDoOnce {
public:
	CDoOnce()
	{
		mainthread_id = GetCurrentThreadId();
	}
} do_once;

bool IsMainThread()
{
	DWORD thread_id = GetCurrentThreadId();
	return (thread_id == mainthread_id);
}


#else // LINUX & OSX

#include <sys/syscall.h>
#include <unistd.h>

pid_t mainthread_id;

class CDoOnce {
public:
	CDoOnce()
	{
		mainthread_id = syscall(SYS_gettid);
	}
} do_once;

bool IsMainThread()
{
	pid_t thread_id = syscall(SYS_gettid);
	return (thread_id == mainthread_id);
}

#endif // WIN32
*/
