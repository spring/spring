/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef THREADSIGNALHANDLER_H
#define THREADSIGNALHANDLER_H

#if defined(__APPLE__)
	// FIXME: exclusively for ucontext.h
	#define _XOPEN_SOURCE 700
#endif
#include <ucontext.h>
#include <memory>
#include <boost/thread.hpp>

namespace Threading {
	class ThreadControls;

	void ThreadStart(
		boost::function<void()> taskFunc,
		std::shared_ptr<ThreadControls>* ppCtlsReturn,
		ThreadControls* tempCtls
	);
}

int gettid ();

#endif // THREADSIGNALHANDLER_H
