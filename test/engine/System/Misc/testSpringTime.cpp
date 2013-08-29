/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define BOOST_TEST_MODULE SpringTime
#include <boost/test/unit_test.hpp>

#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"
#include <cmath>

#include <boost/chrono/include.hpp> // boost chrono
#include <boost/thread.hpp>

static const int testRuns = 1000000;


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


struct SpringClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "SpringTime"; }
	static inline int64_t Get() {
		return spring_time::gettime().toNanoSecs();
	}
};


template<class Clock>
struct TestProcessor {
	static inline float Run()
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
		return avgMsTick;
	}
};


BOOST_AUTO_TEST_CASE( ClockQualityCheck )
{
	BOOST_CHECK(boost::chrono::high_resolution_clock::is_steady);
#if __cplusplus > 199711L
	BOOST_WARN(std::chrono::high_resolution_clock::is_steady);
#endif

	float bestAvg = 1e9;

	bestAvg = std::min(bestAvg, TestProcessor<SDLClock>::Run());
	bestAvg = std::min(bestAvg, TestProcessor<BoostChronoClock>::Run());
	bestAvg = std::min(bestAvg, TestProcessor<BoostChronoMicroClock>::Run());
#ifdef Boost_TIMER_FOUND
	bestAvg = std::min(bestAvg, TestProcessor<BoostTimerClock>::Run());
#endif
#if defined(__USE_GNU) && !defined(WIN32)
	bestAvg = std::min(bestAvg, TestProcessor<PosixClock>::Run());
#endif
#if __cplusplus > 199711L
	bestAvg = std::min(bestAvg, TestProcessor<Cpp11ChronoClock>::Run());
#endif
	float springAvg = TestProcessor<SpringClock>::Run();

	bestAvg = std::min(bestAvg, springAvg);
	BOOST_CHECK( std::abs(springAvg - bestAvg) < 3.f * bestAvg );


	// check min precision range
	{
		const spring_time d = spring_time::fromNanoSecs(1e3); // 1us
		BOOST_CHECK( std::abs(1000.0f * d.toSecsf() - d.toMilliSecsf()) < d.toMilliSecsf() );
		BOOST_CHECK( d.toSecsf() > 0.0f );
	}

	// check max precision range
	{
		static const float DAYS_TO_SECS = 60*60*24;
		static const float SECS_TO_MS   = 1000;
		const spring_time d = spring_time(4 * DAYS_TO_SECS * SECS_TO_MS);
		BOOST_CHECK( std::abs(d.toSecsf() - (4 * DAYS_TO_SECS)) < 1.0f);
		BOOST_CHECK( d.toSecsf() > 0.0f ); // else there is a overflow!
	}

	// check toMilliSecsf precision range
	for (int i = 0; i<16; ++i) {
		const float f10ei = std::pow(10.0f, i);
		if (i > 12) {
			BOOST_WARN( std::abs(spring_time(f10ei).toMilliSecsf() - f10ei) < 1.0f);
		} else {
			BOOST_CHECK( std::abs(spring_time(f10ei).toMilliSecsf() - f10ei) < 1.0f);
		}
	}

	// check toMilliSecsf behind dot precision range
	for (int i = 0; i>=-6; --i) {
		const float f10ei = std::pow(10.0f, i);
		BOOST_CHECK( std::abs(spring_time(f10ei).toMilliSecsf()) > 0.0f);
	}

	// check toSecsf precision range
	for (int i = 0; i<12; ++i) {
		const float f10ei = std::pow(10.0f, i);
		if (i > 7) {
			// everything above 10e7 seconds might be unprecise
			BOOST_WARN( std::abs(spring_time::fromSecs(f10ei).toSecsf() - f10ei) < 1.0f);
		} else {
			// 10e7 seconds should be minimum in precision range
			BOOST_CHECK( std::abs(spring_time::fromSecs(f10ei).toSecsf() - f10ei) < 1.0f);
		}
	}

	// check toSecsf behind dot precision range
	for (int i = 0; i>=-9; --i) {
		const float f10ei = std::pow(10.0f, i);
		BOOST_CHECK( std::abs(spring_time(f10ei * 1000.f).toSecsf()) > 0.0f);
	}

	// check toSecs precision range
	boost::int64_t i10ei = 10;
	for (int i = 1; i<10; ++i) {
		BOOST_CHECK( std::abs(spring_time::fromSecs(i10ei).toSecs() - i10ei) < 1.0f);
		i10ei *= 10LL;
	}

	BOOST_CHECK( std::abs(spring_time(1).toMilliSecsf() - 1.0f) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e3).toSecsf() - 1e0) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e6).toSecsf() - 1e3) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e9).toSecsf() - 1e6) < 0.1f);
}



void sleep_posix()  { boost::this_thread::sleep(boost::posix_time::milliseconds(1)); }
#ifdef BOOST_THREAD_USES_CHRONO
void sleep_boost() { boost::this_thread::sleep_for(boost::chrono::nanoseconds( 1 )); }
#endif
void yield_boost() { boost::this_thread::yield(); }
#if (__cplusplus > 199711L) && !defined(__MINGW32__) && defined(_GLIBCXX_USE_SCHED_YIELD) //last one is a gcc 4.7 bug
#include <thread>
void yield_chrono() { std::this_thread::yield(); }
#endif
void sleep_spring() { spring_sleep(spring_msecs(0)); }


void BenchmarkSleepFnc(const std::string& name, void (*sleep)(), const int runs)
{
	spring_time t = spring_gettime();
	spring_time tmin, tmax;
	float tavg = 0;

	for (int i=0; i<runs; ++i) {
		sleep();

		spring_time diff = spring_gettime() - t;
		if ((diff > tmax) || !spring_istime(tmax)) tmax = diff;
		if ((diff < tmin) || !spring_istime(tmin)) tmin = diff;
		tavg = float(i * tavg + diff.toNanoSecs()) / (i + 1);
		t = spring_gettime();
	}

	LOG("[%12s] min: %.6fms avg: %.6fms max: %.6fms", name.c_str(), tmin.toMilliSecsf(), tavg * 1e-6, tmax.toMilliSecsf());
}

BOOST_AUTO_TEST_CASE( ThreadSleepTime )
{
	BenchmarkSleepFnc("sleep_posix", &sleep_posix, 1000);
#ifdef BOOST_THREAD_USES_CHRONO
	BenchmarkSleepFnc("sleep_boost", &sleep_boost, 100000);
#endif
	BenchmarkSleepFnc("yield_boost", &yield_boost, 1000000);
#if (__cplusplus > 199711L) && !defined(__MINGW32__) && defined(_GLIBCXX_USE_SCHED_YIELD) //last one is a gcc 4.7 bug
	BenchmarkSleepFnc("yield_chrono", &yield_chrono, 1000000);
#endif
	BenchmarkSleepFnc("sleep_spring", &sleep_spring, 1000000);
}
