/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "thread_backtrace.h"


#include <vector>
#include <stdint.h>
#include <string.h> // memcpy

#if defined(__FreeBSD__)
#include <pthread_np.h> // pthread_attr_get_np
#endif

#define FP_OFFSET 1 //! in sizeof(void*)

#define MAX(a,b) (a>b)?a:b
#define INSTACK(a)  ((a) >= stack_buffer && (a) <= (stack_buffer + stack_size))

//TODO Our custom thread_backtrace() only works on the mainthread.
//     Use to gdb's libthread_db to get the stacktraces of all threads.
//     note, the stacktrace of non-mainthreads is disabled in CrashHandler.cpp:450 (date: 2012-03)

static uint8_t* TranslateStackAddrToBufferAddr(uint8_t* p, uint8_t* stackbot, uint8_t* stack_buffer)
{
	uintptr_t addr = *(uintptr_t*)(p);
	uintptr_t pos = addr - (uintptr_t)stackbot;
	uint8_t* frame = stack_buffer + pos;
	return frame;
}


static void internal_pthread_backtrace(pthread_t thread, void** buffer, size_t max, size_t* depth, size_t skip)
{
	*depth = 0;

	//! Get information about the stack (pos & size).
	uint8_t* stackbot;
	size_t stack_size; //! in bytes
	size_t guard_size; //! in bytes
#if defined(__FreeBSD__)
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_get_np(thread, &attr);
	pthread_attr_getstack(&attr, (void**)&stackbot, &stack_size);
	pthread_attr_getguardsize(&attr, &guard_size);
	pthread_attr_destroy(&attr);
#elif defined(__APPLE__)
	stackbot = (uint8_t*)pthread_get_stackaddr_np(thread);
	stack_size = pthread_get_stacksize_np(thread);
	guard_size = 0; //FIXME anyway to get that?
#else
	pthread_attr_t attr;
	pthread_getattr_np(thread, &attr);
	pthread_attr_getstack(&attr, (void**)&stackbot, &stack_size);
	pthread_attr_getguardsize(&attr, &guard_size);
	pthread_attr_destroy(&attr);
#endif
	stack_size -= guard_size;

	//! The thread is is still running!!!
	//! So make a buffer copy of the stack, else it gets changed while we reading from it!
	std::vector<uint8_t> sbuffer(stack_size);
	uint8_t* stack_buffer = &sbuffer[0];
	memcpy(stack_buffer, stackbot, stack_size);

	//! It is impossible to get the current frame (the current function pos) on the stack
	//! for other pthreads, so we need to find it ourself. The beneath method is very naive
	//! and just searchs for the starting address that gives us the longest stackdepth.
	unsigned longest_stack = 0;
	unsigned longest_offset = 0;
	int check_area = MAX(0, stack_size - 1024*sizeof(uintptr_t));
	check_area *= 0.8f; //! only check ~20% from the top of the stack
	for (int offset = stack_size - sizeof(uintptr_t); offset >= check_area; offset -= sizeof(uintptr_t)) {
		unsigned stack_depth = 0;
		uint8_t* frame = stack_buffer + offset;
		frame = TranslateStackAddrToBufferAddr(frame, stackbot, stack_buffer);
		uint8_t* last_frame = frame;
		while (INSTACK(frame) && (frame > last_frame)) {
			last_frame = frame;
			frame = TranslateStackAddrToBufferAddr(frame, stackbot, stack_buffer);
			stack_depth++;
		}
		if (stack_depth > longest_stack) {
			longest_stack = stack_depth;
			longest_offset = offset;
		}
	}

	//! Now get the list of function addresses from the stack.
	uint8_t* frame = stack_buffer + longest_offset;
	if(!INSTACK(frame))
		return;
	while (skip--) {
		uint8_t* next = TranslateStackAddrToBufferAddr(frame, stackbot, stack_buffer);
		if(!INSTACK(next))
			return;
		frame = next;
	}
	while (max--) {
		uint8_t* next = TranslateStackAddrToBufferAddr(frame, stackbot, stack_buffer);
		buffer[*depth] = (void*)*((uintptr_t*)frame + FP_OFFSET);
		(*depth)++;
		if(!INSTACK(next))
			return;
		frame = next;
	}
}


int thread_backtrace(pthread_t thread, void** buffer, int size)
{
	size_t num_frames;
	internal_pthread_backtrace(thread, buffer, size, &num_frames, 0);
	while (num_frames >= 1 && buffer[num_frames-1] == NULL) num_frames -= 1;
	return num_frames;
}
