/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if !defined(__APPLE__) || (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_4)
// ### Unix(compliant) CrashHandler START

#include <AvailabilityMacros.h>

//! Same as Linux
#include "System/Platform/Linux/CrashHandler.cpp"

// ### Unix(compliant) CrashHandler END
#else
// ### Fallback CrashHandler (old Apple) START

namespace CrashHandler {
	void HandleSignal(int signal) {}
	void Install() {}
	void Remove() {}
	void InstallHangHandler() {}
	void UninstallHangHandler() {}
	void ClearDrawWDT(bool disable) {}
	void ClearSimWDT(bool disable) {}
	void GameLoading(bool) {}
};

// ### Fallback CrashHandler (old Apple) END
#endif
