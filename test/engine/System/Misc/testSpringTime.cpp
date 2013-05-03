/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define BOOST_TEST_MODULE Matrix44f
#include <boost/test/unit_test.hpp>

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"
#include <cmath>

#include <boost/chrono/include.hpp> // boost chrono
#include <boost/thread.hpp>

static const int testRuns = 1000000;

template<class Clock>
struct TestProcessor {
	static inline void Run()
	{
		ScopedOnceTimer timer(Clock::GetName());
		int64_t lastTick = Clock::Get();
		int64_t maxTick = 0;
		int64_t minTick = 1e9;
		float avgTick = 0;

		for (int i=0; i < testRuns; ++i) {
			int64_t curTick = Clock::Get();
			int64_t tick = curTick - lastTick;
			maxTick = std::max<int64_t>(tick, maxTick);
			minTick = std::min<int64_t>(tick, minTick);
			avgTick = float(i * avgTick + tick) / (i + 1);
			lastTick = curTick;
		}

		float maxMsTick = maxTick * Clock::ToMs();
		float minMsTick = std::max<int64_t>(minTick, 1LL) * Clock::ToMs();
		float avgMsTick = std::max<int64_t>(avgTick, 1.0f) * Clock::ToMs();
		LOG("[%17s] maxTick: %3.6fms minTick: %3.6fms avgTick: %3.6fms", Clock::GetName().c_str(), maxMsTick, minMsTick, avgMsTick);
	}
};

#include <SDL_timer.h>
struct SDLClock {
	static inline float ToMs() { return 1.0f; }
	static inline std::string GetName() { return "SDL_GetTicks"; }
	static inline int64_t Get() {
		return SDL_GetTicks();
	}
};


#ifdef Boost_TIMER_FOUND
#include <boost/timer/timer.hpp> // boost timer
static boost::timer::cpu_timer boost_clock;
struct BoostTimerClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "BoostTimer"; }
	static inline int64_t Get() {
		return boost_clock.elapsed().wall;
	}
};
#endif

struct BoostChronoClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "BoostChrono"; }
	static inline int64_t Get() {
		return boost::chrono::duration_cast<boost::chrono::nanoseconds>(boost::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
};


struct BoostChronoMicroClock {
	static inline float ToMs() { return 1.0f / 1e3; }
	static inline std::string GetName() { return "BoostChronoMicro"; }
	static inline int64_t Get() {
		return boost::chrono::duration_cast<boost::chrono::microseconds>(boost::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
};


#if __cplusplus > 199711L
#include <chrono>
struct Cpp11ChronoClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "StdChrono"; }
	static inline int64_t Get() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
};
#endif


#if defined(__USE_GNU) && !defined(WIN32)
#include <time.h>
struct PosixClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "clock_gettime"; }
	static inline int64_t Get() {
		timespec t1;
	#ifdef CLOCK_MONOTONIC_RAW
		clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
	#else
		clock_gettime(CLOCK_MONOTONIC, &t1);
	#endif
		return t1.tv_nsec + int64_t(t1.tv_sec) * int64_t(1e9);
	}
};
#endif


BOOST_AUTO_TEST_CASE( ClockQualityCheck )
{
	BOOST_CHECK(boost::chrono::high_resolution_clock::is_steady);
#if __cplusplus > 199711L
	BOOST_CHECK(std::chrono::high_resolution_clock::is_steady);
#endif

	TestProcessor<SDLClock>::Run();
	TestProcessor<BoostChronoClock>::Run();
	TestProcessor<BoostChronoMicroClock>::Run();
#ifdef Boost_TIMER_FOUND
	TestProcessor<BoostTimerClock>::Run();
#endif
#if defined(__USE_GNU) && !defined(WIN32)
	TestProcessor<PosixClock>::Run();
#endif
#if __cplusplus > 199711L
	TestProcessor<Cpp11ChronoClock>::Run();
#endif
}


void sleep_posix()  { boost::this_thread::sleep(boost::posix_time::milliseconds(1)); }
#ifdef BOOST_THREAD_USES_CHRONO
void sleep_chrono() { boost::this_thread::sleep_for(boost::chrono::nanoseconds( 1 )); }
#endif

void BenchmarkSleepFnc(const std::string& name, void (*sleep)() )
{
	spring_time t = spring_gettime();
	spring_time tmin, tmax;

	for (int i=0; i<1000; ++i) {
		sleep();

		spring_time diff = spring_gettime() - t;
		if ((diff > tmax) || !spring_istime(tmax)) tmax = diff;
		if ((diff < tmin) || !spring_istime(tmin)) tmin = diff;
		t = spring_gettime();
	}

	LOG("[%12s] min: %.6fms max: %.6fms", name.c_str(), tmin.toMilliSecsf(), tmax.toMilliSecsf());
}

BOOST_AUTO_TEST_CASE( ThreadSleepTime )
{
	BenchmarkSleepFnc("sleep_posix",  &sleep_posix);
#ifdef BOOST_THREAD_USES_CHRONO
	BenchmarkSleepFnc("sleep_chrono", &sleep_chrono);
#endif
}
