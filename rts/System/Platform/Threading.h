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
#include "System/Threading/SpringThreading.h"

#include <atomic>
#include <functional>

#include <cinttypes>
#include <cstring>



namespace Threading {
	class ThreadControls;

	enum {
		THREAD_IDX_MAIN = 0,
		THREAD_IDX_LOAD = 1,
		THREAD_IDX_SND  = 2,
		THREAD_IDX_VFSI = 3,
		THREAD_IDX_WDOG = 4,
		THREAD_IDX_LAST = 5,
	};


	// used to indicate the result of a suspend or resume operation
	enum SuspendResult {
		THREADERR_NONE,
		THREADERR_NOT_RUNNING,
		THREADERR_MISC
	};

	// generic types & functions to handle OS native threads
#ifdef WIN32
	typedef DWORD     NativeThreadId;
	typedef HANDLE    NativeThreadHandle;
#else
	typedef pthread_t NativeThreadId;
	typedef pthread_t NativeThreadHandle;
#endif
	NativeThreadHandle GetCurrentThread();
	NativeThreadId GetCurrentThreadId();

#ifndef WIN32
	extern thread_local std::shared_ptr<ThreadControls> localThreadControls;
#endif


	/**
	 * Creates a new spring::thread whose entry function is wrapped by some boilerplate code that allows for suspend/resume.
	 * These suspend/resume controls are exposed via the ThreadControls object that is provided by the caller and initialized by the thread.
	 * The thread is guaranteed to be in a running and initialized state when this function returns.
	 *
	 * The threadCtls object is an optional return parameter that gives access to the Suspend/Resume controls under Linux.
	 *
	 */
	spring::thread CreateNewThread(std::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* threadCtls = nullptr);

	/**
	 * Retrieves a shared pointer to the current ThreadControls for the calling thread.
	 */
	std::shared_ptr<ThreadControls> GetCurrentThreadControls();

#ifndef WIN32
	void SetupCurrentThreadControls(std::shared_ptr<ThreadControls>& threadCtls);
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
		spring::mutex            mutSuspend;
		spring::condition_variable condInitialized;
		ucontext_t              ucontext;
		pid_t                   thread_id;
	#endif
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
	std::uint32_t GetAffinity();
	std::uint32_t SetAffinity(std::uint32_t cores_bitmask, bool hard = true);
	void SetAffinityHelper(const char* threadName, std::uint32_t affinity);
	std::uint32_t GetAvailableCoresMask();

	/**
	 * returns count of cpu cores/ hyperthreadings cores
	 */
	int GetPhysicalCpuCores(); /// physical cores only (excluding hyperthreading)
	int GetLogicalCpuCores();  /// physical + hyperthreading
	bool HasHyperThreading();

	/**
	 * Inform the OS kernel that we are a cpu-intensive task
	 */
	void SetThreadScheduler();

	/**
	 * Used to detect the main-thread which runs SDL, GL, Input, Sim, ...
	 */
	void SetMainThread();
	void SetGameLoadThread();
	void SetAudioThread();
	void SetFileSysThread();
	void SetWatchDogThread();

	bool IsMainThread();
	bool IsMainThread(NativeThreadId threadID);

	bool IsGameLoadThread();
	bool IsGameLoadThread(NativeThreadId threadID);

	bool IsAudioThread();
	bool IsAudioThread(NativeThreadId threadID);

	bool IsFileSysThread();
	bool IsFileSysThread(NativeThreadId threadID);

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
		Error() : flags(0) {
			memset(caption, 0, sizeof(caption));
			memset(message, 0, sizeof(message));
		}
		Error(const char* _caption, const char* _message, unsigned int _flags) : flags(_flags) {
			strncpy(caption, _caption, sizeof(caption) - 1);
			strncpy(message, _message, sizeof(message) - 1);
		}
		Error(const Error&) = delete;
		Error(Error&& e) { *this = std::move(e); }

		Error& operator = (const Error& e) = delete;
		Error& operator = (Error&& e) {
			memcpy(caption, e.caption, sizeof(caption));
			memcpy(message, e.message, sizeof(message));
			memset(e.caption, 0, sizeof(caption));
			memset(e.message, 0, sizeof(message));

			flags = e.flags;
			e.flags = 0;
			return *this;
		}

		bool Empty() const { return (caption[0] == 0 && message[0] == 0 && flags == 0); }

		char caption[512];
		char message[512];
		unsigned int flags;
	};

	const Error* GetThreadErrorC();
	      Error* GetThreadErrorM();
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
}

#endif // _THREADING_H_
