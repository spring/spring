/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Misc.h"

#ifdef __linux__
#include <unistd.h>
#include <dlfcn.h> // for dladdr(), dlopen()

#elif WIN32
#include <io.h>
#include <direct.h>
#include <process.h>
#include <shlobj.h>
#include <shlwapi.h>
#ifndef SHGFP_TYPE_CURRENT
	#define SHGFP_TYPE_CURRENT 0
#endif
#include "System/Platform/Win/WinVersion.h"

#elif __APPLE__
#include <unistd.h>
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <dlfcn.h> // for dladdr(), dlopen()
#include <climits> // for PATH_MAX

#elif defined __FreeBSD__
#include <unistd.h>
#include <dlfcn.h> // for dladdr(), dlopen()
#include <sys/types.h>
#include <sys/sysctl.h>

#else

#endif

#if !defined(WIN32)
#include <sys/utsname.h> // for uname()
#include <sys/types.h> // for getpw
#include <pwd.h> // for getpw

#include <fstream>
#endif

#include <cstring>
#include <cerrno>

#include "System/Util.h"
#include "System/SafeCStrings.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileSystem.h"

#if       defined WIN32
/**
 * Returns a handle to the currently loaded module.
 * Note: requires at least Windows 2000
 * @return handle to the currently loaded module, or NULL if an error occures
 */
static HMODULE GetCurrentModule()
{
	// both solutions use the address of this function
	// both found at:
	// http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code/557774

	// Win 2000+ solution
	MEMORY_BASIC_INFORMATION mbi = {0};
	::VirtualQuery((void*)GetCurrentModule, &mbi, sizeof(mbi));
	HMODULE hModule = reinterpret_cast<HMODULE>(mbi.AllocationBase);

	// Win XP+ solution (cleaner)
	//::GetModuleHandleEx(
	//		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
	//		(LPCTSTR)GetCurrentModule,
	//		&hModule);

	return hModule;
}
#endif // defined WIN32

/**
 * The user might want to define the user dir manually,
 * to locate spring related data in a non-default location.
 * @link http://en.wikipedia.org/wiki/Environment_variable#Synopsis
 */
static std::string GetUserDirFromEnvVar()
{
#ifdef _WIN32
	#define HOME_ENV_VAR "LOCALAPPDATA"
#else
	#define HOME_ENV_VAR "HOME"
#endif
	char* home = getenv(HOME_ENV_VAR);
#undef HOME_ENV_VAR

	return (home == NULL) ? "" : home;
}

static std::string GetUserDirFromSystemApi()
{
#ifdef _WIN32
	TCHAR strPath[MAX_PATH + 1];
	SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath);
	return strPath;
#else
	struct passwd* pw = getpwuid(getuid());
	return pw->pw_dir;
#endif
}


