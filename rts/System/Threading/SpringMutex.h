/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGMUTEX_H
#define SPRINGMUTEX_H

//#define USE_FUTEX

#include <atomic>
#if   defined(_WIN32)
	#include "CriticalSection.h"
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	#include <mutex>
#else
	#include "Futex.h"
	#include <mutex>
#endif
#include <boost/thread/shared_mutex.hpp>



namespace spring {
#if   defined(_WIN32)
	typedef CriticalSection mutex;
	typedef CriticalSection recursive_mutex;
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	typedef std::mutex mutex;
	typedef std::recursive_mutex recursive_mutex;
#else
	typedef spring_futex mutex;
	//typedef recursive_futex recursive_mutex;
	typedef std::recursive_mutex recursive_mutex;
#endif


	class spinlock {
	private:
		std::atomic_flag state;

	public:
		spinlock() : state(ATOMIC_FLAG_INIT) {}

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


	class shared_spinlock : public boost::shared_mutex {
	public:
		void lock() {
			while (!try_lock()) { /* busy-wait */ }
		}

		void lock_shared() {
			while (!try_lock_shared()) { /* busy-wait */ }
		}
	};
}

#endif // SPRINGMUTEX_H
