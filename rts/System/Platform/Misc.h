/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PLATFORM_MISC_H
#define PLATFORM_MISC_H

#include <string>
#include <array>

namespace Platform
{
	/**
	 * Returns the path to the original PWD/CWD dir from where the user started the execution.
	 * For security reasons we change the CWD to Spring's write-dir (most of the time it's the home-dir), so we need to cache the PWD/CWD.
	 * @return path to the CWD, with trailing path separator
	 */
	const std::string& GetOrigCWD();
	const std::string& SetOrigCWD();

	/**
	 * Returns the path to the current users dir for storing application settings.
	 * examples:
	 * - "/home/pablo"
	 * - "/root"
	 * - "C:\Users\USER\AppData\Local"
	 * - "C:\Users\USER\Local Settings\Application Data"
	 * - "C:\Documents and Settings\USER\Local Settings\App Data"
	 * - "C:\Dokumente und Einstellungen\USER\Anwendungsdaten"
	 * @return path to the current users home dir, without trailing path separator
	 */
	std::string GetUserDir();

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
	 * @return path to the current module dir (with trailing path separator) or ""
	 */
	std::string GetModulePath(const std::string& moduleName = "");

	std::string GetOSNameStr();
	std::string GetOSVersionStr();
	std::string GetOSDisplayStr(); // name + version
	std::string GetOSFamilyStr();
	std::string GetWordSizeStr();
	std::string GetPlatformStr(); // OS family + wordsize
	std::string GetHardwareStr(); // processor + memory
	std::string GetSysInfoHash();
	std::string GetMacAddrHash();

	bool Is64Bit();
	bool Is32BitEmulation();
	bool IsRunningInGDB();

	uint64_t FreeDiskSpace(const std::string& path);
	uint32_t NativeWordSize(); // compiled process code
	uint32_t SystemWordSize(); // host operating system


	/**
	 * Executes a native binary, file and args have to be not escaped!
	 * http://linux.die.net/man/3/execvp
	 * @param  args[0  ] path to an executable, eg. "/usr/bin/games/spring"
	 * @param  args[1:n] arguments to the executable, eg. {"-f", "/tmp/test.txt"}
	 * @return error message, or "" on success
	 */
	const char* ExecuteProcess(std::array<std::string, 32>& args, bool asSubprocess = false);


	std::array<uint8_t,  6> GetRawMacAddr();
	std::array<   char, 18> GetHexMacAddr();
}

#endif // PLATFORM_MISC_H
