/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include <sys/syscall.h>

#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"


#define LOG_SECTION_CRASHHANDLER "CrashHandler"


// already registerd in CrashHandler.cpp
//LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_CRASHHANDLER);

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
        #undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_CRASHHANDLER


namespace Threading {


enum LinuxThreadState {
	LTS_RUNNING,
	LTS_SLEEP,
	LTS_DISK_SLEEP,
	LTS_STOPPED,
	LTS_PAGING,
	LTS_ZOMBIE,
	LTS_UNKNOWN
};

/**
 * There is no glibc wrapper for this system call, so you have to write one:
 */
static int gettid () {
	long tid = syscall(SYS_gettid);
	return tid;
}

/**
 * This method requires at least a 2.6 kernel in order to access the file /proc/<pid>/task/<tid>/status.
 *
 * @brief GetLinuxThreadState
 * @param handle
 * @return
 */
static LinuxThreadState GetLinuxThreadState(int tid)
{
	char filename[64];
	int pid = getpid();
	snprintf(filename, 64, "/proc/%d/task/%d/status", pid, tid);
	std::fstream sfile;
	sfile.open(filename, std::fstream::in);
	if (sfile.fail()) {
		LOG_L(L_WARNING, "[%s] could not query %s", __func__, filename);
		sfile.close();
		return LTS_UNKNOWN;
	}
	char statestr[64];
	char flags[64];
	sfile.getline(statestr,64); // first line isn't needed
	sfile.getline(statestr,64); // second line contains thread running state
	sscanf(statestr, "State: %s", flags);

	switch (flags[0]) {
		case 'R': return LTS_RUNNING;
		case 'S': return LTS_SLEEP;
		case 'D': return LTS_DISK_SLEEP;
		case 'T': return LTS_STOPPED;
		case 'W': return LTS_PAGING;
		case 'Z': return LTS_ZOMBIE;
	}
	return LTS_UNKNOWN;
}


static void ThreadSIGUSR1Handler(int signum, siginfo_t* info, void* pCtx)
{
	LOG_L(L_DEBUG, "[%s][1]", __func__);

	// Fill in ucontext_t structure before locking, this allows stack walking...
	const int err = getcontext(&(localThreadControls->ucontext));

	if (err != 0) {
		LOG_L(L_ERROR, "[%s] couldn't get thread context within suspend signal handler: %s", __func__, strerror(err));
		return;
	}

	// Change the "running" flag to false. Note that we don't own a lock on the suspend mutex, but in order to get here,
	//   it had to have been locked by some other thread.
	localThreadControls->running.store(false);

	LOG_L(L_DEBUG, "[%s][2]", __func__);

	// Wait on the mutex. This should block the thread.
	{
		localThreadControls->mutSuspend.lock();
		localThreadControls->running.store(true);
		localThreadControls->mutSuspend.unlock();
	}

	LOG_L(L_DEBUG, "[%s][3]", __func__);
}


static bool SetThreadSignalHandler()
{
	// Installing new ThreadControls object, so install signal handler also
	int err = 0;
	sigset_t sigSet;
	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGUSR1);

	err = pthread_sigmask(SIG_UNBLOCK, &sigSet, nullptr);

	if (err != 0) {
		LOG_L(L_FATAL, "[%s] error while setting new pthread's signal mask: %s", __func__, strerror(err));
		return false;
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = ThreadSIGUSR1Handler;
	sa.sa_flags |= SA_SIGINFO;

	if (sigaction(SIGUSR1, &sa, nullptr)) {
		LOG_L(L_FATAL, "[%s] error while installing pthread SIGUSR1 handler", __func__);
		return false;
	}

	return true;
}


void SetupCurrentThreadControls(std::shared_ptr<ThreadControls>& threadCtls)
{
	assert(!Threading::IsWatchDogThread());

	#ifndef WIN32
	if (threadCtls.get() != nullptr) {
		// old shared_ptr will be deleted by the reset below
		LOG_L(L_WARNING, "[%s] thread already has ThreadControls installed", __func__);
	} else {
		// new ThreadControls object, so install SIGUSR1 signal handler also
		if (!SetThreadSignalHandler())
			return;
	}

	{
		threadCtls.reset(new Threading::ThreadControls());

		threadCtls->handle = GetCurrentThread();
		threadCtls->thread_id = gettid();
		threadCtls->running.store(true);
	}
	#endif
}


/**
 * @brief ThreadStart Entry point for wrapped pthread. Allows us to register signal handlers specific to that thread, enabling suspend/resume functionality.
 * @param ptr Points to a platform-specific ThreadControls object.
 */
void ThreadStart(
	std::function<void()> taskFunc,
	std::shared_ptr<ThreadControls>* threadCtls,
	ThreadControls* tempCtls
) {
	// install the SIGUSR1 handler
	SetupCurrentThreadControls(localThreadControls);

	if (threadCtls != nullptr)
		*threadCtls = localThreadControls;

	{
		// Lock the thread object so that users can't suspend/resume yet.
		tempCtls->mutSuspend.lock();

		LOG_L(L_DEBUG, "[%s] new thread handle %.4lx", __func__, localThreadControls->handle);

		// We are fully initialized, so notify the condition variable. The
		// thread's parent will unblock in whatever function created this
		// thread.
		tempCtls->condInitialized.notify_all();

		// Ok, the thread should be ready to run its task now, so unlock the suspend mutex!
		tempCtls->mutSuspend.unlock();
	}

	// Run the task function...
	taskFunc();

	// Finish up: change the thread's running state to false.
	localThreadControls->mutSuspend.lock();
	localThreadControls->running = false;
	localThreadControls->mutSuspend.unlock();
}



SuspendResult ThreadControls::Suspend()
{
	// Return an error if the running flag is false.
	if (!running) {
		LOG_L(L_ERROR, "[ThreadControls::%s] cannot suspend if a thread's running flag is set to false, refusing to use pthread_kill", __func__);
		return Threading::THREADERR_NOT_RUNNING;
	}

	mutSuspend.lock();

	LOG_L(L_DEBUG, "[ThreadControls::%s] sending SIGUSR1 to 0x%lx", __func__, handle);

	// Send signal to thread to trigger its handler
	const int err = pthread_kill(handle, SIGUSR1);

	if (err != 0) {
		LOG_L(L_ERROR, "[ThreadControls::%s] error while trying to send signal to suspend thread: %s", __func__, strerror(err));
		return Threading::THREADERR_MISC;
	}

	// Before leaving this function, we need some kind of guarantee that the stalled thread is suspended, so spinwait until it is guaranteed.
	// FIXME: this sort of spin-waiting inside the watchdog loop could be avoided by creating another worker thread
	//        inside SuspendedStacktrace itself to do the work of checking that the stalled thread has been suspended and performing the trace there.
	LinuxThreadState tstate;
	constexpr int max_attempts = 40; // 40 attempts * 0.025s = 1 sec max.
	for (int a = 0; a < max_attempts; a++) {
		if ((tstate = GetLinuxThreadState(thread_id)) == LTS_SLEEP)
			break;
	}

	return Threading::THREADERR_NONE;
}

SuspendResult ThreadControls::Resume()
{
	mutSuspend.unlock();

	return Threading::THREADERR_NONE;
}

}
