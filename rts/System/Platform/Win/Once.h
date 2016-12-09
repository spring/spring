/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ONCE_H
#define ONCE_H

// Adapted/ripped from Boost, original license below:

//  once.hpp
//
//  (C) Copyright 2005-7 Anthony Williams
//  (C) Copyright 2005 John Maddock
//  (C) Copyright 2011-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <cstddef>

namespace spring
{
	struct once_flag;
	namespace once
	{
		struct once_context;

		inline bool enter_once_region(once_flag& flag, once_context& ctx) noexcept;
		inline void commit_once_region(once_flag& flag, once_context& ctx) noexcept;
		inline void rollback_once_region(once_flag& flag, once_context& ctx) noexcept;
	}

	struct once_flag
	{
			constexpr once_flag() noexcept
				: status(0), count(0)
			{}
			long status;
			long count;
	};

#define SPRING_ONCE_INIT once_flag()


#define SPRING_THREAD_INVOKE_RET_VOID std::bind
#define SPRING_THREAD_INVOKE_RET_VOID_CALL ()

	namespace once
	{
		typedef char once_char_type;
		unsigned const once_mutex_name_fixed_length=54;
		unsigned const once_mutex_name_length=once_mutex_name_fixed_length+
			sizeof(void*)*2+sizeof(unsigned long)*2+1;

		template <class I>
		void int_to_string(I p, once_char_type* buf)
		{
			for(unsigned i=0; i < sizeof(I)*2; ++i,++buf)
			{
				once_char_type const a='A';
				*buf = a + static_cast<once_char_type>((p >> (i*4)) & 0x0f);
			}
			*buf = 0;
		}

		inline void name_once_mutex(once_char_type* mutex_name,void* flag_address)
		{
			static const once_char_type fixed_mutex_name[]="Local\\{C15730E2-145C-4c5e-B005-3BC753F42475}-once-flag";

			std::memcpy(mutex_name,fixed_mutex_name,sizeof(fixed_mutex_name));
			once::int_to_string(reinterpret_cast<std::ptrdiff_t>(flag_address),
														mutex_name + once_mutex_name_fixed_length);
			once::int_to_string(GetCurrentProcessId(),
														mutex_name + once_mutex_name_fixed_length + sizeof(void*)*2);
		}

		inline HANDLE open_once_event(once_char_type* mutex_name,void* flag_address)
		{
			if(!*mutex_name)
			{
				name_once_mutex(mutex_name,flag_address);
			}

			return OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE,
				FALSE,
				mutex_name);
		}

		inline HANDLE create_once_event(once_char_type* mutex_name,void* flag_address)
		{
			if(!*mutex_name)
			{
					name_once_mutex(mutex_name,flag_address);
			}

			return CreateEventA(0, TRUE, FALSE, mutex_name);
		}

		struct once_context {
			long const function_complete_flag_value;
			long const running_value;
			bool counted;
			HANDLE event_handle;
			once::once_char_type mutex_name[once_mutex_name_length];
			once_context() :
				function_complete_flag_value(0xc15730e2),
				running_value(0x7f0725e3),
				counted(false),
				event_handle(NULL)
			{
				mutex_name[0]=0;
			}
			~once_context() {
				CloseHandle(event_handle);
			}
		};
		enum once_action {try_, break_, continue_};

