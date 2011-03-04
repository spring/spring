/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread* mainThread = NULL;
	static boost::thread::id mainThreadID;
	static Error* threadError = NULL;
	static boost::thread* loadingThread = NULL;
	static boost::thread::id loadingThreadID;


	NativeThreadHandle _GetCurrentThread()
	{
	#ifdef WIN32
		//! we need to use this cause GetCurrentThread() just returns a pseudo handle,
		//! which returns in all threads the current active one, so we need to translate it
		//! with DuplicateHandle to an absolute handle valid in our watchdog thread
		NativeThreadHandle hThread;
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		return hThread;
	#else
		return pthread_self();
	#endif
	}


	NativeThreadId _GetCurrentThreadId()
	{
	#ifdef WIN32
		return GetCurrentThreadId();
	#else
		return pthread_self();
	#endif
	}


	void SetMainThread(boost::thread* mt) {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThread = mt;
			mainThreadID = mt ? mt->get_id() : boost::this_thread::get_id();
		}
	}

	void SetLoadingThread(boost::thread* lt) {
		loadingThread = lt;
		loadingThreadID = lt ? lt->get_id() : boost::this_thread::get_id();
	}

	boost::thread* GetMainThread() {
		return mainThread;
	}

	boost::thread* GetLoadingThread() {
		return loadingThread;
	}

	void SetThreadError(const Error& err) {
		threadError = new Error(err); //FIXME memory leak!
	}

	Error* GetThreadError() {
		return threadError;
	}

	bool IsMainThread() {
		return (boost::this_thread::get_id() == Threading::mainThreadID);
	}

	bool IsLoadingThread() {
		return (boost::this_thread::get_id() == Threading::loadingThreadID);
	}
};
