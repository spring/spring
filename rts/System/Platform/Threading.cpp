/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread* mainThread = NULL;
	static boost::thread::id mainThreadID;
	static Error* threadError = NULL;

	void SetMainThread(boost::thread* mt) {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThread = mt;
			mainThreadID = mainThread->get_id();
		}
	}

	boost::thread* GetMainThread() {
		return mainThread;
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
};
