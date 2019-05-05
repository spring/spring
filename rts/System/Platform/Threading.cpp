/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"
#include "System/bitops.h"
#include "System/Log/ILog.h"
#include "System/Platform/CpuID.h"

#ifndef DEDICATED
	#include "System/Sync/FPUCheck.h"
#endif

#include <functional>
#include <memory>
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



namespace Threading {
#ifndef WIN32
	thread_local std::shared_ptr<ThreadControls> localThreadControls;
#endif

	static NativeThreadId nativeThreadIDs[THREAD_IDX_LAST] = {};
	static Error threadError;

#if defined(__APPLE__) || defined(__FreeBSD__)
#elif defined(WIN32)
	static DWORD_PTR cpusSystem = 0;
#else
	static cpu_set_t cpusSystem;
#endif


	void DetectCores()
	{
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
	}



	#if defined(__APPLE__) || defined(__FreeBSD__)
	#elif defined(WIN32)
	#else
	static std::uint32_t CalcCoreAffinityMask(const cpu_set_t* cpuSet) {
		std::uint32_t coreMask = 0;

		// without the min(..., 32), `(1 << n)` could overflow
		const int numCPUs = std::min(CPU_COUNT(cpuSet), 32);

		for (int n = numCPUs - 1; n >= 0; --n) {
			if (CPU_ISSET(n, cpuSet))
				coreMask |= (1 << n);
		}

		return coreMask;
	}

	static void SetWantedCoreAffinityMask(const cpu_set_t* cpuSrcSet, cpu_set_t* cpuDstSet, std::uint32_t coreMask) {
		CPU_ZERO(cpuDstSet);

		const int numCPUs = std::min(CPU_COUNT(cpuSrcSet), 32);

		for (int n = numCPUs - 1; n >= 0; --n) {
			if ((coreMask & (1 << n)) != 0)
				CPU_SET(n, cpuDstSet);
		}

		CPU_AND(cpuDstSet, cpuDstSet, cpuSrcSet);
	}
	#endif



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

		return (CalcCoreAffinityMask(&curAffinity));
	#endif
	}


	std::uint32_t SetAffinity(std::uint32_t coreMask, bool hard)
	{
		if (coreMask == 0)
			return (~0);

	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op
		return 0;

	#elif defined(WIN32)
		// create mask
		DWORD_PTR cpusWanted = (coreMask & cpusSystem);
		DWORD_PTR result = 0;

		HANDLE thread = GetCurrentThread();

		// set the affinity
		if (hard) {
			result = SetThreadAffinityMask(thread, cpusWanted);
		} else {
			result = SetThreadIdealProcessor(thread, (DWORD)cpusWanted);
		}

		// return final mask
		return ((static_cast<std::uint32_t>(cpusWanted)) * (result > 0));
	#else
		cpu_set_t cpusWanted;

		// create wanted mask
		SetWantedCoreAffinityMask(&cpusSystem, &cpusWanted, coreMask);

		// set the affinity; return final mask (can differ from wanted)
		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpusWanted) == 0)
			return (CalcCoreAffinityMask(&cpusWanted));

		return 0;
	#endif
	}

	void SetAffinityHelper(const char* threadName, std::uint32_t affinity) {
		const std::uint32_t cpuMask = Threading::SetAffinity(affinity);

		if (cpuMask == ~0u) {
			LOG("[Threading] %s thread CPU affinity not set", threadName);
			return;
		}
		if (cpuMask != affinity) {
			LOG("[Threading] %s thread CPU affinity mask set: %d (config is %d)", threadName, cpuMask, affinity);
			return;
		}
		if (cpuMask == 0) {
			LOG_L(L_ERROR, "[Threading] %s thread CPU affinity mask failed: %d", threadName, affinity);
			return;
		}

		LOG("[Threading] %s thread CPU affinity mask set: %d", threadName, cpuMask);
	}


	std::uint32_t GetAvailableCoresMask()
	{
	#if defined(__APPLE__) || defined(__FreeBSD__)
		// no-op
		return (~0);
	#elif defined(WIN32)
		return cpusSystem;
	#else
		return (CalcCoreAffinityMask(&cpusSystem));
	#endif
	}


	int GetLogicalCpuCores() {
		// auto-detect number of system threads (including hyperthreading)
		return spring::thread::hardware_concurrency();
	}

	/** Function that returns the number of real cpu cores (not
	    hyperthreading ones). These are the total cores in the system
	    (across all existing processors, if more than one)*/
	int GetPhysicalCpuCores() {
		static springproc::CPUID cpuid;
		return cpuid.getTotalNumCores();
	}

	bool HasHyperThreading() { return (GetLogicalCpuCores() > GetPhysicalCpuCores()); }


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


