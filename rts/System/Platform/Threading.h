/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <boost/thread.hpp>

namespace Threading {
	static bool haveMainThreadID = false;
	static boost::thread::id mainThreadID;

	static void SetMainThreadID() {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThreadID = boost::this_thread::get_id();
		}
	}

	static bool IsMainThread() {
		return (boost::this_thread::get_id() == Threading::mainThreadID);
	}
};

#endif // _THREADING_H_
