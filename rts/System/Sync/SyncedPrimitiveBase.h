/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCED_PRIMITIVE_BASSE_H
#define SYNCED_PRIMITIVE_BASSE_H

#ifdef SYNCCHECK
	#include "SyncChecker.h"
#endif

#ifdef SYNCDEBUG
	#include "SyncDebugger.h"
#endif

#include <assert.h>


// NOTE: lowercase sync clashes with extern void sync(...) from unistd.h
namespace Sync {

	static inline void AssertDebugger(const void* p, unsigned size, const char* msg) {
	#ifdef SYNCDEBUG
		CSyncDebugger::GetInstance()->Sync(p, size, msg);
	#endif
	}

	/**
	 * @brief Check sync of the argument x.
	 */
	template<typename T>
	static inline void AssertDebugger(const T& x, const char* msg = "assert") {
		AssertDebugger(&x, sizeof(T), msg);
	}


	/**
	 * @brief Assert a contiguous memory area is still synchronized.
	 * @param p    Start of the memory area
	 * @param size Size of the memory area
	 * @param msg  An arbitrary debugging text (preferably short)
	 *
	 * (A checksum of) the memory may be passed to either the CSyncDebugger,
	 * or the CSyncChecker, depending on the enabled @code #defines @endcode.
	 */
	static inline void Assert(const void* p, unsigned size, const char* msg) {
		AssertDebugger(p, size, msg);
#ifdef SYNCCHECK
		assert(CSyncChecker::InSyncedCode());
		CSyncChecker::Sync(p, size);
	#ifdef TRACE_SYNC_HEAVY
		tracefile << "Sync " << msg << " " << CSyncChecker::GetChecksum() << "\n";
	#endif
#endif
	}

	/**
	 * @brief Check sync of the argument x.
	 */
	template<typename T>
	static inline void Assert(const T& x, const char* msg = "assert") {
		Assert(&x, sizeof(T), msg);
	}

}

#if !defined(NDEBUG) && defined(SYNCCHECK)
#  define ENTER_SYNCED_CODE() CSyncChecker::EnterSyncedCode()
#  define LEAVE_SYNCED_CODE() CSyncChecker::LeaveSyncedCode()
#else
#  define ENTER_SYNCED_CODE()
#  define LEAVE_SYNCED_CODE()
#endif

#ifdef SYNCDEBUG
#  define ASSERT_SYNCED(x) Sync::AssertDebugger(x, "assert(" #x ")")
#else
#  define ASSERT_SYNCED(x)
#endif

#endif
