#include "Misc.h"

#ifdef linux
#include <unistd.h>

#elif WIN32
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>
#include "System/Platform/Win/WinVersion.h"

#elif MACOSX_BUNDLE
#include <CoreFoundation/CoreFoundation.h>

#else

#endif

#include "System/LogOutput.h"

/**
 * Utility function for cutting off the file name form a path
 * @param  path "/path/to/my/file.ext"
 * @return "/path/to/my/" or "" if (path == "")
 */
static std::string GetParentPath(const std::string& path)
{
	std::string parentPath = "";

	if (!path.empty()) {
#if defined linux || defined MACOSX_BUNDLE
		const size_t parentPath_length = path.find_last_of('/');
		if (parentPath_length != std::string::npos) {
			parentPath = path.substr(0, parentPath_length);
		}
#elif defined WIN32
		char drive[MAX_PATH], dir[MAX_PATH], file[MAX_PATH], ext[MAX_PATH], parentPathC[MAX_PATH];
		_splitpath(path.c_str(), drive, dir, file, ext);
		_makepath(parentPathC, drive, dir, NULL, NULL);
		parentPath = std::string(parentPathC);
#else
		#error implement this
#endif // linux, WIN32, MACOSX_BUNDLE, else
	}

	return parentPath;
}


namespace Platform
{

std::string GetProcessExecutableFile()
{
	std::string procExeFilePath = "";

#ifdef linux
#ifdef DEBUG
	logOutput.Print("Note: Using the file path of the process executable is bad practise on Linux!");
#endif
	char file[512];
	const int ret = readlink("/proc/self/exe", file, sizeof(file)-1);
	if (ret >= 0) {
		file[ret] = '\0';
		procExeFilePath = std::string(file);
	}
#elif WIN32
	const HANDLE hProcess = ::GetCurrentProcess();

	// fetch
	TCHAR procExeFile[MAX_PATH+1];
	const int ret = ::GetProcessImageFileName(hProcess, procExeFile, sizeof(procExeFile));

	if ((ret != 0) && (ret != sizeof(procExeFile))) {
		procExeFilePath = std::string(procExeFile);
	}
#elif MACOSX_BUNDLE
	char cPath[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();

	CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);

	CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);

	CFStringGetCString(cfStringRef, cPath, sizeof(cPath), kCFStringEncodingASCII);

	CFRelease(mainBundleURL);
	CFRelease(cfStringRef);

	procExeFilePath = std::string(cPath);
#else
	#error implement this
#endif
	if (procExeFilePath.empty()) {
		logOutput.Print("WARNING: Failed to get file path of the process executable");
	}

	return procExeFilePath;
}
std::string GetProcessExecutablePath() {
	return GetParentPath(GetProcessExecutableFile());
}

std::string GetModuleFile()
{
	std::string moduleFilePath = "";

#ifdef linux
	logOutput.Print("WARNING: Using the file path of the module is bad practise on Linux,\n"
		"and in addition to that, also technically non portable.\n"
		"Therefore, we do not support it at all!");
#elif WIN32
	// The NULL handle means: falls back to the current module
	//const HANDLE hModule = GetModuleHandle("unitsync.dll");
	const HANDLE hModule = NULL;

	// fetch
	TCHAR moduleFile[MAX_PATH+1];
	const int ret = ::GetModuleFileName(hModule, moduleFile, sizeof(moduleFile));

	if (ret == 0 || ret == sizeof(moduleFile)) {
		logOutput.Print("WARNING: Failed to get file path of the module");
	} else {
		moduleFilePath = std::string(moduleFile);
	}
#elif MACOSX_BUNDLE
	// TODO
	#warning implement this (or use linux version)
#else
	#warning implement this
#endif

	return moduleFilePath;
}
std::string GetModulePath() {
	return GetParentPath(GetModuleFile());
}

std::string GetSyncLibraryFile()
{
	std::string syncLibFilePath = "";

#if       defined UNITSYNC
	syncLibFilePath = GetModuleFile();
#else  // defined UNITSYNC
#ifdef linux
	logOutput.Print("WARNING: Using the file path of the synchronisation library is bad practise on Linux,\n"
		"and in addition to that, also technically not possible.\n"
		"Therefore, we do not support it at all!");
#elif WIN32
	// we assume the synchronization lib to be in the same path
	// as the engine launcher executable
	std::string syncLibFilePath = GetProcessExecutablePath();
	if (!syncLibFilePath.empty()) {
		syncLibFilePath = syncLibFilePath + "\\unitsync.dll";
	}
#elif MACOSX_BUNDLE
	// TODO
	#warning implement this (or use linux version)
#else
	#warning implement this
#endif // linux, WIN32, MACOSX_BUNDLE, else
#endif // defined UNITSYNC

	return syncLibFilePath;
}
std::string GetSyncLibraryPath() {
	return GetParentPath(GetSyncLibraryFile());
}


// legacy support (next three functions)
std::string GetBinaryFile() {

#ifdef WIN32
	// this is wrong, but that is how it worked before
	return GetModuleFile();
#else
	return GetProcessExecutableFile();
#endif // linux, WIN32, MACOSX_BUNDLE, else
}
std::string GetBinaryPath() {
	return GetParentPath(GetBinaryFile());
}

std::string GetLibraryPath() {
	return GetSyncLibraryPath();
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

