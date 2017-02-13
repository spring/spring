/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FUTURE_H
#define FUTURE_H


////////////////
// UGLY HACK
////////////////

// This hack allows the use of the standard library's future
// the code below is copied directly from GNU ISO C++ Library
// it is available under the following license:

// We include thread before the test to make sure that _GLIBCXX_HAS_GTHREADS is/isn't defined
#include <thread>

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
	#include <mutex>
	#include <atomic>
	#include <condition_variable>

	#include <mingw.thread.h>
	#include <mingw.condition_variable.h>
	#include <mingw.mutex.h>
	#include "Once.h"

	namespace std {
		using spring::once_flag;
		using spring::call_once;
	}

	#define _GLIBCXX_HAS_GTHREADS
	#include <future>
	#undef _GLIBCXX_HAS_GTHREADS
#else
	#include <future>
#endif


#endif // FUTURE_H
