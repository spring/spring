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

#ifndef DEDICATED
	#include "System/Sync/FPUCheck.h"
#endif

#include <functional>
#include <memory>
#include <boost/thread.hpp>
#include <cinttypes>
#if defined(__APPLE__) || defined(__FreeBSD__)
#elif defined(WIN32)
	#include <windows.h>
#else
	#if defined(__USE_GNU)
		#include <sys/prctl.h>
	#endif
	#include <sched.h>
#endif

#ifndef WIN32
	#include "Linux/ThreadSupport.h"
#endif

#ifndef UNIT_TEST
CONFIG(int, WorkerThreadCount).defaultValue(-1).safemodeValue(0).minimumValue(-1).description("Count of worker threads (including mainthread!) used in parallel sections.");
CONFIG(int, WorkerThreadSpinTime).defaultValue(5).minimumValue(0).description("The number of milliseconds worker threads will spin after no tasks to perform.");
#endif


namespace Threading {
	static std::shared_ptr<Error> threadError;

	static bool haveMainThreadID = false;
	static bool haveGameLoadThreadID = false;
	static bool haveWatchDogThreadID = false;

	static NativeThreadId nativeMainThreadID;
	static NativeThreadId nativeGameLoadThreadID;
	static NativeThreadId nativeWatchDogThreadID;

