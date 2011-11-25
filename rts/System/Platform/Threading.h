/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <string>
#ifdef WIN32
	#include <windows.h> // HANDLE & DWORD
#else
	#include <pthread.h>
#endif
#ifdef __APPLE__
	#include <libkern/OSAtomic.h> // OSAtomicIncrement64
#endif
#include "System/Platform/Win/win32.h"
#include <boost/cstdint.hpp>


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
	 * Used to detect the main-thread which runs SDL, GL, Input, Sim, ...
	 */
	void SetMainThread();
	bool IsMainThread();
	bool IsMainThread(NativeThreadId threadID);


	/**
	 * GML Functions
	 */
	void SetSimThread(bool set);
	bool IsSimThread();
	void SetBatchThread(bool set);
	bool IsBatchThread();


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
	#ifdef WIN32
		return (thID1 == thID2);
	#else
		return pthread_equal(thID1, thID2);
	#endif
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
	private:
	#ifdef _MSC_VER
		volatile __declspec(align(8)) boost::int64_t num;
	#else
		volatile __attribute__ ((aligned (8))) boost::int64_t num;
	#endif
	};
}

#endif // _THREADING_H_
