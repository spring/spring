/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"
#include "System/Misc/SpringTime.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <functional>

#ifndef _WIN32
	#include <mutex>
	#include <sys/syscall.h>
	#include <linux/futex.h>
#endif

#ifdef _WIN32
	#include <windows.h>
#endif

#define BOOST_TEST_MODULE Mutex
#include <boost/test/unit_test.hpp>

BOOST_GLOBAL_FIXTURE(InitSpringTime);

#ifndef _WIN32
	typedef boost::uint32_t futex;

	static void futex_init(futex* m)
	{
		*m = 0;
	}

	static void futex_destroy(futex* m)
	{
		*m = 0;
	}

	static void futex_lock(futex* m)
	{
		futex c;
		if ((c = __sync_val_compare_and_swap(m, 0, 1)) != 0)  {
			do {
				if ((c == 2) || __sync_val_compare_and_swap(m, 1, 2) != 0)
					syscall(SYS_futex, m, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
			} while((c = __sync_val_compare_and_swap(m, 0, 2)) != 0);
		}
	}

	static void futex_unlock(futex* m)
	{
		if (__sync_fetch_and_sub(m, 1) != 1) {
			*m = 0;
			syscall(SYS_futex, m, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
		}
	}
#endif



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

	spring::mutex spmtx;
	spring::recursive_mutex sprmtx;
	boost::mutex mtx;
	boost::recursive_mutex rmtx;

	spring_time tRaw    = Test("raw",                     []{                 }, []{                   });
	spring_time tSpMtx  = Test("spring::mutex",           [&]{ spmtx.lock();  }, [&]{ spmtx.unlock();  });
	spring_time tSpRMtx = Test("spring::recursive_mutex", [&]{ sprmtx.lock(); }, [&]{ sprmtx.unlock(); });
	spring_time tMtx    = Test("boost::mutex",            [&]{ mtx.lock();    }, [&]{ mtx.unlock();    });
	spring_time tRMtx   = Test("boost::recursive_mutex",  [&]{ rmtx.lock();   }, [&]{ rmtx.unlock();   });

#ifndef _WIN32
	std::mutex smtx;
	std::recursive_mutex srmtx;
	spring_time tSMtx  = Test("std::mutex",           [&]{ smtx.lock();  }, [&]{ smtx.unlock();  });
	spring_time tSRMtx = Test("std::recursive_mutex", [&]{ srmtx.lock(); }, [&]{ srmtx.unlock(); });
#endif

#ifndef _WIN32
	futex ftx;
	futex_init(&ftx);
	spring_time tCrit = Test("futex", [&]{ futex_lock(&ftx); }, [&]{ futex_unlock(&ftx); });
	futex_init(&ftx);
#endif

#ifdef _WIN32
	CRITICAL_SECTION cs;
	InitializeCriticalSection(&cs);
	spring_time tCrit = Test("critical section", [&]{ EnterCriticalSection(&cs); }, [&]{ LeaveCriticalSection(&cs); });
	DeleteCriticalSection(&cs);
#endif

	//BOOST_CHECK(tMtx.toMilliSecsi()  <= 4 * tRaw.toMilliSecsi());
	//BOOST_CHECK(tRMtx.toMilliSecsi() <= 4 * tRaw.toMilliSecsi());
}

BOOST_AUTO_TEST_CASE( ConditionVariable )
{
	spring::mutex m;
	spring::condition_variable_any cv;
	std::unique_lock<spring::mutex> lk(m);
	// check error in wait_for times
	spring_time t, emin, emax;
	float eavg = 0;
	for (int i=0; i<100; ++i) {
		const auto sleepTime = ((rand() % 50) + 1) * 100; // 100..5100ns

		t = spring_gettime();
		cv.wait_for(lk, std::chrono::nanoseconds(sleepTime));
		spring_time diff = (spring_gettime() - t) - spring_time::fromNanoSecs(sleepTime);

		if ((diff > emax) || !emax.isDuration()) emax = diff;
		if ((diff < emin) || !emin.isDuration()) emin = diff;
		eavg = float(i * eavg + std::abs(diff.toNanoSecsf())) / (i + 1);
	}
	LOG("[spring::condition_variable::wait_for] accuracy:={ err: %+.4fms %+.4fms erravg: %.4fms } ", emin.toMilliSecsf(), emax.toMilliSecsf(), eavg * 1e-6);
}
