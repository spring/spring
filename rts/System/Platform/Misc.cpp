/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Misc.h"

#ifdef linux
#include <unistd.h>
#include <dlfcn.h> // for dladdr(), dlopen()

#elif WIN32
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "System/Platform/Win/WinVersion.h"

#elif __APPLE__
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <dlfcn.h> // for dladdr(), dlopen()
#include <climits> // for PATH_MAX

#else

#endif

#include "System/LogOutput.h"
#include "FileSystem/FileSystem.h"


#if       defined WIN32
/**
 * Returns a handle to the currently loaded module.
 * Note: requires at least Windows 2000
 * @return handle to the currently loaded module, or NULL if an error occures
 */
static HMODULE GetCurrentModule() {

	HMODULE hModule = NULL;

	// both solutions use the address of this function
	// both found at:
	// http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code/557774

	// Win 2000+ solution
	MEMORY_BASIC_INFORMATION mbi = {0};
	::VirtualQuery((void*)GetCurrentModule, &mbi, sizeof(mbi));
	hModule = reinterpret_cast<HMODULE>(mbi.AllocationBase);

	// Win XP+ solution (cleaner)
	//::GetModuleHandleEx(
	//		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
	//		(LPCTSTR)GetCurrentModule,
	//		&hModule);

	return hModule;
}
#endif // defined WIN32


namespace Platform
{

// Mac OS X:        _NSGetExecutablePath() (man 3 dyld)
// Linux:           readlink /proc/self/exe
// Solaris:         getexecname()
// FreeBSD:         sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
// BSD with procfs: readlink /proc/curproc/file
// Windows:         GetModuleFileName() with hModule = NULL
std::string GetProcessExecutableFile()
{
	std::string procExeFilePath = "";
	// will only be used if procExeFilePath stays empty
	const char* error = "Fetch not implemented";

#ifdef linux
	char file[512];
	const int ret = readlink("/proc/self/exe", file, sizeof(file)-1);
	if (ret >= 0) {
		file[ret] = '\0';
		procExeFilePath = std::string(file);
	} else {
		error = "Failed to read /proc/self/exe";
	}
#elif WIN32
	// with NULL, it will return the handle
	// for the main executable of the process
	const HMODULE hModule = GetModuleHandle(NULL);

	// fetch
	TCHAR procExeFile[MAX_PATH+1];
	const int ret = ::GetModuleFileName(hModule, procExeFile, sizeof(procExeFile));

	if ((ret != 0) && (ret != sizeof(procExeFile))) {
		procExeFilePath = std::string(procExeFile);
	} else {
		error = "Unknown";
	}

#elif __APPLE__
	uint32_t pathlen = PATH_MAX;
	char path[PATH_MAX];
	int err = _NSGetExecutablePath(path, &pathlen);
	if (err == 0)
	{
		char pathReal[PATH_MAX];
		realpath(path, pathReal);
		procExeFilePath = std::string(pathReal);
	}
#else
	#error implement this
#endif

	if (procExeFilePath.empty()) {
		logOutput.Print("WARNING: Failed to get file path of the process executable, reason: %s", error);
	}

	return procExeFilePath;
}

std::string GetProcessExecutablePath()
{
	return filesystem.GetDirectory(GetProcessExecutableFile());
}

std::string GetModuleFile(std::string moduleName)
{
	std::string moduleFilePath = "";
	// will only be used if moduleFilePath stays empty
	const char* error = "Fetch not implemented";

#if defined(linux) || defined(__APPLE__)
#ifdef __APPLE__
	#define SHARED_LIBRARY_EXTENSION "dylib"
#else
	#define SHARED_LIBRARY_EXTENSION "so"
#endif
	void* moduleAddress = NULL;

	// find an address in the module we are looking for
	if (moduleName.empty()) {
		// look for current module
		moduleAddress = (void*) GetModuleFile;
	} else {
		// look for specified module

		// add extension if it is not in the file name
		// it could also be "libXZY.so-1.2.3"
		// -> does not have to be the end, my friend
		if (moduleName.find("."SHARED_LIBRARY_EXTENSION) == std::string::npos) {
			moduleName = moduleName + "."SHARED_LIBRARY_EXTENSION;
		}

		// will not not try to load, but return the libs address
		// if it is already loaded, NULL otherwise
		moduleAddress = dlopen(moduleName.c_str(), RTLD_LAZY | RTLD_NOLOAD);

		if (moduleAddress == NULL) {
			// if not found, try with "lib" prefix
			moduleName = "lib" + moduleName;
			moduleAddress = dlopen(moduleName.c_str(), RTLD_LAZY | RTLD_NOLOAD);
		}
	}

	if (moduleAddress != NULL) {
		// fetch info about the module containing the address we just evaluated
		Dl_info moduleInfo;
		const int ret = dladdr(moduleAddress, &moduleInfo);
		if ((ret != 0) && (moduleInfo.dli_fname != NULL)) {
			moduleFilePath = moduleInfo.dli_fname;
		} else {
			error = dlerror();
			if (error == NULL) {
				error = "Unknown";
			}
		}
	} else {
		error = "Not loaded";
	}

#elif WIN32
	HMODULE hModule = NULL;
	if (moduleName.empty()) {
		hModule = GetCurrentModule();
	} else {
		// If this fails, we get a NULL handle
		hModule = GetModuleHandle(moduleName.c_str());
	}

	if (hModule != NULL) {
		// fetch module file name
		TCHAR moduleFile[MAX_PATH+1];
		const int ret = ::GetModuleFileName(hModule, moduleFile, sizeof(moduleFile));

		if ((ret != 0) && (ret != sizeof(moduleFile))) {
			moduleFilePath = std::string(moduleFile);
		} else {
			error = + "Unknown";
		}
	} else {
		error = "Not found";
	}
#else
	#warning implement this (or use linux version)
#endif

	if (moduleFilePath.empty()) {
		if (moduleName.empty()) {
			moduleName = "<current>";
		}
		logOutput.Print("WARNING: Failed to get file path of the module \"%s\", reason: %s", moduleName.c_str(), error);
	}

	return moduleFilePath;
}
std::string GetModulePath(const std::string& moduleName)
{
	return filesystem.GetDirectory(GetModuleFile(moduleName));
}

std::string GetOS()
{
#if defined(WIN32)
	return "Microsoft Windows\n" +
		GetOSDisplayString() + "\n" +
		GetHardwareInfoString();
#elif defined(__linux__)
	return "Linux";
#elif defined(__FreeBSD__)
	return "FreeBSD";
#elif defined(__APPLE__)
	return "Mac OS X";
#else
	#warning improve this
	return "unknown OS";
#endif
}

bool Is64Bit()
{
	return (sizeof(void*) == 8);
}

#ifdef WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

/** @brief checks if the current process is running in 32bit emulation mode
    @return FALSE, TRUE, -1 on error (usually no permissions) */
bool Is32BitEmulation()
{
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
		{
			return false;
		}
	}
	return bIsWow64;
}
#else
// simply assume other OS doesn't need 32bit emulation
bool Is32BitEmulation()
{
	return false;
}
#endif

}

