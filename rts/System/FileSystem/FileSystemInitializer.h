/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_INITIALIZER_H
#define FILE_SYSTEM_INITIALIZER_H

#include <string>

class FileSystemInitializer {
public:
	/// call in defined order!
	static void PreInitializeConfigHandler(const std::string& configSource = "", const bool safemode = false);
	static void InitializeLogOutput(const std::string& filename = "");
	static bool Initialize();
	static void InitializeThr(bool* retPtr) { *retPtr = Initialize(); }
	static void Cleanup(bool deallocConfigHandler = true);

	// either result counts
	static bool Initialized() { return (initSuccess || initFailure); }

private:
	static volatile bool initSuccess;
	static volatile bool initFailure;
};

#endif // FILE_SYSTEM_INITIALIZER_H
