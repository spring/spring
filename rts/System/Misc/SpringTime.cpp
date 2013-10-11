/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"
#include "System/maindefines.h"
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
	#ifdef _GLIBCXX_USE_SCHED_YIELD
	#undef _GLIBCXX_USE_SCHED_YIELD
	#endif
	#define _GLIBCXX_USE_SCHED_YIELD // workaround a gcc <4.8 bug
	#include <thread>
	namespace this_thread { using namespace std::this_thread; };
#endif

#define USE_NATIVE_WINDOWS_CLOCK (defined(WIN32) && !defined(FORCE_CHRONO_TIMERS))
#ifdef USE_NATIVE_WINDOWS_CLOCK
#include <windows.h>
#endif



namespace spring_clock {
	void PushTickRate(bool hres) {
		#ifdef USE_NATIVE_WINDOWS_CLOCK
		// set the number of milliseconds between interrupts
		// NOTE: THIS IS A GLOBAL OS SETTING, NOT PER PROCESS
		// (should not matter for users, SDL 1.2 also sets it)
		if (!hres) {
			timeBeginPeriod(1);
		}
		#endif
	}
	void PopTickRate(bool hres) {
		#ifdef USE_NATIVE_WINDOWS_CLOCK
		if (!hres) {
			timeEndPeriod(1);
		}
		#endif
	}

	#ifdef USE_NATIVE_WINDOWS_CLOCK
	// QPC wants the LARGE_INTEGER's to be qword-aligned
	__FORCE_ALIGN_STACK__
	boost::int64_t GetTicksNative(bool hres) {
		if (hres) {
			// NOTE:
			//   SDL 1.2 by default does not use QueryPerformanceCounter
			//   SDL 2.0 does (but code does not seem aware of the issues)
			//
			//   QPC is an interrupt-independent (unlike timeGetTime & co)
			//   virtual timer that runs at a "fixed" frequency which is
			//   derived from hardware, but can be *severely* affected by
			//   thermal drift (heavy CPU load will change the precision!)
			//
			//   QPC is an *interface* to either the TSC or the HPET or the
			//   ACPI timer, MS claims "it should not matter which processor
			//   is called" and setting thread affinity is only necessary in
			//   case QPC picks TSC (can happen if ACPI BIOS code is broken!)
			//
			//      const DWORD_PTR oldMask = SetThreadAffinityMask(::GetCurrentThread(), 0);
			//      QueryPerformanceCounter(...);
			//      SetThreadAffinityMask(::GetCurrentThread(), oldMask);
			//
			//   TSC is not invariant and completely unreliable on multi-core
			//   systems, but there exists an enhanced TSC on modern hardware
			//   which IS invariant (check CPUID 80000007H:EDX[8]) --> useful
			//   because reading TSC is much faster than an API call like QPC
			//
			//   the range of possible frequencies is *HUGE* (KHz - GHz) and
			//   the hardware counter might only have a 32-bit register while
			//   QuadPart is a 64-bit integer --> no monotonicity guarantees!
			//   (especially in combination with TSC if thread switches cores)
			LARGE_INTEGER tickFreq;
			LARGE_INTEGER currTick;

			if (!QueryPerformanceFrequency(&tickFreq))
				return (FromMilliSecs<boost::uint32_t>(0));

			QueryPerformanceCounter(&currTick);

			// we want the raw tick (uncorrected for frequency)
			// currTick.QuadPart /= tickFreq.QuadPart;

			if (tickFreq.QuadPart > 100000000000LL) return (FromNanoSecs <boost::uint64_t>(std::max(0LL, currTick.QuadPart)));
			if (tickFreq.QuadPart >     10000000LL) return (FromMicroSecs<boost::uint64_t>(std::max(0LL, currTick.QuadPart)));
			if (tickFreq.QuadPart >         1000LL) return (FromMilliSecs<boost::uint64_t>(std::max(0LL, currTick.QuadPart)));

			return (FromSecs<boost::uint64_t>(std::max(0LL, currTick.QuadPart)));
		} else {
			// timeGetTime is affected by time{Begin,End}Period whereas
			// GetTickCount is not ---> resolution of the former can be
			// configured but not for a specific process (they both read
			// from a shared counter that is updated by the system timer
			// interrupt)
			// it returns "the time elapsed since Windows was started"
			// (which is usually not a large value so there is little
			// risk of overflowing)
			//
			// note: there is a GetTickCount64 but no timeGetTime64
			return (FromMilliSecs<boost::uint32_t>(timeGetTime()));
		}
	}
	#endif

	boost::int64_t GetTicks(bool hres) {
		#ifdef USE_NATIVE_WINDOWS_CLOCK
		return (GetTicksNative(hres));
		#else
		return (chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count());
		#endif
	}

	const char* GetName(bool hres) {
		#ifdef USE_NATIVE_WINDOWS_CLOCK

		if (hres) {
			return "win32::QueryPerformanceCounter";
		} else {
			return "win32::TimeGetTime";
		}

		#else

		#ifdef SPRINGTIME_USING_BOOST
		return "boost::chrono::high_resolution_clock";
		#endif
		#ifdef SPRINGTIME_USING_STDCHRONO
		return "std::chrono::high_resolution_clock";
		#endif

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

