/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <functional>


#define BOOST_TEST_MODULE Mutex
#include <boost/test/unit_test.hpp>



typedef std::function<void()> voidFnc;

spring_time Test(const char* name, voidFnc pre, voidFnc post)
{
	ScopedOnceTimer timer(name);
	volatile int x = 0;
	for (int i = 0; i < 0xFFFF; ++i) {
		for (int j = 0; j < 0xFF; ++j) {
			pre();
			x += j / 3;
			x *= 0.33f;
			post();
		}
	}
	return timer.GetDuration();
}



BOOST_AUTO_TEST_CASE( Mutex )
{
	boost::mutex mtx;
	boost::recursive_mutex rmtx;

	spring_time tRaw  = Test("raw",             []{              },  []{                });
	spring_time tMtx  = Test("mutex",           [&]{ mtx.lock();  }, [&]{ mtx.unlock();  });
	spring_time tRMtx = Test("recursive_mutex", [&]{ rmtx.lock(); }, [&]{ rmtx.unlock(); });

	BOOST_CHECK(tMtx.toMilliSecsi()  <= 4 * tRaw.toMilliSecsi());
	BOOST_CHECK(tRMtx.toMilliSecsi() <= 4 * tRaw.toMilliSecsi());
}
