/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "StdAfx.h"
#include "Threading.h"
#include "Rendering/GL/myGL.h"

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
#ifdef USE_GML
	static int const noThreadID = -1;
	static int simThreadID = noThreadID;
#else
	static boost::thread::id noThreadID;
	static boost::thread::id simThreadID;
#endif	


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

	void SetSimThread(bool set) {
#ifdef USE_GML // gmlThreadNumber is likely to be much faster than boost::this_thread::get_id()
		simThreadID = set ? gmlThreadNumber : noThreadID;
#else
		simThreadID = set ? boost::this_thread::get_id() : noThreadID;
#endif
	}
	bool IsSimThread() {
#ifdef USE_GML
		return gmlThreadNumber == simThreadID;
#else
		return boost::this_thread::get_id() == simThreadID;
#endif
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
