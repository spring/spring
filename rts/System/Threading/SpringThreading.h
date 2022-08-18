/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTHREADING_H
#define SPRINGTHREADING_H

#define USE_FUTEX

#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <immintrin.h>


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

	class noop_mutex {
	public:
		using native_handle_type = spring::mutex::native_handle_type;

		noop_mutex() noexcept {}
		~noop_mutex() {}

		noop_mutex(const noop_mutex&) = delete;
		noop_mutex& operator=(const noop_mutex&) = delete;

		void lock() {}
		bool try_lock() { return true; }
		void unlock() {}

		native_handle_type native_handle() { return native_handle_type{}; }
	};

	class mutex_wrapper_concept {
	public:
		using native_handle_type = spring::mutex::native_handle_type;

		mutex_wrapper_concept() noexcept {}
		virtual ~mutex_wrapper_concept() {}

		mutex_wrapper_concept(const mutex_wrapper_concept&) = delete;
		mutex_wrapper_concept& operator=(const mutex_wrapper_concept&) = delete;

		virtual void lock() = 0;
		virtual bool try_lock() noexcept = 0;
		virtual void unlock() = 0;

		virtual native_handle_type native_handle() = 0;
	};

	template<typename M>
	class mutex_wrapper : public mutex_wrapper_concept {
		static_assert(std::is_member_function_pointer<decltype(&M::lock)>::value);
		static_assert(std::is_member_function_pointer<decltype(&M::try_lock)>::value);
		static_assert(std::is_member_function_pointer<decltype(&M::unlock)>::value);
		static_assert(std::is_member_function_pointer<decltype(&M::native_handle)>::value);
	public:
		mutex_wrapper() noexcept {};

		mutex_wrapper(const mutex_wrapper&) = delete;
		mutex_wrapper& operator=(const mutex_wrapper&) = delete;

		void lock() override { mut.lock(); }
		bool try_lock() noexcept override { return mut.try_lock(); }
		void unlock() override { mut.unlock(); }

		virtual native_handle_type native_handle() override { return mut.native_handle(); }
	protected:
		M mut;
	};


	// updated as per https://rigtorp.se/spinlock/
	class spinlock {
	private:
		std::atomic<bool> state = {false};

	public:
		void lock()
		{
			// changed implementation from a test-and-set (TAS) to test and test-and-set (TTAS)
			// this reduces cache coherency traffic on the processor
			for(;;){
				if (!state.exchange(true, std::memory_order_acquire)) {
					break;
				}
				while (state.load(std::memory_order_relaxed)) {
					// add a pause to allow yielding to an SMT/HT thread on the same core
					_mm_pause();
				}
			}
		}
		void unlock()
		{
			state.store(false, std::memory_order_release);
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
