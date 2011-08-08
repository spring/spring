/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FILE_SYSTEM_INITIALIZER_H
#define FILE_SYSTEM_INITIALIZER_H

class FileSystemInitializer {
public:
	static void Initialize();
	static void Cleanup();

private:
	static bool initialized;
};

#endif // FILE_SYSTEM_INITIALIZER_H
