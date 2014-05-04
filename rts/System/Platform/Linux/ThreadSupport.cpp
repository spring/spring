#include <pthread.h>
#include <signal.h>
#include <ucontext.h>
#include <boost/thread/tss.hpp>
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include <iostream>

static pthread_key_t  threadMutexKey = 0;
static pthread_key_t  ucontextKey = 0;
static pthread_once_t threadInitKey = PTHREAD_ONCE_INIT;

namespace Threading {

void ThreadSIGUSR1Handler (int signum, siginfo_t* info, void* pCtx)
{ // Signal handler, so don't use LOG or anything that might write directly to disk. Just use perror().
    int err=0;

    LOG("ThreadSIGUSR1Handler caught signal!");

    // ucontext_t is passed in by OS.
    ucontext_t* pContext = reinterpret_cast<ucontext_t*> (pCtx);
    assert(threadObjectKey != 0);

    // Thread object should be in thread-local storage
    assert(threadCtls->get() != nullptr);
    assert(threadCtls->get()->get() != nullptr);

    std::shared_ptr<Threading::ThreadControls> pThreadCtls = *threadCtls;

    // Fill in ucontext_t structure before locking, this allows stack walking...
    if (err = getcontext(&pThreadCtls->ucontext)) {
        LOG_L(L_ERROR, "Couldn't get thread context within suspend signal handler: %s", strerror(err));
        return;
    }

    // Change the "running" flag to false. Note that we don't own a lock on the suspend mutex, but in order to get here,
    //   it had to have been locked by some other thread.
    pThreadCtls->running = false;

    // Try to acquire the suspend/resume mutex; this will block the signal handler and the corresponding thread.
    {
        pThreadCtls->mutSuspend.lock();

        pThreadCtls->mutSuspend.unlock();
    }

    pThreadCtls->running = true;
}



void SetCurrentThreadControls(std::shared_ptr<ThreadControls> * ppThreadCtls)
{
    assert(ppThreadCtls != nullptr);
    auto pThreadCtls = ppThreadCtls->get();
    assert(pThreadCtls != nullptr);
    if (threadCtls.get() != nullptr) { // Generally, you only need to set this once per thread. The shared pointer object is deleted at the thread's end by the TLS framework.
        LOG_L(L_WARNING, "Setting a ThreadControls object on a thread that already has such an object registered.");
        auto oldPtr = threadCtls.get();
        threadCtls.reset();
        delete oldPtr;
    } else {
        // Installing new ThreadControls object, so install signal handler also
        int err = 0;
        sigset_t sigSet;
        sigemptyset(&sigSet);
        sigaddset(&sigSet, SIGUSR1);

        if (err = pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL)) {
            LOG_L(L_FATAL, "Error while setting new pthread's signal mask: %s", strerror(err));
            return;
        }

        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_sigaction = ThreadSIGUSR1Handler;
        sa.sa_flags |= SA_SIGINFO;
        if (sigaction(SIGUSR1, &sa, NULL)) {
            LOG_L(L_FATAL,"Error while installing pthread SIGUSR1 handler.");
            return;
        }
    }

    pThreadCtls->handle = GetCurrentThread();
    pThreadCtls->running = true;

    threadCtls.reset(ppThreadCtls);
}

/**
 * @brief ThreadStart Entry point for wrapped pthread. Allows us to register signal handlers specific to that thread, enabling suspend/resume functionality.
 * @param ptr Points to a platform-specific ThreadData object.
 */
void ThreadStart (boost::function<void()> taskFunc, std::shared_ptr<ThreadControls> * ppThreadCtls)
{
    assert(ppThreadCtls != nullptr);
    auto pThreadCtls = ppThreadCtls->get();
    assert(pThreadCtls != nullptr);

    // Lock the thread object so that users can't suspend/resume yet.
    {
        pThreadCtls->mutSuspend.lock();
        pThreadCtls->running = true;

        // Install the SIGUSR1 handler:
        SetCurrentThreadControls(ppThreadCtls);

        LOG("New thread's handle is %.4x");

        // We are fully initialized, so notify the condition variable. The thread's parent will unblock in whatever function created this thread.
        pThreadCtls->condInitialized.notify_all();

        // Ok, the thread should be ready to run its task now, so unlock the suspend mutex!
        pThreadCtls->mutSuspend.unlock();

        // Run the task function...
        taskFunc();
    }

    // Finish up: change the thread's running state to false.
    {
        pThreadCtls->mutSuspend.lock();
        pThreadCtls->running = false;
        pThreadCtls->mutSuspend.unlock();
    }

}


SuspendResult ThreadControls::Suspend ()
{
    int err=0;

    mutSuspend.lock();

    // Return an error if the running flag is false.
    if (!running) {
        LOG("Weird, thread's running flag is set to false. Refusing to suspend using pthread_kill.");
        mutSuspend.unlock();
        return Threading::THREADERR_NOT_RUNNING;
    }

    LOG("Sending SIGUSR1 to %.4x", handle);

    // Send signal to thread to trigger its handler
    if (err = pthread_kill(handle, SIGUSR1)) {
        LOG_L(L_ERROR, "Error while trying to send signal to suspend thread: %s", strerror(err));
        return Threading::THREADERR_MISC;
    }

    return Threading::THREADERR_NONE;
}

SuspendResult ThreadControls::Resume ()
{
    mutSuspend.unlock();

    return Threading::THREADERR_NONE;
}

}
