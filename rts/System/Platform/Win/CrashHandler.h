/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

// don't use directly
namespace CrashHandler {
	namespace Win32 {
		void Install();
		void Remove();
		void InstallHangHandler();
		void UninstallHangHandler();
		void ClearDrawWDT(bool disable);
		void ClearSimWDT(bool disable);
	};
};

#endif // !CRASHHANDLER_H
