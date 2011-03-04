/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include <string>
#include "System/Platform/Threading.h"


namespace Watchdog
{
	//! Installs the watchdog thread
	void Install();
	void Uninstall();

	//! Call this to reset the watchdog timer of the current thread
	void ClearTimer(bool disable = false, Threading::NativeThreadId* _threadId = NULL);
	void ClearTimer(const std::string& name, bool disable = false);
	inline void ClearTimer(const char* name, bool disable = false) {
		ClearTimer(std::string(name), disable);
	}

	//! Call these in the threads you want to monitor
	void RegisterThread(std::string name);
	//void DeregisterThread();
}

#endif // _WATCHDOG_H
