/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#define BOOST_TEST_MODULE SpringTime
#include <boost/test/unit_test.hpp>

#include "System/TimeProfiler.h"
#include "System/Misc/SpringTime.h"
#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"
#include <cmath>

#include <boost/chrono/include.hpp> // boost chrono
#include <boost/thread.hpp>

BOOST_GLOBAL_FIXTURE(InitSpringTime);

// #define BOOST_MONOTONIC_RAW_CLOCK

static const int testRuns = 1000000;


#ifdef TEST_SDL
#include <SDL_timer.h>
struct SDLClock {
	static inline float ToMs() { return 1.0f; }
	static inline std::string GetName() { return "SDL_GetTicks"; }
	static inline int64_t Get() {
		// on Linux, same as clock_gettime(MT) with SDL 1.2
		// on Windows, same as GetTickCount or timeGetTime
		assert(SDL_WasInit(SDL_INIT_TIMER));
		return SDL_GetTicks();
	}
};
#endif


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
struct PosixClockMT {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "clock_gettime(MT)"; }
	static inline int64_t Get() {
		timespec t1;

		// boost::chrono has a system_clock (CLOCK_REALTIME --> affected
		// by NTP and can jump forward and backward) and a steady_clock
		// (CLOCK_MONOTONIC --> can be slewed by NTP but will never jump)
		//
		// which of the two becomes a typedef for high_resolution_clock
		// depends on BOOST_CHRONO_HAS_CLOCK_STEADY, note however there
		// is also a CLOCK_MONOTONIC_RAW (never slews or jumps which is
		// what we want, no NTP adjustments at all) but boost DOES *NOT*
		// USE this even in the latest release (1.54)!
	#if defined(CLOCK_MONOTONIC_RAW) && defined(BOOST_MONOTONIC_RAW_CLOCK)
		clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
	#else
		clock_gettime(CLOCK_MONOTONIC, &t1);
	#endif
		return t1.tv_nsec + int64_t(t1.tv_sec) * int64_t(1e9);
	}
};

struct PosixClockRT {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "clock_gettime(RT)"; }
	static inline int64_t Get() {
		timespec t1;
		clock_gettime(CLOCK_REALTIME, &t1);
		return t1.tv_nsec + int64_t(t1.tv_sec) * int64_t(1e9);
	}
};
#endif



struct SpringClock {
	static inline float ToMs() { return 1.0f / 1e6; }
	static inline std::string GetName() { return "SpringTime"; }
	static inline int64_t Get() {
		return spring_time::gettime().toNanoSecsi();
	}
};


template<class Clock>
struct TestProcessor {
	static inline float Run()
	{
		const auto startTestTime = spring_time::gettime();

		int64_t lastTick = Clock::Get();
		int64_t maxTick = 0;
		int64_t minTick = 1e9;
		int64_t lowTick = 1e9;
		float avgTick = 0;

		for (int i=0; i < testRuns; ++i) {
			int64_t curTick = Clock::Get();
			int64_t tick = curTick - lastTick;
			maxTick = std::max<int64_t>(tick, maxTick);
			minTick = std::min<int64_t>(tick, minTick);
			avgTick = float(i * avgTick + tick) / (i + 1);
			if (tick > 0) lowTick = std::min<int64_t>(tick, lowTick);
			lastTick = curTick;
		}

		float maxMsTick = maxTick * Clock::ToMs();
		float minMsTick = std::max<int64_t>(minTick, 1LL) * Clock::ToMs();
		float avgMsTick = std::max<int64_t>(avgTick, 1.0f) * Clock::ToMs();
		float minNonNullMsTick = lowTick * Clock::ToMs();

		LOG("[%17s] maxTick: %3.6fms minTick: %3.6fms avgTick: %3.6fms minNonNullTick: %3.6fms totalTestRuntime: %4.0fms", Clock::GetName().c_str(), maxMsTick, minMsTick, avgMsTick, minNonNullMsTick, (spring_time::gettime() - startTestTime).toMilliSecsf());
		return avgMsTick;
	}
};


