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

#include <thread>
#include <condition_variable>
#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
	#include <mingw-std-threads/mingw.thread.h>
	#include <mingw-std-threads/mingw.condition_variable.h>
	#include <mingw-std-threads/mingw.mutex.h>

////////////////
// BEGIN UGLY HACK
////////////////

// This hack allows the use of the standard library's future
// the code below is copied directly from GNU ISO C++ Library
// it is available under the following license:

// Copyright (C) 2003-2014 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.
namespace std {
	/// once_flag
	struct once_flag
	{
	private:
		typedef __gthread_once_t __native_type;
		__native_type  _M_once = __GTHREAD_ONCE_INIT;

	public:
		/// Constructor
		constexpr once_flag() noexcept = default;

		/// Deleted copy constructor
		once_flag(const once_flag&) = delete;
		/// Deleted assignment operator
		once_flag& operator=(const once_flag&) = delete;

		template<typename _Callable, typename... _Args>
			friend void
			call_once(once_flag& __once, _Callable&& __f, _Args&&... __args);
	};

	extern __thread void* __once_callable;
	extern __thread void (*__once_call)();

	template<typename _Callable>
		inline void
		__once_call_impl()
		{
			(*(_Callable*)__once_callable)();
		}

	extern "C" void __once_proxy(void);

	/// call_once
	template<typename _Callable, typename... _Args>
		void
		call_once(once_flag& __once, _Callable&& __f, _Args&&... __args)
		{
			auto __bound_functor = std::__bind_simple(std::forward<_Callable>(__f),
					std::forward<_Args>(__args)...);
			__once_callable = &__bound_functor;
			__once_call = &__once_call_impl<decltype(__bound_functor)>;

			int __e = __gthread_once(&__once._M_once, &__once_proxy);

			if (__e)
				__throw_system_error(__e);
		}
}
	#define _GLIBCXX_HAS_GTHREADS
	#include <future>
	#undef _GLIBCXX_HAS_GTHREADS
////////////////
// END UGLY HACK
////////////////
#else
	#include <future>
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
}

#endif // SPRINGTHREADING_H
