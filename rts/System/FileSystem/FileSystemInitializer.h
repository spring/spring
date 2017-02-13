/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_INITIALIZER_H
#define FILE_SYSTEM_INITIALIZER_H

#include <string>

class FileSystemInitializer {
public:
	/// call in defined order!
	static void PreInitializeConfigHandler(const std::string& configSource = "", const bool safemode = false);
	static void InitializeLogOutput(const std::string& filename = "");
	static void Initialize();
	static void Cleanup(bool deallocConfigHandler = true);

private:
	static bool initialized;
};

#endif // FILE_SYSTEM_INITIALIZER_H
