/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CRASH_HANDLER_H_
#define _CRASH_HANDLER_H_

#include <string.h>
#include "Threading.h"
#include "System/Log/Level.h"

namespace CrashHandler {
	void Install();
	void Remove();

	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName, const int logLevel = LOG_LEVEL_ERROR);
	void PrepareStacktrace(const int logLevel = LOG_LEVEL_ERROR);
	void CleanupStacktrace(const int logLevel = LOG_LEVEL_ERROR);
	
#ifndef WIN32
	/*
	 * The following method breaks the Stacktrace() interface, but it is unavoidable since we need to provide the
	 *   ucontext_t parameter to thread_unwind (and hence to libunwind) in order to work with the foreign thread.
	 * The ucontext_t structure is embeded within Threading::ThreadControls under Linux.
	 *
	 * Imho this is a better solution than adding yet another optional parameter to the Stacktrace interface because
	 *   the parameter is specific to the needs of one platform.
	 */
	void SuspendedStacktrace(Threading::ThreadControls* ctls, const char* threadName);
#else
	bool InitImageHlpDll();
#endif

	void OutputStacktrace();
}

#endif // _CRASH_HANDLER_H_
