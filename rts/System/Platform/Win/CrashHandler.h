/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CRASH_HANDLER_WIN32_H_
#define _CRASH_HANDLER_WIN32_H_

// don't use directly
namespace CrashHandler {
	namespace Win32 {
		void Install();
		void Remove();
		void InstallHangHandler();
		void UninstallHangHandler();
		void ClearDrawWDT(bool disable);
		void ClearSimWDT(bool disable);
		void GameLoading(bool);
	};

	static volatile bool gameLoading;
};

#endif // !_CRASH_HANDLER_WIN32_H_
