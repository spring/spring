/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SpringTime.h"
#include "System/MainDefines.h"
#include "System/SpringMath.h"

#define FORCE_CHRONO_TIMERS

#include <chrono>
#include <atomic>

#if defined(USING_CREG) && !defined(UNIT_TEST)
	#include "System/creg/Serializer.h"

	//FIXME always use class even in non-debug! for creg!
	CR_BIND(spring_time, )
	CR_REG_METADATA(spring_time,(
		CR_IGNORED(x),
		CR_SERIALIZER(Serialize)
	))
#endif


#if (defined(WIN32) && !defined(FORCE_CHRONO_TIMERS))
	#define USE_NATIVE_WINDOWS_CLOCK 1
#endif

#if USE_NATIVE_WINDOWS_CLOCK
	#include <windows.h>
#endif


#ifdef _GLIBCXX_USE_SCHED_YIELD
#undef _GLIBCXX_USE_SCHED_YIELD
#endif
#define _GLIBCXX_USE_SCHED_YIELD // workaround a gcc <4.8 bug

#include "System/Threading/SpringThreading.h"
namespace this_thread { using namespace std::this_thread; }



namespace spring_clock {
	static bool highResMode = false;
	static bool timerInited = false;

	void PushTickRate(bool b) {
		assert(!timerInited);

		highResMode = b;
		timerInited = true;

		#if USE_NATIVE_WINDOWS_CLOCK
		// set the number of milliseconds between interrupts
		// NOTE: THIS IS A GLOBAL OS SETTING, NOT PER PROCESS
		// (should not matter for users, SDL 1.2 also sets it)
		if (!highResMode) {
			timeBeginPeriod(1);
		}
		#endif
	}
	void PopTickRate() {
		assert(timerInited);

		#if USE_NATIVE_WINDOWS_CLOCK
		if (!highResMode) {
			timeEndPeriod(1);
		}
		#endif
	}

	#if USE_NATIVE_WINDOWS_CLOCK
	// QPC wants the LARGE_INTEGER's to be qword-aligned
	__FORCE_ALIGN_STACK__
	std::int64_t GetTicksNative() {
		assert(timerInited);

		if (highResMode) {
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
				return (FromMilliSecs<std::int64_t>(0));

			QueryPerformanceCounter(&currTick);

			// we want the raw tick (uncorrected for frequency)
			// if clock ticks <freq> times per second, then the
			// total number of {milli,micro,nano}seconds elapsed
			// for any given tick is <tick> / <freq / resolution>
			// eg. if freq = 15000Hz and tick = 5000, then
			//        secs = 5000 / (15000 / 1e0) =                    0.3333333
			//   millisecs = 5000 / (15000 / 1e3) = 5000 / 15.000000 =       333
			//   microsecs = 5000 / (15000 / 1e6) = 5000 /  0.015000 =    333333
			//    nanosecs = 5000 / (15000 / 1e9) = 5000 /  0.000015 = 333333333
			//
			// currTick.QuadPart /= tickFreq.QuadPart;

			if (tickFreq.QuadPart >= std::int64_t(double(1e9))) return (FromNanoSecs <std::uint64_t>(std::max(double(0.0), currTick.QuadPart / (tickFreq.QuadPart * double(1e-9)))));
			if (tickFreq.QuadPart >= std::int64_t(double(1e6))) return (FromMicroSecs<std::uint64_t>(std::max(double(0.0), currTick.QuadPart / (tickFreq.QuadPart * double(1e-6)))));
			if (tickFreq.QuadPart >= std::int64_t(double(1e3))) return (FromMilliSecs<std::uint64_t>(std::max(double(0.0), currTick.QuadPart / (tickFreq.QuadPart * double(1e-3)))));

			return (FromSecs<std::int64_t>(std::max(0LL, currTick.QuadPart)));
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
			return (FromMilliSecs<std::uint32_t>(timeGetTime()));
		}
	}
	#endif

	std::int64_t GetTicks() {
		assert(timerInited);

		#if USE_NATIVE_WINDOWS_CLOCK
		return (GetTicksNative());
		#else
		return (chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count());
		#endif
	}

	const char* GetName() {
		assert(timerInited);

		#if USE_NATIVE_WINDOWS_CLOCK

		if (highResMode) {
			return "win32::QueryPerformanceCounter";
		} else {
			return "win32::TimeGetTime";
		}

		#else
		return "std::chrono::high_resolution_clock";
		#endif
	}
}



std::int64_t spring_time::xs = 0;

static std::atomic_int avgThreadYieldTimeMicroSecs = {0};
static std::atomic_int avgThreadSleepTimeMicroSecs = {0};


static void thread_yield()
{
	const spring_time t0 = spring_time::gettime();
	this_thread::yield();
	const spring_time t1 = spring_time::gettime();
	const spring_time dt = t1 - t0;

	if (t1 >= t0) {
		// yes, it's not 100% thread correct, but it's okay when 1 of 1 million writes is dropped
		int avg = avgThreadYieldTimeMicroSecs.load();
		int newAvg = mix<float>(avg, dt.toMicroSecsf(), 0.1f);
		avgThreadYieldTimeMicroSecs.store(newAvg);
	}
}


void spring_time::sleep(bool forceThreadSleep)
{
	if (forceThreadSleep) {
		spring::this_thread::sleep_for(chrono::nanoseconds(toNanoSecsi()));
		return;
	}


	// for very short time intervals use a yielding loop (yield is ~5x more accurate than sleep(), check the UnitTest)
	if (toMicroSecsi() < (avgThreadSleepTimeMicroSecs + avgThreadYieldTimeMicroSecs * 5)) {
		const spring_time s = gettime();

		while ((gettime() - s) < *this)
			thread_yield();

		return;
	}

	// expected wakeup time
	const spring_time t0 = gettime() + *this;

	spring::this_thread::sleep_for(chrono::nanoseconds(toNanoSecsi()));

	const spring_time t1 = gettime();
	const spring_time dt = t1 - t0;

	if (t1 >= t0) {
		// yes, it's not 100% thread correct, but it's okay when 1 of 1 million writes is dropped
		int avg = avgThreadSleepTimeMicroSecs.load();
		int newAvg = mix<float>(avg, dt.toMicroSecsf(), 0.1f);
		avgThreadSleepTimeMicroSecs.store(newAvg);
	}
}

void spring_time::sleep_until()
{
	auto tp = chrono::time_point<chrono::high_resolution_clock, chrono::nanoseconds>(chrono::nanoseconds(toNanoSecsi()));
	this_thread::sleep_until(tp);
}

#if defined USING_CREG && !defined UNIT_TEST
void spring_time::Serialize(creg::ISerializer* s)
{
	if (s->IsWriting()) {
		int y = spring_tomsecs(*this - spring_gettime());
		s->SerializeInt(&y, 4);
	} else {
		int y;
		s->SerializeInt(&y, 4);
		*this = *this + spring_msecs(y);
	}
}
#endif

