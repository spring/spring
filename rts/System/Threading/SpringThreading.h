/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTHREADING_H
#define SPRINGTHREADING_H

//#define USE_FUTEX

#include <mutex>
#include <atomic>
#if   defined(_WIN32)
	#include "CriticalSection.h"
#elif !defined(__APPLE__) && defined(USE_FUTEX)
	#include "Futex.h"
#endif

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
	#include <mingw-std-threads/mingw.thread.h>
	#include <mingw-std-threads/mingw.condition_variable.h>
#else
	#include <thread>
	#include <condition_variable>
#endif



namespace spring {
#if   defined(_WIN32)
	typedef CriticalSection mutex;
	typedef CriticalSection recursive_mutex;
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	typedef std::mutex mutex;
	typedef std::recursive_mutex recursive_mutex;
#elif !defined(__APPLE__) && defined(USE_FUTEX)
	typedef spring_futex mutex;
	//typedef recursive_futex recursive_mutex;
	typedef std::recursive_mutex recursive_mutex;
#endif

	typedef std::thread thread;
	namespace this_thread { using namespace std::this_thread; };

	typedef std::cv_status cv_status;
	typedef std::condition_variable_any condition_variable_any;
	typedef std::condition_variable condition_variable;


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




	// class shared_spinlock : public boost::shared_mutex {
	// public:
		// void lock() {
			// while (!try_lock()) { /* busy-wait */ }
		// }

		// void lock_shared() {
			// while (!try_lock_shared()) { /* busy-wait */ }
		// }
	// };
}

#endif // SPRINGTHREADING_H
