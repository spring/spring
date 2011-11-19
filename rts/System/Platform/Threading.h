/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <string>
#ifdef WIN32
	#include <windows.h> // HANDLE & DWORD
#else
	#include <pthread.h>
#endif
#include "System/Platform/Win/win32.h"
#include <boost/cstdint.hpp>


namespace Threading {
#ifdef WIN32
	typedef HANDLE NativeThreadHandle;
#else
	typedef pthread_t NativeThreadHandle;
#endif

#ifdef WIN32
	typedef DWORD NativeThreadId;
#else
	typedef pthread_t NativeThreadId;
#endif

	NativeThreadHandle GetCurrentThread();
	NativeThreadId GetCurrentThreadId();
	inline bool NativeThreadIdsEqual(const NativeThreadId thID1, const NativeThreadId thID2)
	{
	#ifdef WIN32
		return (thID1 == thID2);
	#else
		return pthread_equal(thID1, thID2);
	#endif
	}
	void SetMainThread();
	bool IsMainThread();
	bool IsMainThread(NativeThreadId threadID);

	struct Error {
		Error(const std::string& _caption, const std::string& _message, const unsigned int _flags) : caption(_caption), message(_message), flags(_flags) {};
		std::string caption;
		std::string message;
		unsigned int flags;
	};

	void SetSimThread(bool set);
	bool IsSimThread();

	void SetBatchThread(bool set);
	bool IsBatchThread();

	void SetThreadError(const Error& err);
	Error* GetThreadError();

	class AtomicCounterInt64 {
	public:
		AtomicCounterInt64(boost::int64_t start) { num = start; }
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
};

#endif // _THREADING_H_