	boost::thread_specific_ptr<std::shared_ptr<Threading::ThreadControls>> threadCtls;

#if defined(__APPLE__) || defined(__FreeBSD__)
#elif defined(WIN32)
	static DWORD_PTR cpusSystem = 0;
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
		DWORD_PTR curMask;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &cpusSystem);

	#else
		// Get the available cores
		CPU_ZERO(&cpusSystem);
		sched_getaffinity(0, sizeof(cpu_set_t), &cpusSystem);
	#endif

		GetPhysicalCpuCores(); // (uses a static, too)
		inited = true;
	}


	std::uint32_t GetAffinity()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op
		return 0;

	#elif defined(WIN32)
		DWORD_PTR curMask;
		DWORD_PTR systemCpus;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &systemCpus);
		return curMask;
	#else
		cpu_set_t curAffinity;
		CPU_ZERO(&curAffinity);
		sched_getaffinity(0, sizeof(cpu_set_t), &curAffinity);

		std::uint32_t mask = 0;

		int numCpus = std::min(CPU_COUNT(&curAffinity), 32); // w/o the min(.., 32) `(1 << n)` could overflow!
		for (int n = numCpus - 1; n >= 0; --n) {
			if (CPU_ISSET(n, &curAffinity)) {
				mask |= (1 << n);
			}
		}

		return mask;
	#endif
	}


	std::uint32_t SetAffinity(std::uint32_t cores_bitmask, bool hard)
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
		return (result > 0) ? (std::uint32_t)cpusWanted : 0;
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

	void SetAffinityHelper(const char *threadName, std::uint32_t affinity) {
		const std::uint32_t cpuMask  = Threading::SetAffinity(affinity);
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


	std::uint32_t GetAvailableCoresMask()
	{
		std::uint32_t systemCores = 0;
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


	static std::uint32_t GetCpuCoreForWorkerThread(int index, std::uint32_t availCores, std::uint32_t avoidCores)
	{
		std::uint32_t ompCore = 1;

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
		return spring::thread::hardware_concurrency();
	}


	/** Function that returns the number of real cpu cores (not
	    hyperthreading ones). These are the total cores in the system
	    (across all existing processors, if more than one)*/
	int GetPhysicalCpuCores() {
		// Get CPU features
		static springproc::CpuId cpuid;
		return cpuid.getCoreTotalNumber();
	}


	bool HasHyperThreading() {
		return (GetLogicalCpuCores() > GetPhysicalCpuCores());
	}


	void InitThreadPool() {
		std::uint32_t systemCores   = Threading::GetAvailableCoresMask();
		std::uint32_t mainAffinity  = systemCores;
#ifndef UNIT_TEST
		mainAffinity &= configHandler->GetUnsigned("SetCoreAffinity");
#endif
		std::uint32_t ompAvailCores = systemCores & ~mainAffinity;

		{
#ifndef UNIT_TEST
			int workerCount = configHandler->GetInt("WorkerThreadCount");
			ThreadPool::SetThreadSpinTime(configHandler->GetInt("WorkerThreadSpinTime"));
#else
			int workerCount = -1;
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
		std::uint32_t ompCores = 0;
		ompCores = parallel_reduce([&]() -> std::uint32_t {
			const int i = ThreadPool::GetThreadNum();

			// 0 is the source thread, skip
			if (i == 0)
				return 0;

			std::uint32_t ompCore = GetCpuCoreForWorkerThread(i - 1, ompAvailCores, mainAffinity);
			//std::uint32_t ompCore = ompAvailCores;
			Threading::SetAffinity(ompCore);
			return ompCore;
		}, [](std::uint32_t a, boost::unique_future<std::uint32_t>& b) -> std::uint32_t { return a | b.get(); });

		// affinity of mainthread
		std::uint32_t nonOmpCores = ~ompCores;
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



	ThreadControls::ThreadControls():
		handle(0),
		running(false)
	{
#ifndef WIN32
		memset(&ucontext, 0, sizeof(ucontext_t));
#endif
	}



	std::shared_ptr<ThreadControls> GetCurrentThreadControls()
	{
		// If there is no object registered, need to return an "empty" shared_ptr
		if (threadCtls.get() == nullptr) {
			return std::shared_ptr<ThreadControls>();
		}

		return *(threadCtls.get());
	}


	spring::thread CreateNewThread(std::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* ppCtlsReturn)
	{
#ifndef WIN32
		// only used as locking mechanism, not installed by thread
		Threading::ThreadControls tempCtls;

		std::unique_lock<spring::mutex> lock(tempCtls.mutSuspend);
		spring::thread localthread(std::bind(Threading::ThreadStart, taskFunc, ppCtlsReturn, &tempCtls));

		// Wait so that we know the thread is running and fully initialized before returning.
		tempCtls.condInitialized.wait(lock);
#else
		spring::thread localthread(taskFunc);
#endif

		return localthread;
	}

	void SetMainThread() {
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			// springMainThreadID = spring::this_thread::get_id();
			nativeMainThreadID = Threading::GetCurrentThreadId();
		}

		SetCurrentThreadControls(false);
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
			// springGameLoadThreadID = spring::this_thread::get_id();
			nativeGameLoadThreadID = Threading::GetCurrentThreadId();
		}

		SetCurrentThreadControls(true);
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
			// springWatchDogThreadID = spring::this_thread::get_id();
			nativeWatchDogThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsWatchDogThread() {
		return NativeThreadIdsEqual(Threading::GetCurrentThreadId(), nativeWatchDogThreadID);
	}
	bool IsWatchDogThread(NativeThreadId threadID) {
		return NativeThreadIdsEqual(threadID, Threading::nativeWatchDogThreadID);
	}

	void SetThreadName(const std::string& newname)
	{
	#if defined(__USE_GNU) && !defined(WIN32)
		//alternative: pthread_setname_np(pthread_self(), newname.c_str());
		prctl(PR_SET_NAME, newname.c_str(), 0, 0, 0);
	#elif _MSC_VER
		const DWORD MS_VC_EXCEPTION = 0x406D1388;
		#pragma pack(push,8)
		struct THREADNAME_INFO
		{
			DWORD dwType; // Must be 0x1000.
			LPCSTR szName; // Pointer to name (in user addr space).
			DWORD dwThreadID; // Thread ID (-1=caller thread).
			DWORD dwFlags; // Reserved for future use, must be zero.
		} info;
		#pragma pack(pop)
		info.dwType = 0x1000;
		info.szName = newname.c_str();
		info.dwThreadID = (DWORD)-1;
		info.dwFlags = 0;
		__try {
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*) &info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	#endif
	}


	void SetThreadError(const Error& err) { threadError.reset(new Error(err)); }
	Error* GetThreadError() { return (threadError.get()); }
}

