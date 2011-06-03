/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CRASH_HANDLER_H_
#define _CRASH_HANDLER_H_

#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "Threading.h"

namespace CrashHandler {
	void Install();
	void Remove();

	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName);
	void PrepareStacktrace();
	void CleanupStacktrace();

#ifdef WIN32
	//! used by seh
	LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e);
#endif
	void OutputStacktrace();
};

#endif // _CRASH_HANDLER_H_