		inline bool enter_once_region(once_flag& flag, once_context& ctx) noexcept
		{
			long status=InterlockedCompareExchange(&flag.status,ctx.running_value,0);
			if(!status)
			{
				if(!ctx.event_handle)
				{
					ctx.event_handle=once::open_once_event(ctx.mutex_name,&flag);
				}
				if(ctx.event_handle)
				{
					ResetEvent(ctx.event_handle);
				}
				return true;
			}
			return false;
		}
		inline void commit_once_region(once_flag& flag, once_context& ctx) noexcept
		{
			if(!ctx.counted)
			{
				InterlockedIncrement(&flag.count);
				ctx.counted=true;
			}
			InterlockedExchange(&flag.status,ctx.function_complete_flag_value);
			if(!ctx.event_handle &&
				(InterlockedCompareExchange(&flag.count,0,0)>1))
			{
				ctx.event_handle=once::create_once_event(ctx.mutex_name,&flag);
			}
			if(ctx.event_handle)
			{
				SetEvent(ctx.event_handle);
			}
		}
		inline void rollback_once_region(once_flag& flag, once_context& ctx) noexcept
		{
			InterlockedExchange(&flag.status,0);
			if(!ctx.event_handle)
			{
				ctx.event_handle=once::open_once_event(ctx.mutex_name,&flag);
			}
			if(ctx.event_handle)
			{
				SetEvent(ctx.event_handle);
			}
		}
	}

	inline void call_once(once_flag& flag, void (*f)())
	{
		// Try for a quick win: if the procedure has already been called
		// just skip through:
		once::once_context ctx;
		while(InterlockedCompareExchange(&flag.status,0,0)
				!=ctx.function_complete_flag_value)
		{
			if(once::enter_once_region(flag, ctx))
			{
				f();
				once::commit_once_region(flag, ctx);
				break;
			}
			if(!ctx.counted)
			{
				InterlockedIncrement(&flag.count);
				ctx.counted=true;
				long status=InterlockedCompareExchange(&flag.status,0,0);
				if(status==ctx.function_complete_flag_value)
				{
					break;
				}
				if(!ctx.event_handle)
				{
					ctx.event_handle=once::create_once_event(ctx.mutex_name,&flag);
					continue;
				}
			}
			WaitForSingleObjectEx(ctx.event_handle, INFINITE, 0);
		}
	}
//#endif
	template<typename Function>
	inline void call_once(once_flag& flag, Function&& f)
	{
		// Try for a quick win: if the procedure has already been called
		// just skip through:
		once::once_context ctx;
		while(InterlockedCompareExchange(&flag.status,0,0)
				!=ctx.function_complete_flag_value)
		{
			if(once::enter_once_region(flag, ctx))
			{
				f();
				once::commit_once_region(flag, ctx);
				break;
			}
			if(!ctx.counted)
			{
				InterlockedIncrement(&flag.count);
				ctx.counted=true;
				long status=InterlockedCompareExchange(&flag.status,0,0);
				if(status==ctx.function_complete_flag_value)
				{
					break;
				}
				if(!ctx.event_handle)
				{
					ctx.event_handle=once::create_once_event(ctx.mutex_name,&flag);
					continue;
				}
			}
			WaitForSingleObjectEx(ctx.event_handle,INFINITE,0);
		}
	}
	template<typename Function, class A, class ...ArgTypes>
	inline void call_once(once_flag& flag, Function&& f, A&& a, ArgTypes&&... args)
	{
		// Try for a quick win: if the procedure has already been called
		// just skip through:
		once::once_context ctx;
		while(InterlockedCompareExchange(&flag.status,0,0)
				!=ctx.function_complete_flag_value)
		{
			if(once::enter_once_region(flag, ctx))
			{
				std::bind(
					std::forward<Function>(f),
					std::forward<A>(a),
					std::forward<ArgTypes>(args)...
				)();
				once::commit_once_region(flag, ctx);
				break;
			}
			if(!ctx.counted)
			{
				InterlockedIncrement(&flag.count);
				ctx.counted=true;
				long status=InterlockedCompareExchange(&flag.status,0,0);
				if(status==ctx.function_complete_flag_value)
				{
					break;
				}
				if(!ctx.event_handle)
				{
					ctx.event_handle=once::create_once_event(ctx.mutex_name,&flag);
					continue;
				}
			}
			WaitForSingleObjectEx(ctx.event_handle,INFINITE,0);
		}
	}
}

#endif
