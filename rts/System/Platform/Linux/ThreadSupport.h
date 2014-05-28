#ifndef THREADSIGNALHANDLER_H
#define THREADSIGNALHANDLER_H

#include <ucontext.h>
#include <memory>
#include <boost/thread.hpp>

namespace Threading {

	class ThreadControls;

	void ThreadStart (boost::function<void()> taskFunc, std::shared_ptr<ThreadControls> * ppThreadCtls);

}

#endif // THREADSIGNALHANDLER_H
