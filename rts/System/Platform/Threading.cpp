/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"

#include <boost/thread.hpp>
#ifdef WIN32
namespace windows {
	#include <windows.h>
};
#endif

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread::id mainThreadID;
	static NativeThreadId nativeMainThreadID;
	static Error* threadError = NULL;


	NativeThreadHandle GetCurrentThread()
	{
	#ifdef WIN32
		//! we need to use this cause GetCurrentThread() just returns a pseudo handle,
		//! which returns in all threads the current active one, so we need to translate it
		//! with DuplicateHandle to an absolute handle valid in our watchdog thread
		NativeThreadHandle hThread;
		DuplicateHandle(GetCurrentProcess(), ::GetCurrentThread(), GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		return hThread;
	#else
		return pthread_self();
	#endif
	}


	NativeThreadId GetCurrentThreadId()
	{
	#ifdef WIN32
		return ::GetCurrentThreadId();
	#else
		return pthread_self();
	#endif
	}


	bool NativeThreadIdsEqual(const NativeThreadId& thID1, const NativeThreadId& thID2)
	{
	#ifdef WIN32
		return (thID1 == thID2);
	#else
		return pthread_equal(thID1, thID2);
	#endif
	}


	void SetMainThread()
	{
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThreadID = boost::this_thread::get_id();
			nativeMainThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsMainThread()
	{
		return (boost::this_thread::get_id() == Threading::mainThreadID);
	}

	bool IsMainThread(NativeThreadId threadID)
	{
		return NativeThreadIdsEqual(threadID, Threading::nativeMainThreadID);
	}


	void SetThreadError(const Error& err)
	{
		threadError = new Error(err); //FIXME memory leak!
	}

	Error* GetThreadError()
	{
		return threadError;
	}
};
