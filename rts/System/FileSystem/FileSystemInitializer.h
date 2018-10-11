/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_INITIALIZER_H
#define FILE_SYSTEM_INITIALIZER_H

#include <atomic>
#include <string>

class FileSystemInitializer {
public:
	/// call in defined order!
	static void PreInitializeConfigHandler(const std::string& configSource = "", const std::string& configName = "", const bool safemode = false);
	static void InitializeLogOutput(const std::string& filename = "");
	static bool Initialize();
	static void InitializeThr(bool* retPtr) { *retPtr = Initialize(); }
	static void Cleanup(bool deallocConfigHandler = true);
	static void Reload();

	// either result counts
	static bool Initialized() { return (initSuccess || initFailure); }

private:
	static std::atomic<bool> initSuccess;
	static std::atomic<bool> initFailure;
};

#endif // FILE_SYSTEM_INITIALIZER_H
