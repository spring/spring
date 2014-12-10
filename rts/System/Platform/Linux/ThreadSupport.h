/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef THREADSIGNALHANDLER_H
#define THREADSIGNALHANDLER_H

#include <ucontext.h>
#include <memory>
#include <boost/thread.hpp>

namespace Threading {

	class ThreadControls;

	void ThreadStart (boost::function<void()> taskFunc, std::shared_ptr<ThreadControls> * ppThreadCtls);

}

int gettid ();

#endif // THREADSIGNALHANDLER_H
