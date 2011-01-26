/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CRASH_HANDLER_H_
#define _CRASH_HANDLER_H_

#ifdef WIN32
#include <windows.h>
#endif

namespace CrashHandler {
	void Install();
	void Remove();
	void InstallHangHandler();
	void UninstallHangHandler();
	void ClearDrawWDT(bool disable = false);
	void ClearSimWDT(bool disable = false);
	void GameLoading(bool);
#ifdef WIN32
	LONG CALLBACK ExceptionHandler(LPEXCEPTION_POINTERS e);
#endif
};

#endif // _CRASH_HANDLER_H_