BOOST_AUTO_TEST_CASE( ClockQualityCheck )
{
	LOG("Clock Precision Test");

	#ifdef BOOST_CHRONO_HAS_CLOCK_STEADY
	LOG("[%s] BOOST_CHRONO_HAS_CLOCK_STEADY defined --> CLOCK_MONOTONIC", __FUNCTION__);
	#else
	LOG("[%s] BOOST_CHRONO_HAS_CLOCK_STEADY undefined --> CLOCK_REALTIME", __FUNCTION__);
	#endif

	BOOST_CHECK(boost::chrono::high_resolution_clock::is_steady); // true if BOOST_CHRONO_HAS_CLOCK_STEADY
#if __cplusplus > 199711L
	BOOST_WARN(std::chrono::high_resolution_clock::is_steady);
#endif

	float bestAvg = 1e9;

	#ifdef TEST_SDL
	bestAvg = std::min(bestAvg, TestProcessor<SDLClock>::Run());
	#endif
	bestAvg = std::min(bestAvg, TestProcessor<BoostChronoClock>::Run());
	bestAvg = std::min(bestAvg, TestProcessor<BoostChronoMicroClock>::Run());
#ifdef Boost_TIMER_FOUND
	bestAvg = std::min(bestAvg, TestProcessor<BoostTimerClock>::Run());
#endif
#if defined(__USE_GNU) && !defined(WIN32)
	bestAvg = std::min(bestAvg, TestProcessor<PosixClockMT>::Run());
	bestAvg = std::min(bestAvg, TestProcessor<PosixClockRT>::Run());
#endif
#if __cplusplus > 199711L
	bestAvg = std::min(bestAvg, TestProcessor<Cpp11ChronoClock>::Run());
#endif
	float springAvg = TestProcessor<SpringClock>::Run();

	bestAvg = std::min(bestAvg, springAvg);

	const float diff = std::abs(springAvg - bestAvg);
	if (diff >= 3.0f * bestAvg) {
		LOG_L(L_ERROR, "Clockquality is bad: %f, running inside a VM?", diff);
	}


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
		BOOST_CHECK( std::abs(spring_time::fromSecs(i10ei).toSecsi() - i10ei) < 1.0f);
		i10ei *= 10LL;
	}

	BOOST_CHECK( std::abs(spring_time(1).toMilliSecsf() - 1.0f) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e3).toSecsf() - 1e0) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e6).toSecsf() - 1e3) < 0.1f);
	BOOST_CHECK( std::abs(spring_time(1e9).toSecsf() - 1e6) < 0.1f);

	spring_clock::PopTickRate();
}



#ifdef TEST_SDL
void sleep_sdl(int time)  { SDL_Delay(time); }
#endif

void sleep_boost_posix(int time)  { boost::this_thread::sleep(boost::posix_time::milliseconds(time)); }
void sleep_boost_posix2(int time) { boost::this_thread::sleep(boost::posix_time::microseconds(time)); }
#ifdef BOOST_THREAD_USES_CHRONO
	void sleep_boost_chrono(int time) { boost::this_thread::sleep_for(boost::chrono::nanoseconds(time)); }
#endif
void yield_boost(int time) { boost::this_thread::yield(); }
#if (__cplusplus > 199711L) && !defined(__MINGW32__) && defined(_GLIBCXX_USE_SCHED_YIELD) //last one is a gcc 4.7 bug
	#include <thread>
	void sleep_stdchrono(int time) { std::this_thread::sleep_for(std::chrono::nanoseconds(time)); }
	void yield_chrono(int time) { std::this_thread::yield(); }
#endif
void sleep_spring(int time) { spring_sleep(spring_msecs(time)); }
void sleep_spring2(int time) { spring_sleep(spring_time::fromNanoSecs(time)); }
#ifdef WIN32
	#include <windows.h>
	void sleep_windows(int time)  { Sleep(time); }
