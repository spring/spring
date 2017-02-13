/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTHREADING_H
#define SPRINGTHREADING_H

#define USE_FUTEX

#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>


#if   defined(_WIN32)
	#include "System/Platform/Win/CriticalSection.h"
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	#include "System/Platform/Mac/Signal.h"
#elif !defined(__APPLE__) && defined(USE_FUTEX)
	#include "System/Platform/Linux/Futex.h"
#endif


#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
	#include "System/Platform/Win/Future.h"
#else
	#include <future>
#endif






namespace spring {
#if   defined(_WIN32)
	typedef CriticalSection mutex;
	typedef CriticalSection recursive_mutex;
	typedef win_signal signal;
	typedef std::condition_variable_any condition_variable;
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	typedef std::mutex mutex;
	typedef std::recursive_mutex recursive_mutex;
	typedef mac_signal signal;
	typedef std::condition_variable condition_variable;
#elif !defined(__APPLE__) && defined(USE_FUTEX)
	typedef spring_futex mutex;
	//typedef recursive_futex recursive_mutex;
	typedef std::recursive_mutex recursive_mutex;
	typedef linux_signal signal;
	typedef std::condition_variable_any condition_variable;
#endif

	typedef std::thread thread;
	namespace this_thread { using namespace std::this_thread; };
	typedef std::cv_status cv_status;
	typedef std::condition_variable_any condition_variable_any;


	class spinlock {
	private:
		std::atomic_flag state;

	public:
		spinlock() {
			state.clear();
		}

		void lock()
		{
			while (state.test_and_set(std::memory_order_acquire)) {
				/* busy-wait */
			}
		}
		void unlock()
		{
			state.clear(std::memory_order_release);
		}
	};


	// barrier from http://stackoverflow.com/a/24465624
	//XXX make this based on semaphores? condition_vars only wakeup one thread at a time on notify_all (check wiki)
	class barrier
	{
	private:
		mutex _mutex;
		condition_variable_any _cv;
		std::size_t _count;
	public:
		explicit barrier(std::size_t count) : _count(count) { }
		void wait()
		{
			std::unique_lock<mutex> lock(_mutex);
			if (--_count == 0) {
				_cv.notify_all();
			} else {
				_cv.wait(lock, [this] { return _count == 0; });
			}
		}
	};
}

#endif // SPRINGTHREADING_H
