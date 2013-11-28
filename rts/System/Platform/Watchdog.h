/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <string>
#include "System/Platform/Threading.h"

// Update Watchdog::threadNames also if adding threads
enum WatchdogThreadnum {
	WDT_MAIN  = 0,
	WDT_SIM   = 1,
	WDT_LOAD  = 2,
	WDT_AUDIO = 3,
	WDT_COUNT = 4,
};

namespace Watchdog
{
	//! Installs the watchdog thread
	void Install();
	void Uninstall();

	//! Call this to reset the watchdog timer of the current thread
	void ClearTimer(bool disable = false, Threading::NativeThreadId* _threadId = NULL);
	void ClearTimer(const WatchdogThreadnum num, bool disable = false);
	void ClearTimer(const std::string& name, bool disable = false);
	inline void ClearTimer(const char* name, bool disable = false) {
		ClearTimer(std::string(name), disable);
	}
	void ClearPrimaryTimers(bool disable = false);

	//! Call these in the threads you want to monitor
	void RegisterThread(WatchdogThreadnum num, bool primary = false);
	void DeregisterThread(WatchdogThreadnum num);
}

#endif // _WATCHDOG_H
