/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Threading.h"
#include "Rendering/GL/myGL.h"

#include <boost/thread.hpp>
#if defined(__APPLE__)
#elif defined(WIN32)
	#include <windows.h>
#else
	#if defined(__USE_GNU)
		#include <sys/prctl.h>
	#endif
	#include <sched.h>
#endif


namespace Threading {
	static Error* threadError = NULL;
	static bool haveMainThreadID = false;
	static boost::thread::id mainThreadID;
	static NativeThreadId nativeMainThreadID;
#ifdef USE_GML
	static int const noThreadID = -1;
	static int simThreadID      = noThreadID;
	static int batchThreadID    = noThreadID;
#else
	static boost::thread::id noThreadID;
	static boost::thread::id simThreadID;
	static boost::thread::id batchThreadID;
#endif	
	boost::uint32_t SetAffinity(boost::uint32_t cores_bitmask, bool hard)
	{
		if (cores_bitmask == 0) {
			return 0xFFFFFF;
		}

	#if defined(__APPLE__)
		// no-op

	#elif defined(WIN32)
		// Get the available cores
		DWORD curMask;
		DWORD cpusSystem = 0;
		GetProcessAffinityMask(GetCurrentProcess(), &curMask, &cpusSystem);

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
		// Get the available cores
		cpu_set_t cpusSystem; CPU_ZERO(&cpusSystem);
		cpu_set_t cpusWanted; CPU_ZERO(&cpusWanted);
		sched_getaffinity(0, sizeof(cpu_set_t), &cpusSystem);

		// Create mask
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

	unsigned GetAvailableCores()
	{
		return boost::thread::hardware_concurrency();
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



	void SetMainThread()
	{
		if (!haveMainThreadID) {
			haveMainThreadID = true;
			mainThreadID = boost::this_thread::get_id();
			nativeMainThreadID = Threading::GetCurrentThreadId();
		}
	}

	bool IsMainThread()
	{
		return (boost::this_thread::get_id() == Threading::mainThreadID);
	}

	bool IsMainThread(NativeThreadId threadID)
	{
		return NativeThreadIdsEqual(threadID, Threading::nativeMainThreadID);
	}



	void SetSimThread(bool set) {
	#ifdef USE_GML // GML::ThreadNumber() is likely to be much faster than boost::this_thread::get_id()
		batchThreadID = simThreadID = set ? GML::ThreadNumber() : noThreadID;
	#else
		batchThreadID = simThreadID = set ? boost::this_thread::get_id() : noThreadID;
	#endif
	}
	bool IsSimThread() {
	#ifdef USE_GML
		return GML::ThreadNumber() == simThreadID;
	#else
		return boost::this_thread::get_id() == simThreadID;
	#endif
	}

	void SetBatchThread(bool set) {
	#ifdef USE_GML // GML::ThreadNumber() is likely to be much faster than boost::this_thread::get_id()
		batchThreadID = set ? GML::ThreadNumber() : noThreadID;
	#else
		batchThreadID = set ? boost::this_thread::get_id() : noThreadID;
	#endif
	}
	bool IsBatchThread() {
	#ifdef USE_GML
		return GML::ThreadNumber() == batchThreadID;
	#else
		return boost::this_thread::get_id() == batchThreadID;
	#endif
	}


	void SetThreadName(std::string newname)
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
