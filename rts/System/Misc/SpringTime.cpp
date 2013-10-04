/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"
#include "System/Log/ILog.h"

#ifdef USING_CREG
#include "System/creg/Serializer.h"

//FIXME always use class even in non-debug! for creg!
CR_BIND(spring_time, );
CR_REG_METADATA(spring_time,(
	CR_IGNORED(x),
	CR_SERIALIZER(Serialize)
));
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


static float avgYieldMs = 0.0f;
static float avgSleepErrMs = 0.0f;


static void yield()
{
	const spring_time beforeYield = spring_time::gettime();

	this_thread::yield();

	const auto diffMs = (spring_time::gettime() - beforeYield).toMilliSecsf();
	avgYieldMs = avgYieldMs * 0.9f + diffMs * 0.1f;
}


void spring_time::sleep()
{
	// for very short time intervals use a yielding loop (yield is ~5x more accurate than sleep(), check the UnitTest)
	if (toMilliSecsf() < (avgSleepErrMs + avgYieldMs * 5.0f)) {
		const spring_time s = gettime();
		while ((gettime() - s) < *this) {
			yield();
		}
		return;
	}

	const spring_time expectedWakeUpTime = gettime() + *this;

	#if defined(SPRINGTIME_USING_STD_SLEEP)
		this_thread::sleep_for(chrono::nanoseconds( toNanoSecsi() ));
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
	auto tp = chrono::time_point<chrono::high_resolution_clock, chrono::nanoseconds>(chrono::nanoseconds( toNanoSecsi() ));
	this_thread::sleep_until(tp);
#else
	spring_time napTime = gettime() - *this;

	if (napTime.toMilliSecsf() < avgYieldMs) {
		while (napTime.isTime()) {
			yield();
			napTime = gettime() - *this;
		}
		return;
	}

	napTime.sleep();
#endif
}
