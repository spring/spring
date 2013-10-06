/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"
#include "System/Log/ILog.h"

#ifndef UNIT_TEST
#ifdef USING_CREG
#include "System/creg/Serializer.h"

//FIXME always use class even in non-debug! for creg!
CR_BIND(spring_time, );
CR_REG_METADATA(spring_time,(
	CR_IGNORED(x),
	CR_SERIALIZER(Serialize)
));
#endif
#endif


// mingw doesn't support std::this_thread (yet?)
#if defined(__MINGW32__) || defined(SPRINGTIME_USING_BOOST)
	#undef gt
	#include <boost/thread/thread.hpp>
	namespace this_thread { using namespace boost::this_thread; };
#else
	#define SPRINGTIME_USING_STD_SLEEP
	#define _GLIBCXX_USE_SCHED_YIELD // workaround a gcc <4.8 bug
	#include <thread>
	namespace this_thread { using namespace std::this_thread; };
#endif


namespace Cpp11Clock {
	#if (defined(WIN32) && !defined(FORCE_HIGHRES_TIMERS))
	#include <SDL/SDL_timer.h>
	#endif

	boost::int64_t Get() {
		#if (defined(WIN32) && !defined(FORCE_HIGHRES_TIMERS))
		assert(SDL_WasInit(SDL_INIT_TIMER));
		// upsample number of milliseconds since SDL library
		// initialization to nanoseconds (it is better to be
		// imprecise by six orders than totally broken...)
		return (FromMilliSecs<boost::uint32_t>(SDL_GetTicks()));
		#else
		return (chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count());
		#endif
	}
}



boost::int64_t spring_time::xs = 0;

#ifndef UNIT_TEST
void spring_time::Serialize(creg::ISerializer& s)
{
	if (s.IsWriting()) {
		int y = spring_tomsecs(*this - spring_gettime());
		s.SerializeInt(&y, 4);
	} else {
		int y;
		s.SerializeInt(&y, 4);
		*this = *this + spring_msecs(y);
	}
}
#endif



// FIXME: NOT THREADSAFE
static float avgYieldMs = 0.0f;
static float avgSleepErrMs = 0.0f;

static void thread_yield()
{
	const spring_time beforeYield = spring_time::gettime();

	this_thread::yield();

	const float diffMs = (spring_time::gettime() - beforeYield).toMilliSecsf();
	avgYieldMs = avgYieldMs * 0.9f + diffMs * 0.1f;
}


void spring_time::sleep()
{
	// for very short time intervals use a yielding loop (yield is ~5x more accurate than sleep(), check the UnitTest)
	if (toMilliSecsf() < (avgSleepErrMs + avgYieldMs * 5.0f)) {
		const spring_time s = gettime();
		while ((gettime() - s) < *this) {
			thread_yield();
		}
		return;
	}

	const spring_time expectedWakeUpTime = gettime() + *this;

	#if defined(SPRINGTIME_USING_STD_SLEEP)
		this_thread::sleep_for(chrono::nanoseconds(toNanoSecsi()));
	#else
		boost::this_thread::sleep(boost::posix_time::microseconds(std::ceil(toNanoSecsf() * 1e-3)));
	#endif

	const float diffMs = (gettime() - expectedWakeUpTime).toMilliSecsf();
	avgSleepErrMs = avgSleepErrMs * 0.9f + diffMs * 0.1f;

	//if (diffMs > 7.0f) {
	//	LOG_L(L_WARNING, "SpringTime: used sleep() function is too inaccurate");
	//}
}

void spring_time::sleep_until()
{
#if defined(SPRINGTIME_USING_STD_SLEEP)
	auto tp = chrono::time_point<chrono::high_resolution_clock, chrono::nanoseconds>(chrono::nanoseconds(toNanoSecsi()));
	this_thread::sleep_until(tp);
#else
	spring_time napTime = gettime() - *this;

	if (napTime.toMilliSecsf() < avgYieldMs) {
		while (napTime.isTime()) {
			thread_yield();
			napTime = gettime() - *this;
		}
		return;
	}

	napTime.sleep();
#endif
}

