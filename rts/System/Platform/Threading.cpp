/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread* mainThread = NULL;
	static boost::thread::id mainThreadID;
	static Error* threadError = NULL;
	static boost::thread* loadingThread = NULL;
	static boost::thread::id loadingThreadID;

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
