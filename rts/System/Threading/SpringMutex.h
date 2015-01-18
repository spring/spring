/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGMUTEX_H
#define SPRINGMUTEX_H

#define USE_FUTEX

#if   defined(_WIN32)
	#include "CritSection.h"
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	#include <mutex>
#else
	#include "Futex.h"
	#include <mutex>
#endif


namespace spring {
#if   defined(_WIN32)
	typedef critsection mutex;
	typedef critsection recursive_mutex;
#elif defined(__APPLE__) || !defined(USE_FUTEX)
	typedef std::mutex mutex;
	typedef std::recursive_mutex recursive_mutex;
#else
	typedef spring_futex mutex;
	//typedef recursive_futex recursive_mutex;
	typedef std::recursive_mutex recursive_mutex;
#endif
}

#endif // SPRINGMUTEX_H
