/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

#if defined(__APPLE__) 
// ### Unix(compliant) CrashHandler START

#include <AvailabilityMacros.h>

//! Same as Linux
#include "System/Platform/Linux/CrashHandler.cpp"

// ### Unix(compliant) CrashHandler END
#else
// ### Fallback CrashHandler (old Apple) START

namespace CrashHandler {
	void Install() {}
	void Remove() {}

	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName) {}
	void PrepareStacktrace() {}
	void CleanupStacktrace() {}

	void OutputStacktrace() {}
};

// ### Fallback CrashHandler (old Apple) END
#endif
