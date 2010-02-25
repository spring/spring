/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLATFORM_MISC_H
#define PLATFORM_MISC_H

#include <string>

namespace Platform
{

/**
 * Returns the path to the main binary of the running process,
 * including the file name.
 * examples:
 * examples when calling from the engine:
 * - "/usr/games/bin/spring"
 * - "/home/user/spring/spring"
 * - "C:\Program Files\Spring\spring.exe"
 * examples when calling from the synchronization library:
 * - "/usr/games/bin/springlobby"
 * - "/home/user/springlobby/springlobby"
 * - "C:\Program Files\SpringLobby\springlobby.exe"
 * @return path to the main binary of the running process file or ""
 */
std::string GetProcessExecutableFile();

/**
 * Returns the path to the main binary of the running process,
 * excluding the file name.
 * @see GetProcessExecutableFile()
 * @return path to the main binary of the running process dir or ""
 */
std::string GetProcessExecutablePath();

/**
 * Returns the path to the module/shared library, including the file name.
 * If moduleName is "", the path to the current library is returned.
 * examples when calling from the engine:
 * - "/usr/games/bin/spring"
 * - "/home/user/spring/spring"
 * - "C:\Program Files\Spring\spring.exe"
 * examples when calling from the synchronization library:
 * - "/usr/lib/libunitsync.so"
 * - "/home/user/spring/libunitsync.so"
 * - "C:\Program Files\Spring\unitsync.dll"
 * @param  moduleName eg. "spring" or "unitsync", "" for current
 * @return path to the current module file or ""
 */
std::string GetModuleFile(std::string moduleName = "");

/**
 * Returns the path to a module/shared library, excluding the file name.
 * If moduleName is "", the path to the current library is returned.
 * @see GetModuleFile()
 * @param  moduleName eg. "spring" or "unitsync", "" for current
 * @return path to the current module dir or ""
 */
std::string GetModulePath(const std::string& moduleName = "");

std::string GetOS();
bool Is64Bit();
bool Is32BitEmulation();
}

#endif // PLATFORM_MISC_H
