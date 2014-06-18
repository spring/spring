/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"
#include "Game/GameController.h"
#include "System/bitops.h"
#include "System/ThreadPool.h"
#ifndef UNIT_TEST
	#include "System/Config/ConfigHandler.h"
#endif
#include "System/Log/ILog.h"
#include "System/Platform/CpuID.h"
#include "System/Platform/CrashHandler.h"

#include <boost/version.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#if defined(__APPLE__) || defined(__FreeBSD__)
#elif defined(WIN32)
	#include <windows.h>
#else
	#if defined(__USE_GNU)
		#include <sys/prctl.h>
	#endif
	#include <sched.h>
#endif

#ifndef UNIT_TEST
CONFIG(int, WorkerThreadCount).defaultValue(-1).safemodeValue(0).minimumValue(-1).description("Count of worker threads (including mainthread!) used in parallel sections.");
CONFIG(int, WorkerThreadSpinTime).defaultValue(5).minimumValue(0).description("The number of milliseconds worker threads will spin after no tasks to perform.");
#endif


namespace Threading {
	static Error* threadError = NULL;

	static bool haveMainThreadID = false;
	static bool haveGameLoadThreadID = false;
	static bool haveWatchDogThreadID = false;

	// static boost::thread::id boostMainThreadID;
	// static boost::thread::id boostGameLoadThreadID;
	// static boost::thread::id boostWatchDogThreadID;

	static NativeThreadId nativeMainThreadID;
	static NativeThreadId nativeGameLoadThreadID;
	static NativeThreadId nativeWatchDogThreadID;

	static boost::optional<NativeThreadId> simThreadID;
	static boost::optional<NativeThreadId> luaBatchThreadID;

#if defined(__APPLE__) || defined(__FreeBSD__)
#elif defined(WIN32)
	static DWORD cpusSystem = 0;
#else
	static cpu_set_t cpusSystem;
#endif

	void DetectCores()
	{
		static bool inited = false;
		if (inited)
			return;

	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op

	#elif defined(WIN32)
		// Get the available cores
		DWORD curMask;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &cpusSystem);

	#else
		// Get the available cores
		CPU_ZERO(&cpusSystem);
		sched_getaffinity(0, sizeof(cpu_set_t), &cpusSystem);
	#endif