namespace Platform
{

static std::string origCWD;


std::string GetOrigCWD()
{
	return origCWD;
}


void SetOrigCWD()
{
	if (!origCWD.empty())
		return;

	char *buf;
#ifdef WIN32
	buf = _getcwd(NULL, 0);
#else
	buf = getcwd(NULL, 0);
#endif
	origCWD = buf;
	free(buf);
	FileSystemAbstraction::EnsurePathSepAtEnd(origCWD);
}


std::string GetUserDir()
{
	std::string userDir = GetUserDirFromEnvVar();

	if (userDir.empty()) {
		// In some cases, the env var is not set,
		// for example for non-human user accounts,
		// or when starting through the UI on OS X.
		// It is unset by default on windows.
		userDir = GetUserDirFromSystemApi();
	}

	return userDir;
}

#ifndef WIN32
static std::string GetRealPath(const std::string& path) {

	std::string pathReal = path;

	// using NULL here is not supported in very old systems,
	// but should be no problem for spring
	// see for older systems:
	// http://stackoverflow.com/questions/4109638/what-is-the-safe-alternative-to-realpath
	char* pathRealC = realpath(path.c_str(), NULL);
	if (pathRealC != NULL) {
		pathReal = pathRealC;
		free(pathRealC);
		pathRealC = NULL;
	}

	if (FileSystem::GetDirectory(pathReal).empty()) {
		pathReal = GetProcessExecutablePath() + pathReal;
	}

	return pathReal;
}
#endif

// Mac OS X:        _NSGetExecutablePath() (man 3 dyld)
// Linux:           readlink /proc/self/exe
// Solaris:         getexecname()
// FreeBSD:         sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
// BSD with procfs: readlink /proc/curproc/file
// Windows:         GetModuleFileName() with hModule = NULL
std::string GetProcessExecutableFile()
{
	std::string procExeFilePath = "";
	// error will only be used if procExeFilePath stays empty
	const char* error = NULL;

#ifdef __linux__
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
	if (err == 0) {
		procExeFilePath = GetRealPath(path);
	}
#elif defined(__FreeBSD__)
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	const long maxpath = pathconf("/", _PC_PATH_MAX);
	char buf[maxpath];
	size_t cb = sizeof(buf);
	int err = sysctl(mib, 4, buf, &cb, NULL, 0);
	if (err == 0) {
		procExeFilePath = buf;
	}
#else
	#error implement this
#endif

	if (procExeFilePath.empty()) {
		LOG_L(L_WARNING, "Failed to get file path of the process executable, reason: %s", error);
	}

	return procExeFilePath;
}

std::string GetProcessExecutablePath()
{
	return FileSystem::GetDirectory(GetProcessExecutableFile());
}

std::string GetModuleFile(std::string moduleName)
{
	std::string moduleFilePath = "";
	// this will only be used if moduleFilePath stays empty
	const char* error = NULL;

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
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
		if (moduleName.find("." SHARED_LIBRARY_EXTENSION) == std::string::npos) {
			moduleName = moduleName + "." SHARED_LIBRARY_EXTENSION;
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
			// required on APPLE; does not hurt elsewhere
			moduleFilePath = GetRealPath(moduleFilePath);
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
		LOG_L(L_WARNING, "Failed to get file path of the module \"%s\", reason: %s", moduleName.c_str(), error);
	}

	return UnQuote(moduleFilePath);
}
std::string GetModulePath(const std::string& moduleName)
{
	return FileSystem::GetDirectory(GetModuleFile(moduleName));
}

std::string GetOS()
{
#if defined(WIN32)
	return "Microsoft Windows\n" +
		GetOSDisplayString() + "\n" +
		GetHardwareInfoString();
#else
	struct utsname info;
	if (uname(&info) == 0) {
		return std::string(info.sysname) + " "
				+ info.release + " "
				+ info.version + " "
				+ info.machine;
	} else {
#if defined(__linux__)
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
// simply assume other OS don't need 32bit emulation
bool Is32BitEmulation()
{
	return false;
}
#endif

bool IsRunningInGDB() {
	#ifndef _WIN32
	char buf[1024];

	std::string fname = "/proc/" + IntToString(getppid(), "%d") + "/cmdline";
	std::ifstream f(fname.c_str());

	if (!f.good())
		return false;

	f.read(buf, sizeof(buf));
	f.close();

	return (strstr(buf, "gdb") != NULL);
	#else
	return IsDebuggerPresent();
	#endif
}

std::string GetShortFileName(const std::string& file) {
#ifdef WIN32
	std::vector<TCHAR> shortPathC(file.size() + 1, 0);

	// FIXME: stackoverflow.com/questions/843843/getshortpathname-unpredictable-results
	const int length = GetShortPathName(file.c_str(), &shortPathC[0], file.size() + 1);

	if (length > 0 && length <= (file.size() + 1)) {
		return (std::string(reinterpret_cast<const char*>(&shortPathC[0])));
	}
#endif

	return file;
}

std::string ExecuteProcess(const std::string& file, std::vector<std::string> args, bool asSubprocess)
{
	// "The first argument, by convention, should point to
	// the filename associated with the file being executed."
	// NOTE:
	//   spaces in the first argument or quoted file paths
	//   are not supported on Windows, so translate <file>
	//   to a short path there
	args.insert(args.begin(), GetShortFileName(file));

	std::string execError;

	if (asSubprocess) {
		#ifdef WIN32
		    STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory( &si, sizeof(si) );
			si.cb = sizeof(si);
			ZeroMemory( &pi, sizeof(pi) );

			std::string argsStr;
			for (size_t a = 0; a < args.size(); ++a) {
				argsStr += args[a] + ' ';
			}
			char *argsCStr = new char[argsStr.size() + 1];
			std::copy(argsStr.begin(), argsStr.end(), argsCStr);
			argsCStr[argsStr.size()] = '\0';

			LOG("[%s] Windows start process arguments: %s", __FUNCTION__, argsCStr);
			if (!CreateProcess(NULL, argsCStr, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
				delete[] argsCStr;
				LOG("[%s] Error creating subprocess (%lu)", __FUNCTION__, GetLastError());
				return execError;
			}
			delete[] argsCStr;

			return execError;
		#else
			int pid;
			if ((pid = fork()) < 0) {
				LOG("[%s] Error forking process", __FUNCTION__);
			} else if (pid != 0) {
				// TODO: Maybe useful to return the subprocess ID (pid)?
				return execError;
			}
		#endif
	}

	// "The array of pointers must be terminated by a NULL pointer."
	// --> include one extra argument string and leave it NULL
	std::vector<char*> processArgs(args.size() + 1, NULL);
	for (size_t a = 0; a < args.size(); ++a) {
		const std::string& arg = args[a];
		const size_t argSize = arg.length() + 1;

		STRCPY_T(processArgs[a] = new char[argSize], argSize, arg.c_str());
	}

	#ifdef WIN32
		#define EXECVP _execvp
	#else
		#define EXECVP execvp
	#endif
	if (EXECVP(args[0].c_str(), &processArgs[0]) == -1) {
		LOG("[%s] error: \"%s\" %s (%d)", __FUNCTION__, args[0].c_str(), (execError = strerror(errno)).c_str(), errno);
	}
	#undef EXECVP

	for (size_t a = 0; a < args.size(); ++a) {
		delete[] processArgs[a];
	}

	return execError;
}

} // namespace Platform
