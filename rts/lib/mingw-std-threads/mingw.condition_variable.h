/**
* @file condition_variable.h
* @brief std::condition_variable implementation for MinGW
*
* (c) 2013-2016 by Mega Limited, Auckland, New Zealand
* @author Alexander Vassilev
*
* @copyright Simplified (2-clause) BSD License.
* You should have received a copy of the license along with this
* program.
*
* This code is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* @note
* This file may become part of the mingw-w64 runtime package. If/when this happens,
* the appropriate license will be added, i.e. this code will become dual-licensed,
* and the current BSD 2-clause license will stay.
*/

#ifndef MINGW_CONDITIONAL_VARIABLE_H
#define MINGW_CONDITIONAL_VARIABLE_H
#include <atomic>
#include <assert.h>
#include "mingw.mutex.h"
#include <chrono>
#include <system_error>
#include <windows.h>
#ifdef _GLIBCXX_HAS_GTHREADS
#error This version of MinGW seems to include a win32 port of pthreads, and probably    \
    already has C++11 std threading classes implemented, based on pthreads.             \
    It is likely that you will get errors about redefined classes, and unfortunately    \
    this implementation can not be used standalone and independent of the system <mutex>\
    header, since it relies on it for                                                   \
    std::unique_lock and other utility classes. If you would still like to use this     \
    implementation (as it is more lightweight), you have to edit the                    \
    c++-config.h system header of your MinGW to not define _GLIBCXX_HAS_GTHREADS.       \
    This will prevent system headers from defining actual threading classes while still \
    defining the necessary utility classes.
#endif

namespace std
{

enum class cv_status { no_timeout, timeout };
class condition_variable_any
{
protected:
    recursive_mutex mMutex;
    atomic<int> mNumWaiters;
    HANDLE mWakeEvent;
#pragma pack(push, 1)
    HANDLE mSemaphore;
    HANDLE mTimer;
#pragma pack(pop)
public:
    typedef HANDLE native_handle_type;
    native_handle_type native_handle() {return mSemaphore;}
    condition_variable_any(const condition_variable_any&) = delete;
    condition_variable_any& operator=(const condition_variable_any&) = delete;
    condition_variable_any()
        :mNumWaiters(0), mWakeEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
         mSemaphore(CreateSemaphore(NULL, 0, 0xFFFF, NULL)),
         mTimer(CreateWaitableTimer(NULL, FALSE, NULL))
    {}
    ~condition_variable_any() {  CloseHandle(mWakeEvent); CloseHandle(mSemaphore); CloseHandle(mTimer); }
protected:
    template <class M>
    bool wait_impl(M& lock, DWORD timeout)
    {
        {
            lock_guard<recursive_mutex> guard(mMutex);
            mNumWaiters++;
        }
        lock.unlock();
            DWORD ret = WaitForSingleObject(mSemaphore, timeout);

        mNumWaiters--;
        SetEvent(mWakeEvent);
        lock.lock();
        if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_TIMEOUT)
            return false;
//2 possible cases:
//1)The point in notify_all() where we determine the count to
//increment the semaphore with has not been reached yet:
//we just need to decrement mNumWaiters, but setting the event does not hurt
//
//2)Semaphore has just been released with mNumWaiters just before
//we decremented it. This means that the semaphore count
//after all waiters finish won't be 0 - because not all waiters
//woke up by acquiring the semaphore - we woke up by a timeout.
//The notify_all() must handle this grafecully
//
        else
            throw system_error(EPROTO, generic_category());
    }
//SPRING
    template <class M>
    bool wait_impl_ns(M& lock, DWORD timeout)
    {
        {
            lock_guard<recursive_mutex> guard(mMutex);
            mNumWaiters++;
        }
        lock.unlock();
        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = - int64_t(timeout);
        SetWaitableTimer(
           mTimer,                 // Handle to the timer object.
           &liDueTime,             // When timer will become signaled.
           0,                      // signal once.
           NULL,           // Completion routine.
           NULL,                // Argument to the completion routine.
           FALSE );                // Do not restore a suspended system.
        // Notice that mSemapore and mTimer are contigous in the struct so the following works
        DWORD ret = WaitForMultipleObjects(2, &mSemaphore, FALSE, INFINITE);

        mNumWaiters--;
        SetEvent(mWakeEvent);
        lock.lock();
        if (ret == WAIT_OBJECT_0)
            return true;
        else if (ret == WAIT_TIMEOUT || ret == (WAIT_OBJECT_0 + 1))
            return false;

        else
            throw system_error(EPROTO, generic_category());
    }

public:
    template <class M>
    void wait(M& lock)
    {
        wait_impl(lock, INFINITE);
    }
    template <class M, class Predicate>
    void wait(M& lock, Predicate pred)
    {
        while(!pred())
        {
            wait(lock);
        };
    }