#ifndef WIN32
	std::shared_ptr<ThreadControls> GetCurrentThreadControls() { return localThreadControls; }
#endif


	spring::thread CreateNewThread(std::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* threadCtls)
	{
#ifndef WIN32
		// only used as locking mechanism, not installed by thread
		Threading::ThreadControls tempCtls;

		std::unique_lock<spring::mutex> lock(tempCtls.mutSuspend);
		spring::thread localthread(std::bind(Threading::ThreadStart, taskFunc, threadCtls, &tempCtls));

		// wait so that we know the thread is running and fully initialized before returning
		tempCtls.condInitialized.wait(lock);
#else
		spring::thread localthread(taskFunc);
#endif

		return localthread;
	}



	static void SetThreadID(unsigned int threadIndex) {
		// NOTE:
		//   LOAD and SND thread ID's always have to be set unconditionally
		//   (threads are joined and respawned when reloading, so KISS here)
		//   other threads never call Set*Thread more than once, no need for
		//   caching
		nativeThreadIDs[threadIndex] = Threading::GetCurrentThreadId();

		switch (threadIndex) {
			case THREAD_IDX_LOAD: {
				// do nothing if Load is actually Main (LoadingMT=0 case)
				if (IsMainThread())
					return;
			} break;
			#ifndef WIN32
			// both heartBeatThread and soundThread make use of CreateNewThread -> ThreadStart
			// other threads under the eye of watchdog have their control structure setup here
			case THREAD_IDX_SND : { return; } break;
			#endif
			case THREAD_IDX_WDOG: { return; } break;
		}
	#ifndef WIN32
		SetupCurrentThreadControls(localThreadControls);
	#endif
	}

	void     SetMainThread() { SetThreadID(THREAD_IDX_MAIN); }
	void SetGameLoadThread() { SetThreadID(THREAD_IDX_LOAD); }
	void    SetAudioThread() { SetThreadID(THREAD_IDX_SND ); }
	void  SetFileSysThread() { SetThreadID(THREAD_IDX_VFSI); }
	void SetWatchDogThread() { SetThreadID(THREAD_IDX_WDOG); }

	bool IsMainThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_MAIN]); }
	bool IsMainThread(                       ) { return IsMainThread(Threading::GetCurrentThreadId()); }

	bool IsGameLoadThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_LOAD]); }
	bool IsGameLoadThread(                       ) { return IsGameLoadThread(Threading::GetCurrentThreadId()); }

	bool IsAudioThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_SND]); }
	bool IsAudioThread(                       ) { return IsAudioThread(Threading::GetCurrentThreadId()); }

	bool IsFileSysThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_VFSI]); }
	bool IsFileSysThread(                       ) { return IsFileSysThread(Threading::GetCurrentThreadId()); }

	bool IsWatchDogThread(NativeThreadId threadID) { return NativeThreadIdsEqual(threadID, nativeThreadIDs[THREAD_IDX_WDOG]); }
	bool IsWatchDogThread(                       ) { return IsWatchDogThread(Threading::GetCurrentThreadId()); }



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

	// NB: no protection against two threads posting at the same time
	const Error* GetThreadErrorC() { return &threadError; }
	      Error* GetThreadErrorM() { return &threadError; }
}