#else
	#include <time.h>
	#include <unistd.h>
	void sleep_posix_msec(int time)  { usleep(time); }
	void sleep_posix_nanosec(int time)  { struct timespec tim, tim2; tim.tv_sec = 0; tim.tv_nsec = time; if (nanosleep(&tim, &tim2) != 0) nanosleep(&tim2, NULL); }
#endif

void BenchmarkSleepFnc(const std::string& name, void (*sleep)(int time), const int runs, const float toMilliSecondsScale)
{
	// waste a few cycles to push the cpu to higher frequency states
	for (auto spinStopTime = spring_gettime() + spring_secs(2); spring_gettime() < spinStopTime; ) {
	}

	spring_time t = spring_gettime();
	spring_time tmin, tmax;
	float tavg = 0;

	// check lowest possible sleep tick
	for (int i=0; i<runs; ++i) {
		sleep(0);

		spring_time diff = spring_gettime() - t;
		if ((diff > tmax) || !spring_istime(tmax)) tmax = diff;
		if ((diff < tmin) || !spring_istime(tmin)) tmin = diff;
		tavg = float(i * tavg + diff.toNanoSecsi()) / (i + 1);
		t = spring_gettime();
	}

	// check error in sleeping times
	spring_time emin, emax;
	float eavg = 0;
	if (toMilliSecondsScale != 0) {
		for (int i=0; i<100; ++i) {
			const auto sleepTime = (rand() % 50) * 0.1f + 2; // 2..7ms

			t = spring_gettime();
			sleep(sleepTime * toMilliSecondsScale);
			spring_time diff = (spring_gettime() - t) - spring_msecs(sleepTime);

			if ((diff > emax) || !emax.isDuration()) emax = diff;
			if ((diff < emin) || !emin.isDuration()) emin = diff;
			eavg = float(i * eavg + std::abs(diff.toNanoSecsf())) / (i + 1);
		}
	}

	LOG("[%35s] accuracy:={ err: %+.4fms %+.4fms erravg: %.4fms } min sleep time:={ min: %.6fms avg: %.6fms max: %.6fms }", name.c_str(), emin.toMilliSecsf(), emax.toMilliSecsf(), eavg * 1e-6, tmin.toMilliSecsf(), tavg * 1e-6, tmax.toMilliSecsf());
}

BOOST_AUTO_TEST_CASE( ThreadSleepTime )
{
	LOG("Sleep() Precision Test");

	#ifdef TEST_SDL
	BenchmarkSleepFnc("sleep_sdl", &sleep_sdl, 500, 1e0);
	#endif
	BenchmarkSleepFnc("sleep_boost_posixtime_milliseconds", &sleep_boost_posix, 500, 1e0);
	BenchmarkSleepFnc("sleep_boost_posixtime_microseconds", &sleep_boost_posix2, 500, 1e3);
#ifdef BOOST_THREAD_USES_CHRONO
	BenchmarkSleepFnc("sleep_boost_chrono", &sleep_boost_chrono, 50000, 1e6);
#endif
	BenchmarkSleepFnc("yield_boost", &yield_boost, 500000, 0);
#if (__cplusplus > 199711L) && !defined(__MINGW32__) && defined(_GLIBCXX_USE_SCHED_YIELD) //last one is a gcc 4.7 bug
	BenchmarkSleepFnc("sleep_stdchrono", &sleep_stdchrono, 500, 1e6);
	BenchmarkSleepFnc("yield_chrono", &yield_chrono, 500000, 0);
#endif
#ifdef WIN32
	BenchmarkSleepFnc("sleep_windows", &sleep_windows, 500, 1e0);
#else
	BenchmarkSleepFnc("sleep_posix_msec", &sleep_posix_msec, 500, 1e0);
	BenchmarkSleepFnc("sleep_posix_nanosec", &sleep_posix_nanosec, 500, 1e6);
#endif
	BenchmarkSleepFnc("sleep_spring", &sleep_spring, 500, 1e0);
	BenchmarkSleepFnc("sleep_spring2", &sleep_spring2, 500, 1e6);
}
