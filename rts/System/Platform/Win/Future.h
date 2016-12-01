/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FUTURE_H
#define FUTURE_H


////////////////
// UGLY HACK
////////////////

// This hack allows the use of the standard library's future
// the code below is copied directly from GNU ISO C++ Library
// it is available under the following license:

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
	#include <mingw.thread.h>
	#include <mingw.condition_variable.h>
	#include <mingw.mutex.h>

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
#else
	#include <future>
#endif


#endif // FUTURE_H
