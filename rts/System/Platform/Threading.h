/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <string>
#ifndef WIN32
	#include <pthread.h>
#endif
#ifdef __APPLE__
	#include <libkern/OSAtomic.h> // OSAtomicIncrement64
#endif

#include "System/Platform/Win/win32.h"
#include <boost/cstdint.hpp>


class CGameController;


namespace Threading {
	/**
	 * Generic types & functions to handle OS native threads
	 */
#ifdef WIN32
	typedef DWORD     NativeThreadId;
	typedef HANDLE    NativeThreadHandle;
#else
	typedef pthread_t NativeThreadId;
	typedef pthread_t NativeThreadHandle;
#endif
	NativeThreadHandle GetCurrentThread();
	NativeThreadId GetCurrentThreadId();

	inline bool NativeThreadIdsEqual(const NativeThreadId thID1, const NativeThreadId thID2);


	/**
	 * Sets the affinity of the current thread
	 *
	 * Interpret <cores_bitmask> as a bit-mask indicating on which of the
	 * available system CPU's (which are numbered logically from 1 to N) we
	 * want to run. Note that this approach will fail when N > 32.
	 */
	void DetectCores();
	boost::uint32_t SetAffinity(boost::uint32_t cores_bitmask, bool hard = true);
	void SetAffinityHelper(const char* threadName, boost::uint32_t affinity);
	boost::uint32_t GetAvailableCoresMask();

	/**
	 * returns count of cpu cores/ hyperthreadings cores
	 */
	int GetPhysicalCpuCores(); /// physical cores only (excluding hyperthreading)
	int GetLogicalCpuCores();  /// physical + hyperthreading
	bool HasHyperThreading();

	/**
	 * threadpool related stuff
	 */
	void InitThreadPool();

	/**
	 * Inform the OS kernel that we are a cpu-intensive task
	 */
	void SetThreadScheduler();

	/**
	 * Used to detect the main-thread which runs SDL, GL, Input, Sim, ...
	 */
	void SetMainThread();
	bool IsMainThread();
	bool IsMainThread(NativeThreadId threadID);

	void SetGameLoadThread();
	bool IsGameLoadThread();
	bool IsGameLoadThread(NativeThreadId threadID);

	void SetWatchDogThread();
	bool IsWatchDogThread();
	bool IsWatchDogThread(NativeThreadId threadID);

	/**
	 * GML Functions
	 */
	void SetSimThread(bool set);
	bool IsSimThread();

	void SetLuaBatchThread(bool set);
	bool IsLuaBatchThread();


	/**
	 * Give the current thread a name (posix-only)
	 */
	void SetThreadName(const std::string& newname);

	/**
	 * Used to raise errors in the main-thread issued by worker-threads
	 */
	struct Error {
		Error() : flags(0) {}
		Error(const std::string& _caption, const std::string& _message, const unsigned int _flags) : caption(_caption), message(_message), flags(_flags) {}

		std::string caption;
		std::string message;
		unsigned int flags;
	};
	void SetThreadError(const Error& err);
	Error* GetThreadError();


	/**
	 * A 64bit atomic counter
	 */
	struct AtomicCounterInt64;
};


//
// Inlined Definitions
//
namespace Threading {
	bool NativeThreadIdsEqual(const NativeThreadId thID1, const NativeThreadId thID2)
	{
	#ifdef __APPLE__
		// quote from the pthread_equal manpage:
		// Implementations may choose to define a thread ID as a structure.
		// This allows additional flexibility and robustness over using an int.
		//
		// -> Linux implementation seems always to be done as integer type.
		//    Only Windows & MacOSX use structs afaik. But for Windows we use their native thread funcs.
		//    So only MacOSX is left.
		{
			return pthread_equal(thID1, thID2);
		}
	#endif

		return (thID1 == thID2);
	}


	struct AtomicCounterInt64 {
	public:
		AtomicCounterInt64(boost::int64_t start = 0) : num(start) {}

		boost::int64_t operator++() {
	#ifdef _MSC_VER
			return InterlockedIncrement64(&num);
	#elif defined(__APPLE__)
			return OSAtomicIncrement64(&num);
	#else // assuming GCC (__sync_fetch_and_add is a builtin)
			return __sync_fetch_and_add(&num, boost::int64_t(1));
	#endif
		}

		boost::int64_t operator+=(int x) {
	#ifdef _MSC_VER
			return InterlockedExchangeAdd64(&num, boost::int64_t(x));
	#elif defined(__APPLE__)
			return OSAtomicAdd64(boost::int64_t(x), &num);
	#else // assuming GCC (__sync_fetch_and_add is a builtin)
			return __sync_fetch_and_add(&num, boost::int64_t(x));
	#endif
		}

		boost::int64_t operator-=(int x) {
	#ifdef _MSC_VER
			return InterlockedExchangeAdd64(&num, boost::int64_t(-x));
	#elif defined(__APPLE__)
			return OSAtomicAdd64(boost::int64_t(-x), &num);
	#else // assuming GCC (__sync_fetch_and_add is a builtin)
			return __sync_fetch_and_add(&num, boost::int64_t(-x));
	#endif
		}

		operator boost::int64_t() {
			return num;
		}

	private:
	#ifdef _MSC_VER
		__declspec(align(8)) boost::int64_t num;
	#else
		__attribute__ ((aligned (8))) boost::int64_t num;
	#endif
	};
}

#endif // _THREADING_H_
