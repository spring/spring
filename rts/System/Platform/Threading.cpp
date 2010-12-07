/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"
#include "Rendering/GL/myGL.h"

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread* mainThread = NULL;
	static boost::thread::id mainThreadID;
	static Error* threadError = NULL;
#ifdef USE_GML
	static int const noThreadID = -1;
	static int simThreadID = noThreadID;
#else
	static boost::thread::id noThreadID;
	static boost::thread::id simThreadID;
#endif	

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
