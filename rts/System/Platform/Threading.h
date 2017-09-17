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
#include <functional>
#include <atomic>
#include <cinttypes>



namespace Threading {

	class ThreadControls;

	extern thread_local std::shared_ptr<Threading::ThreadControls> threadCtls;

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
	 * Creates a new spring::thread whose entry function is wrapped by some boilerplate code that allows for suspend/resume.
	 * These suspend/resume controls are exposed via the ThreadControls object that is provided by the caller and initialized by the thread.
	 * The thread is guaranteed to be in a running and initialized state when this function returns.
	 *
	 * The ppThreadCtls object is an optional return parameter that gives access to the Suspend/Resume controls under Linux.
	 *
	 */
	spring::thread CreateNewThread(std::function<void()> taskFunc, std::shared_ptr<Threading::ThreadControls>* ppThreadCtls = nullptr);

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
	void SetWatchDogThread();

	bool IsMainThread();
	bool IsMainThread(NativeThreadId threadID);

	bool IsGameLoadThread();
	bool IsGameLoadThread(NativeThreadId threadID);

	bool IsAudioThread();
	bool IsAudioThread(NativeThreadId threadID);

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

		bool Empty() const { return (caption.empty() && message.empty() && flags == 0); }

		std::string caption;
		std::string message;
		unsigned int flags;
	};
	void SetThreadError(const Error& err);
	Error* GetThreadError();
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