		GetPhysicalCpuCores(); // (uses a static, too)
		inited = true;
	}


	boost::uint32_t SetAffinity(boost::uint32_t cores_bitmask, bool hard)
	{
		if (cores_bitmask == 0) {
			return ~0;
		}

	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op
		return 0;

	#elif defined(WIN32)
		// Create mask
		DWORD_PTR cpusWanted = (cores_bitmask & cpusSystem);

		// Set the affinity
		HANDLE thread = GetCurrentThread();
		DWORD_PTR result = 0;
		if (hard) {
			result = SetThreadAffinityMask(thread, cpusWanted);
		} else {
			result = SetThreadIdealProcessor(thread, (DWORD)cpusWanted);
		}

		// Return final mask
		return (result > 0) ? (boost::uint32_t)cpusWanted : 0;
	#else
		// Create mask
		cpu_set_t cpusWanted; CPU_ZERO(&cpusWanted);
		int numCpus = std::min(CPU_COUNT(&cpusSystem), 32); // w/o the min(.., 32) `(1 << n)` could overflow!
		for (int n = numCpus - 1; n >= 0; --n) {
			if ((cores_bitmask & (1 << n)) != 0) {
				CPU_SET(n, &cpusWanted);
			}
		}
		CPU_AND(&cpusWanted, &cpusWanted, &cpusSystem);

		// Set the affinity
		int result = sched_setaffinity(0, sizeof(cpu_set_t), &cpusWanted);

		// Return final mask
		uint32_t finalMask = 0;
		for (int n = numCpus - 1; n >= 0; --n) {
			if (CPU_ISSET(n, &cpusWanted)) {
				finalMask |= (1 << n);
			}
		}
		return (result == 0) ? finalMask : 0;
	#endif
	}

	void SetAffinityHelper(const char *threadName, boost::uint32_t affinity) {
		const boost::uint32_t cpuMask  = Threading::SetAffinity(affinity);
		if (cpuMask == ~0) {
			LOG("[Threading] %s thread CPU affinity not set", threadName);
		}
		else if (cpuMask != affinity) {
			LOG("[Threading] %s thread CPU affinity mask set: %d (config is %d)", threadName, cpuMask, affinity);
		}
		else if (cpuMask == 0) {
			LOG_L(L_ERROR, "[Threading] %s thread CPU affinity mask failed: %d", threadName, affinity);
		}
		else {
			LOG("[Threading] %s thread CPU affinity mask set: %d", threadName, cpuMask);
		}
	}


	boost::uint32_t GetAvailableCoresMask()
	{
		boost::uint32_t systemCores = 0;
	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op
		systemCores = ~0;

	#elif defined(WIN32)
		// Create mask
		systemCores = cpusSystem;

	#else
		// Create mask
		const int numCpus = std::min(CPU_COUNT(&cpusSystem), 32); // w/o the min(.., 32) `(1 << n)` could overflow!
		for (int n = numCpus - 1; n >= 0; --n) {
			if (CPU_ISSET(n, &cpusSystem)) {
				systemCores |= (1 << n);
			}
		}
	#endif

		return systemCores;
	}


	static boost::uint32_t GetCpuCoreForWorkerThread(int index, boost::uint32_t availCores, boost::uint32_t avoidCores)
	{
		boost::uint32_t ompCore = 1;

		// find an unused core
		// count down cause hyperthread cores are appended to the end and we prefer those for our worker threads
		// (the physical cores are prefered for task specific threads)
		{
			while ((ompCore) && !(ompCore & availCores))
				ompCore <<= 1;
			int n = index;
			// select n'th bit in availCores
			while (n--)
				do ompCore <<= 1; while ((ompCore) && !(ompCore & availCores));
		}

		// select one of the mainthread cores if none found
		if (ompCore == 0) {
			/*int cntBits =*/ count_bits_set(avoidCores);
			ompCore = 1;
			while ((ompCore) && !(ompCore & avoidCores))
				ompCore <<= 1;
			int n = index;
			// select n'th bit in avoidCores
			while (n--)
				do ompCore <<= 1; while ((ompCore) && !(ompCore & avoidCores));
		}

		// fallback use all
		if (ompCore == 0) {
			ompCore = ~0;
		}

		return ompCore;
	}


	int GetLogicalCpuCores() {
		// auto-detect number of system threads (including hyperthreading)
		return boost::thread::hardware_concurrency();
	}


	int GetPhysicalCpuCores() {
		//FIXME substract HTT cores here!
		return GetLogicalCpuCores();
	}


	bool HasHyperThreading() {
		return (GetLogicalCpuCores() > GetPhysicalCpuCores());
	}


	void InitThreadPool() {
		boost::uint32_t systemCores   = Threading::GetAvailableCoresMask();
		boost::uint32_t mainAffinity  = systemCores;
#ifndef UNIT_TEST
		mainAffinity &= configHandler->GetUnsigned("SetCoreAffinity");
#endif
		boost::uint32_t ompAvailCores = systemCores & ~mainAffinity;

		{
			int workerCount = -1;
#ifndef UNIT_TEST
			workerCount = configHandler->GetUnsigned("WorkerThreadCount");
			ThreadPool::SetThreadSpinTime(configHandler->GetUnsigned("WorkerThreadSpinTime"));
#endif
			const int numCores = ThreadPool::GetMaxThreads();

			// For latency reasons our worker threads yield rarely and so eat a lot cputime with idleing.
			// So it's better we always leave 1 core free for our other threads, drivers & OS
			if (workerCount < 0) {
				if (numCores == 2) {
					workerCount = numCores;
				} else if (numCores < 6) {
					workerCount = numCores - 1;
				} else {
					workerCount = numCores / 2;
				}
			}
			if (workerCount > numCores) {
				LOG_L(L_WARNING, "Set ThreadPool workers to %i, but there are just %i cores!", workerCount, numCores);
				workerCount = numCores;
			}

			ThreadPool::SetThreadCount(workerCount);
		}

		// set affinity of worker threads
		boost::uint32_t ompCores = 0;
		ompCores = parallel_reduce([&]() -> boost::uint32_t {
			const int i = ThreadPool::GetThreadNum();

			// 0 is the source thread, skip
			if (i == 0)
				return 0;

			boost::uint32_t ompCore = GetCpuCoreForWorkerThread(i - 1, ompAvailCores, mainAffinity);
			//boost::uint32_t ompCore = ompAvailCores;
			Threading::SetAffinity(ompCore);
			return ompCore;
		}, [](boost::uint32_t a, boost::unique_future<boost::uint32_t>& b) -> boost::uint32_t { return a | b.get(); });

		// affinity of mainthread
		boost::uint32_t nonOmpCores = ~ompCores;
		if (mainAffinity == 0) mainAffinity = systemCores;
		Threading::SetAffinityHelper("Main", mainAffinity & nonOmpCores);
	}

	void SetThreadScheduler()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op

	#elif defined(WIN32)
		//TODO add MMCSS (http://msdn.microsoft.com/en-us/library/ms684247.aspx)
		//Note: only available with mingw64!!!

	#else
		if (GetLogicalCpuCores() > 1) {
			// Change os scheduler for this process.
			// This way the kernel knows that we are a CPU-intensive task
			// and won't randomly move us across the cores and tries
			// to maximize the runtime (_slower_ wakeups, less yields)
			//Note:
			// It _may_ be possible that this has negative impact in case
			// threads are waiting for mutexes (-> less yields).
			int policy;
			struct sched_param param;
			pthread_getschedparam(Threading::GetCurrentThread(), &policy, &param);
			pthread_setschedparam(Threading::GetCurrentThread(), SCHED_BATCH, &param);
		}
	#endif
	}


	NativeThreadHandle GetCurrentThread()
	{
	#ifdef WIN32
		// we need to use this cause GetCurrentThread() just returns a pseudo handle,
		// which returns in all threads the current active one, so we need to translate it
		// with DuplicateHandle to an absolute handle valid in our watchdog thread
		NativeThreadHandle hThread;
		::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		return hThread;
	#else
		return pthread_self();
	#endif
	}


	NativeThreadId GetCurrentThreadId()
	{
	#ifdef WIN32
		return ::GetCurrentThreadId();
	#else
		return pthread_self();
	#endif
	}



	void SetMainThread() {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			// boostMainThreadID = boost::this_thread::get_id();
			nativeMainThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsMainThread() {
		return NativeThreadIdsEqual(Threading::GetCurrentThreadId(), nativeMainThreadID);
	}
	bool IsMainThread(NativeThreadId threadID) {
		return NativeThreadIdsEqual(threadID, Threading::nativeMainThreadID);
	}



	void SetGameLoadThread() {
		if (!haveGameLoadThreadID) {
			haveGameLoadThreadID = true;
			// boostGameLoadThreadID = boost::this_thread::get_id();
			nativeGameLoadThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsGameLoadThread() {
		return NativeThreadIdsEqual(Threading::GetCurrentThreadId(), nativeGameLoadThreadID);
	}
	bool IsGameLoadThread(NativeThreadId threadID) {
		return NativeThreadIdsEqual(threadID, Threading::nativeGameLoadThreadID);
	}



	void SetWatchDogThread() {
		if (!haveWatchDogThreadID) {
			haveWatchDogThreadID = true;
			// boostWatchDogThreadID = boost::this_thread::get_id();
			nativeWatchDogThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsWatchDogThread() {
		return NativeThreadIdsEqual(Threading::GetCurrentThreadId(), nativeWatchDogThreadID);
	}
	bool IsWatchDogThread(NativeThreadId threadID) {
		return NativeThreadIdsEqual(threadID, Threading::nativeWatchDogThreadID);
	}



	void SetSimThread(bool set) {
		if (set) {
			simThreadID = Threading::GetCurrentThreadId();
			luaBatchThreadID = simThreadID;
		} else {
			simThreadID.reset();
		}
	}

	bool IsSimThread() {
		return ((!simThreadID)? false : NativeThreadIdsEqual(Threading::GetCurrentThreadId(), *simThreadID));
	}
	void SetLuaBatchThread(bool set) {
		if (set) {
			luaBatchThreadID = Threading::GetCurrentThreadId();
		} else {
			luaBatchThreadID.reset();
		}
	}
	bool IsLuaBatchThread() {
		return ((!luaBatchThreadID)? false : NativeThreadIdsEqual(Threading::GetCurrentThreadId(), *luaBatchThreadID));
	}

	void SetThreadName(const std::string& newname)
	{
	#if defined(__USE_GNU) && !defined(WIN32)
		//alternative: pthread_setname_np(pthread_self(), newname.c_str());
		prctl(PR_SET_NAME, newname.c_str(), 0, 0, 0);
	#endif
	}


	void SetThreadError(const Error& err)
	{
		threadError = new Error(err); //FIXME memory leak!
	}

	Error* GetThreadError()
	{
		return threadError;
	}
};
