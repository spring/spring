/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <string>
#include "System/Platform/Threading.h"

// Update Watchdog::threadNames also if adding threads
enum WatchdogThreadnum {
	WDT_MAIN  = 0,
	WDT_LOAD  = 1,
	WDT_AUDIO = 2,
	WDT_VFSI  = 3,
	WDT_COUNT = 4,
};

namespace Watchdog
{
	// Installs the watchdog thread
	void Install();
	void Uninstall();

	// Call this to reset the watchdog timer of the current thread
	void ClearTimer(Threading::NativeThreadId* _threadId = nullptr, bool disable = false);
	void ClearTimer(const WatchdogThreadnum num, bool disable = false);
	void ClearTimer(const char* name, bool disable = false);
	void ClearTimers(bool disable = false, bool primary = false);

	// Call these in the threads you want to monitor
	void RegisterThread(WatchdogThreadnum num, bool primary = false);
	bool DeregisterThread(WatchdogThreadnum num);
	bool DeregisterCurrentThread();
	bool HasThread(WatchdogThreadnum num);
}

#endif // _WATCHDOG_H
