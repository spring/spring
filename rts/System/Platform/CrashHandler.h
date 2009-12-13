#ifndef CRASHHANDLER
#define CRASHHANDLER

namespace CrashHandler {
	void Install();
	void Remove();
	void InstallHangHandler();
	void UninstallHangHandler();
	void ClearDrawWDT(bool disable = false);
	void ClearSimWDT(bool disable = false);
};

#endif // CRASHHANDLER
