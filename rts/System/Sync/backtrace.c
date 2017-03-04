/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* generic backtrace() implementation shamelessly ripped from GNU libc 6 */

#ifdef __MINGW32__

/* Assume we're being compiled without -fbounding-pointers. */
#define __unbounded
#define BOUNDED_1(PTR) (PTR)

/* We set this in main() to get an approximation to the end of the stack. */
void *stack_end;

/* Taken from sysdeps/generic/frame.h: */
struct layout
{
	void *__unbounded next;
	void *__unbounded return_address;
};

/* Taken from sysdeps/generic/backtrace.c: */

/* Return backtrace of current program state.  Generic version.
   Copyright (C) 1998, 2000, 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1998.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/*
#include <execinfo.h>
#include <signal.h>
#include <frame.h>
#include <sigcontextinfo.h>
#include <bp-checks.h>
#include <ldsodefs.h>
*/

/* This implementation assumes a stack layout that matches the defaults
   used by gcc's `__builtin_frame_address' and `__builtin_return_address'
   (FP is the frame pointer register):

      +-----------------+     +-----------------+
FP -> | previous FP --------> | previous FP ------>...
      |                 |     |                 |
      | return address  |     | return address  |
      +-----------------+     +-----------------+

  */

/* Get some notion of the current stack.  Need not be exactly the top
   of the stack, just something somewhere in the current frame.  */
#ifndef CURRENT_STACK_FRAME
# define CURRENT_STACK_FRAME  ({ char __csf; &__csf; })
#endif

/* By default we assume that the stack grows downward.  */
#ifndef INNER_THAN
# define INNER_THAN <
#endif

/* By default assume the `next' pointer in struct layout points to the
   next struct layout.  */
#ifndef ADVANCE_STACK_FRAME
# define ADVANCE_STACK_FRAME(next) BOUNDED_1 ((struct layout *) (next))
#endif

/* By default, the frame pointer is just what we get from gcc.  */
#ifndef FIRST_FRAME_POINTER
# define FIRST_FRAME_POINTER  __builtin_frame_address (0)
#endif

int backtrace (void **array, int size)
{
	struct layout *current;
	void *__unbounded top_frame;
	void *__unbounded top_stack;
	int cnt = 0;

	top_frame = FIRST_FRAME_POINTER;
	top_stack = CURRENT_STACK_FRAME;

	/* We skip the call to this function, it makes no sense to record it.  */
	current = BOUNDED_1 ((struct layout *) top_frame);
	while (cnt < size)
	{
		if ((void *) current INNER_THAN top_stack
				   || !((void *) current INNER_THAN stack_end))
			/* This means the address is out of range.  Note that for the
			toplevel we see a frame pointer with value NULL which clearly is
			out of range.  */
			break;

		array[cnt++] = current->return_address;

		current = ADVANCE_STACK_FRAME (current->next);
	}

	return cnt;
}

#endif // __MINGW32__

// This part is for MSVC and is (obviously) not from GNU libc
// Might actually work on mingw as well, feel free to test and replace.
#if defined _MSC_VER
int backtrace(void **array, int framesToCapture)
{
	int frames = 0;
#ifdef _M_X64
	frames = CaptureStackBackTrace(framesToSkip, framesToCapture, backTrace, nullptr);
#else
	__try {
		void **frame = 0;
		__asm { mov frame, ebp }
		while (frame && frames < framesToCapture) {
			array[frames] = frame[1];
			frames++;
			frame = (void **) frame[0];
		}
	}
	__except (1 /*EXCEPTION_EXECUTE_HANDLER*/) { }
#endif
	return frames;
}
#endif
