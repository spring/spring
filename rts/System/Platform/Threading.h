/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <string>
#ifndef WIN32
#include <pthread.h>
#include "System/Platform/Linux/ThreadSupport.h"
#include <semaphore.h>
#endif
#ifdef __APPLE__
#include <libkern/OSAtomic.h> // OSAtomicIncrement64
#endif

#include "System/Platform/Win/win32.h"
#include <functional>
#include <atomic>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>



namespace Threading {

	class ThreadControls;

	extern boost::thread_specific_ptr<std::shared_ptr<Threading::ThreadControls>> threadCtls;

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

	/**
	 * Used to indicate the result of a suspend or resume operation.
	 */
	enum SuspendResult {
		THREADERR_NONE,
		THREADERR_NOT_RUNNING,
		THREADERR_MISC
	};

	/**
	 * Creates a new boost::thread whose entry function is wrapped by some boilerplate code that allows for suspend/resume.
	 * These suspend/resume controls are exposed via the ThreadControls object that is provided by the caller and initialized by the thread.
	 * The thread is guaranteed to be in a running and initialized state when this function returns.
	 *
	 * The ppThreadCtls object is an optional return parameter that gives access to the Suspend/Resume controls under Linux.
	 *
	 */
	boost::thread CreateNewThread(boost::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* ppThreadCtls = nullptr);

	/**
	 * Retrieves a shared pointer to the current ThreadControls for the calling thread.
	 */
	std::shared_ptr<ThreadControls> GetCurrentThreadControls();

#ifndef WIN32
	void SetCurrentThreadControls(bool);
#else
	static inline void SetCurrentThreadControls(bool) {}
#endif

	/**
	 * @brief Provides suspend/resume functionality for worker threads.
	 */
	class ThreadControls {
	public:
		ThreadControls();

		/* These are implemented in System/Platform/<platform>/ThreadSupport.cpp */
		SuspendResult Suspend();
		SuspendResult Resume();

		NativeThreadHandle      handle;
		std::atomic<bool>       running;
	#ifndef WIN32
		boost::mutex            mutSuspend;
		boost::condition_variable condInitialized;
		ucontext_t              ucontext;
		pid_t                   thread_id;
	#endif

		friend void ThreadStart(
			boost::function<void()> taskFunc,
			std::shared_ptr<ThreadControls>* ppCtlsReturn,
			ThreadControls* tempCtls
		);
	};

	inline bool NativeThreadIdsEqual(const NativeThreadId thID1, const NativeThreadId thID2);


	/**
	 * Sets the affinity of the current thread
	 *
	 * Interpret <cores_bitmask> as a bit-mask indicating on which of the
	 * available system CPU's (which are numbered logically from 1 to N) we
	 * want to run. Note that this approach will fail when N > 32.
	 */
	void DetectCores();
	boost::uint32_t GetAffinity();
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
}


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

		// prefix
		boost::int64_t operator++() {
	#ifdef _MSC_VER
			return InterlockedIncrement64(&num);
	#elif defined(__APPLE__)
			return OSAtomicIncrement64(&num);
	#else // assuming GCC (__sync_add_and_fetch is a builtin)
			return __sync_add_and_fetch(&num, boost::int64_t(1));
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