    void notify_all() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex); //block any further wait requests until all current waiters are unblocked
        if (mNumWaiters.load() <= 0)
            return;

        ReleaseSemaphore(mSemaphore, mNumWaiters, NULL);
        while(mNumWaiters > 0)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if ((ret == WAIT_FAILED) || (ret == WAIT_ABANDONED))
                throw system_error(EPROTO, generic_category());
        }
        assert(mNumWaiters == 0);
//in case some of the waiters timed out just after we released the
//semaphore by mNumWaiters, it won't be zero now, because not all waiters
//woke up by acquiring the semaphore. So we must zero the semaphore before
//we accept waiters for the next event
//See _wait_impl for details
        while(WaitForSingleObject(mSemaphore, 0) == WAIT_OBJECT_0);
    }
    void notify_one() noexcept
    {
        lock_guard<recursive_mutex> lock(mMutex);
        if (!mNumWaiters)
            return;
        int targetWaiters = mNumWaiters.load() - 1;
        ReleaseSemaphore(mSemaphore, 1, NULL);
        while(mNumWaiters > targetWaiters)
        {
            auto ret = WaitForSingleObject(mWakeEvent, 1000);
            if ((ret == WAIT_FAILED) || (ret == WAIT_ABANDONED))
                throw system_error(EPROTO, generic_category());
        }
        assert(mNumWaiters == targetWaiters);
    }
    template <class M, class Rep, class Period>
    std::cv_status wait_for(M& lock,
      const std::chrono::duration<Rep, Period>& rel_time)
    {
        long long timeout = chrono::duration_cast<chrono::milliseconds>(rel_time).count();
        if (timeout < 0)
            timeout = 0;
        bool ret = wait_impl(lock, (DWORD)timeout);
        return ret?cv_status::no_timeout:cv_status::timeout;
    }

    template <class M, class Rep>
    std::cv_status wait_for(M& lock,
      const std::chrono::duration<Rep, std::nano>& rel_time)
    {
        int timeout = chrono::duration_cast<chrono::nanoseconds>(rel_time).count() / 100;
        if (timeout < 0)
            timeout = 0;
        bool ret = wait_impl_ns(lock, (DWORD)timeout);
        return ret?cv_status::no_timeout:cv_status::timeout;
    }

    template <class M, class Rep, class Period, class Predicate>
    bool wait_for(M& lock,
       const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        wait_for(lock, rel_time);
        return pred();
    }
    template <class M, class Clock, class Duration>
    cv_status wait_until (M& lock,
      const chrono::time_point<Clock,Duration>& abs_time)
    {
        return wait_for(lock, abs_time - Clock::now());
    }
    template <class M, class Clock, class Duration, class Predicate>
    bool wait_until (M& lock,
      const std::chrono::time_point<Clock, Duration>& abs_time,
      Predicate pred)
    {
        auto time = abs_time - Clock::now();
        if (time < 0)
            return pred();
        else
            return wait_for(lock, time, pred);
    }
};
class condition_variable: protected condition_variable_any
{
protected:
    typedef condition_variable_any base;
public:
    using base::native_handle_type;
    using base::native_handle;
    using base::base;
    using base::notify_all;
    using base::notify_one;
    void wait(unique_lock<mutex> &lock)
    {       base::wait(lock);                               }
    template <class Predicate>
    void wait(unique_lock<mutex>& lock, Predicate pred)
    {       base::wait(lock, pred);                         }
    template <class Rep, class Period>
    std::cv_status wait_for(unique_lock<mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time)
    {      return base::wait_for(lock, rel_time);           }
    template <class Rep, class Period, class Predicate>
    bool wait_for(unique_lock<mutex>& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {        return base::wait_for(lock, rel_time, pred);   }
    template <class Clock, class Duration>
    cv_status wait_until (unique_lock<mutex>& lock, const chrono::time_point<Clock,Duration>& abs_time)
    {        return base::wait_for(lock, abs_time);         }
    template <class Clock, class Duration, class Predicate>
    bool wait_until (unique_lock<mutex>& lock, const std::chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {        return base::wait_until(lock, abs_time, pred); }
};
}
#endif // MINGW_CONDITIONAL_VARIABLE_H
