/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Misc.h"

#ifdef __linux__
#include <unistd.h>
#include <dlfcn.h> // for dladdr(), dlopen()
#include <sys/statvfs.h>

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
#include <sys/statvfs.h>

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
#include <deque>
#include <vector>

#include "System/StringUtil.h"
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
	// note: both solutions use this function's own address
	// http://stackoverflow.com/questions/557081/how-do-i-get-the-hmodule-for-the-currently-executing-code/557774

	// Win 2000+ solution
	MEMORY_BASIC_INFORMATION mbi = {0};
	::VirtualQuery((void*)GetCurrentModule, &mbi, sizeof(mbi));
	HMODULE hModule = reinterpret_cast<HMODULE>(mbi.AllocationBase);

	// Win XP+ solution (cleaner)
	//::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR) GetCurrentModule, &hModule);

	return hModule;
}
#endif


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
	const char* home = getenv(HOME_ENV_VAR);
	#undef HOME_ENV_VAR

	return (home == nullptr) ? "" : home;
}

static std::string GetUserDirFromSystemApi()
{
	#ifdef _WIN32
	TCHAR strPath[MAX_PATH + 1];
	SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, strPath);
	return strPath;
	#else
	const struct passwd* pw = getpwuid(getuid());
	return pw->pw_dir;
	#endif
}


namespace Platform
{
	static std::string origCWD;


	const std::string& GetOrigCWD() { return origCWD; }
	const std::string& SetOrigCWD()
	{
		if (!origCWD.empty())
			return origCWD;

		origCWD = std::move(FileSystemAbstraction::GetCwd());
		FileSystemAbstraction::EnsurePathSepAtEnd(origCWD);
		return origCWD;
	}


	std::string GetUserDir()
	{
		const std::string& userDir = GetUserDirFromEnvVar();

		// In some cases the env var is not set, for example for non-human user accounts
		// or when starting through the UI on OS X. It is unset by default on windows.
		if (userDir.empty())
			return (GetUserDirFromSystemApi());

		return userDir;
	}

	#ifndef WIN32
	static std::string GetRealPath(const std::string& path) {

		std::string realPath = path;

		// using NULL here is not supported in very old systems,
		// but should be no problem for spring
		// see for older systems:
		// http://stackoverflow.com/questions/4109638/what-is-the-safe-alternative-to-realpath
		char* realPathC = realpath(path.c_str(), nullptr);

		if (realPathC != nullptr) {
			realPath.assign(realPathC);
			free(realPathC);
		}

		if (FileSystem::GetDirectory(realPath).empty())
			return (GetProcessExecutablePath() + realPath);

		return realPath;
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
		std::string procExeFilePath;

		// error will only be used if procExeFilePath stays empty
		const char* error = nullptr;

	#ifdef __linux__
		char file[512];
		const int ret = readlink("/proc/self/exe", file, sizeof(file) - 1);

		if (ret >= 0) {
			file[ret] = '\0';
			procExeFilePath = std::string(file);
		} else {
			error = "[linux] failed to read /proc/self/exe";
		}


	#elif WIN32
		TCHAR procExeFile[MAX_PATH + 1];

		// with NULL, it will return the handle
		// for the main executable of the process
		const HMODULE hModule = GetModuleHandle(nullptr);
		const int ret = ::GetModuleFileName(hModule, procExeFile, sizeof(procExeFile));

		if ((ret != 0) && (ret != sizeof(procExeFile))) {
			procExeFilePath = std::string(procExeFile);
		} else {
			error = "[win32] unknown";
		}


	#elif __APPLE__
		uint32_t pathlen = PATH_MAX;
		char path[PATH_MAX];

		if (_NSGetExecutablePath(path, &pathlen) == 0)
			procExeFilePath = GetRealPath(path);


	#elif defined(__FreeBSD__)
		const int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
		const long maxpath = pathconf("/", _PC_PATH_MAX);

		char buf[maxpath];
		size_t cb = sizeof(buf);

		if (sysctl(mib, sizeof(mib) / sizeof(mib[0]), buf, &cb, nullptr, 0) == 0)
			procExeFilePath = buf;


	#else
		#error implement this
	#endif

		if (procExeFilePath.empty())
			LOG_L(L_WARNING, "[%s] could not get process executable file path, reason: %s", __func__, error);

		return procExeFilePath;
	}

	std::string GetProcessExecutablePath()
	{
		return FileSystem::GetDirectory(GetProcessExecutableFile());
	}


	std::string GetModuleFile(std::string moduleName)
	{
		std::string moduleFilePath;

		// this will only be used if moduleFilePath stays empty
		const char* error = nullptr;

	#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
		#ifdef __APPLE__
		#define SHARED_LIBRARY_EXTENSION "dylib"
		#else
		#define SHARED_LIBRARY_EXTENSION "so"
		#endif

		void* moduleAddress = nullptr;

		// find an address in the module we are looking for
		if (moduleName.empty()) {
			// look for current module
			moduleAddress = (void*) GetModuleFile;
		} else {
			// look for specified module

			// add extension if it is not in the file name
			// it could also be "libXZY.so-1.2.3"
			// -> does not have to be the end, my friend
			if (moduleName.find("." SHARED_LIBRARY_EXTENSION) == std::string::npos)
				moduleName = moduleName + "." SHARED_LIBRARY_EXTENSION;

			// will not not try to load, but return the libs address
			// if it is already loaded, NULL otherwise
			if ((moduleAddress = dlopen(moduleName.c_str(), RTLD_LAZY | RTLD_NOLOAD)) == nullptr) {
				// if not found, try with "lib" prefix
				moduleName = "lib" + moduleName;
				moduleAddress = dlopen(moduleName.c_str(), RTLD_LAZY | RTLD_NOLOAD);
			}
		}

		if (moduleAddress != nullptr) {
			// fetch info about the module containing the address we just evaluated
			Dl_info moduleInfo;
			const int ret = dladdr(moduleAddress, &moduleInfo);

			if ((ret != 0) && (moduleInfo.dli_fname != nullptr)) {
				moduleFilePath = moduleInfo.dli_fname;
				// required on APPLE; does not hurt elsewhere
				moduleFilePath = GetRealPath(moduleFilePath);
			} else {
				if ((error = dlerror()) == nullptr) {
					error = "Unknown";
				}
			}
		} else {
			error = "Not loaded";
		}

	#elif WIN32
		HMODULE hModule = nullptr;

		if (moduleName.empty()) {
			hModule = GetCurrentModule();
		} else {
			// If this fails, we get a NULL handle
			hModule = GetModuleHandle(moduleName.c_str());
		}

		if (hModule != nullptr) {
			// fetch module file name
			TCHAR moduleFile[MAX_PATH+1];
			const int ret = ::GetModuleFileName(hModule, moduleFile, sizeof(moduleFile));

			if ((ret != 0) && (ret != sizeof(moduleFile))) {
				moduleFilePath = std::string(moduleFile);
			} else {
				error = "Unknown";
			}
		} else {
			error = "Not found";
		}
	#else
		#warning implement this (or use linux version)
	#endif

		if (moduleFilePath.empty()) {
			if (moduleName.empty())
				moduleName = "<current>";

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
		return ("Microsoft Windows\n" + GetOSDisplayString() + "\n" + GetHardwareInfoString());
	#else
		struct utsname info;
		if (uname(&info) == 0)
			return (std::string(info.sysname) + " " + info.release + " " + info.version + " " + info.machine);

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
	#endif
	}

	std::string GetOSFamilyStr()
	{
		#if defined(WIN32)
		return "Windows";
		#elif defined(__linux__)
		return "Linux";
		#elif defined(__FreeBSD__)
		return "FreeBSD";
		#elif defined(__APPLE__)
		return "MacOSX";
		#else
		return "Unknown";
		#endif
	}

	std::string GetWordSizeStr()
	{
		if (Is64Bit())
			return "64-bit (native)";

		return (std::string("32-bit ") + (Is32BitEmulation()? "(emulated)": "(native)"));
	}

	std::string GetPlatformStr()
	{
		return (Platform::GetOSFamilyStr() + " " + Platform::GetWordSizeStr());
	}


	uint64_t FreeDiskSpace(const std::string& path) {
		#ifdef WIN32
		ULARGE_INTEGER bytesFree;

		if (!GetDiskFreeSpaceEx(path.c_str(), &bytesFree, nullptr, nullptr))
			return 0;

		return (bytesFree.QuadPart / (1024 * 1024));

		#else

		struct statvfs st;

		if (statvfs(path.c_str(), &st) != 0)
			return -1;

		if (st.f_frsize != 0)
			return (((uint64_t)st.f_frsize * st.f_bavail) / (1024 * 1024));

		return (((uint64_t)st.f_bsize * st.f_bavail) / (1024 * 1024));
		#endif
	}


	uint32_t NativeWordSize() { return (sizeof(void*)); }
	uint32_t SystemWordSize() { return ((Is32BitEmulation())? 8: NativeWordSize()); }
	uint32_t DequeChunkSize() {
		std::deque<int> q = {0, 1};
		for (int i = 1; ((&q[i] - &q[i - 1]) == 1); q.push_back(++i));
		return (q.size() - 1);
	}


	bool Is64Bit() { return (NativeWordSize() == 8); }

	#ifdef WIN32
	/** @brief checks if the current process is running in 32bit emulation mode
		@return FALSE, TRUE, -1 on error (usually no permissions) */
	bool Is32BitEmulation()
	{
		typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

		HMODULE hModule = GetModuleHandle(TEXT("kernel32"));
		LPFN_ISWOW64PROCESS isWoW64ProcFn = (LPFN_ISWOW64PROCESS) GetProcAddress(hModule, "IsWow64Process");

		BOOL isWoW64Proc = FALSE;

		if (isWoW64ProcFn == nullptr)
			return isWoW64Proc;

		if (!isWoW64ProcFn(GetCurrentProcess(), &isWoW64Proc))
			return false;

		return isWoW64Proc;
	}
	#else
	// simply assume Spring is never run in emulation-mode on other OS'es
	bool Is32BitEmulation() { return false; }
	#endif

	bool IsRunningInGDB() {
		#ifndef _WIN32
		char buf[1024];

		std::ifstream f("/proc/" + IntToString(getppid(), "%d") + "/cmdline");

		if (!f.good())
			return false;

		f.read(buf, sizeof(buf));
		f.close();

		return (strstr(buf, "gdb") != nullptr);
		#else
		return IsDebuggerPresent();
		#endif
	}


	std::string GetShortFileName(const std::string& file) {
		#ifdef WIN32
		std::vector<TCHAR> shortPathC(file.size() + 1, 0);

		// FIXME: stackoverflow.com/questions/843843/getshortpathname-unpredictable-results
		const int length = GetShortPathName(file.c_str(), &shortPathC[0], file.size() + 1);

		if (length > 0 && length <= (file.size() + 1))
			return (std::string(reinterpret_cast<const char*>(&shortPathC[0])));
		#endif

		return file;
	}


	std::string ExecuteProcess(std::array<std::string, 32>& args, bool asSubprocess)
	{
		std::string execError = "ExecuteProcess failure";

		// "The array of pointers must be terminated by a NULL pointer."
		// --> always include one extra argument string and leave it NULL
		std::array<char*, (sizeof(args) / sizeof(args[0])) + 1> argPointers;
		std::array<char[4096], (sizeof(args) / sizeof(args[0]))> processArgs;

		// "The first argument, by convention, should point to
		// the filename associated with the file being executed."
		// NOTE:
		//   spaces in the first argument or quoted file paths
		//   are not supported on Windows, so translate args[0]
		//   to a short path there
		args[0] = GetShortFileName(args[0]);

		if (asSubprocess) {
			#ifdef WIN32
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));

			char* flatArgsStr = &processArgs[0][0];

			{
				size_t i = 0;
				size_t n = sizeof(processArgs);

				// flatten args, i.e. from {"s0", "s1", "s2"} to "s0 s1 s2"
				for (size_t a = 0; a < args.size(); ++a, ++i) {
					if (args[a].empty())
						break;
					if ((i + args[a].size()) >= n)
						break;

					memcpy(&flatArgsStr[i                  ], args[a].data(), args[a].size());
					memset(&flatArgsStr[i += args[a].size()], ' ', 1);
				}

				memset(&flatArgsStr[ std::min(i, n - 1) ], '\0', 1);
			}

			if (!CreateProcess(nullptr, flatArgsStr, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi))
				LOG("[%s] error %lu creating subprocess with arguments \"%s\"", __func__, GetLastError(), flatArgsStr);

			#else

			int pid;
			if ((pid = fork()) < 0) {
				LOG("[%s] error forking process", __func__);
			} else if (pid != 0) {
				// TODO: Maybe useful to return the subprocess ID (pid)?
			}
			#endif

			return execError;
		}

		for (size_t a = 0; a < args.size(); ++a) {
			if (args[a].empty())
				break;

			memset(&processArgs[a][0], 0, sizeof(processArgs[a]));
			memcpy(&processArgs[a][0], args[a].c_str(), std::min(args[a].size(), sizeof(processArgs[a]) - 1));

			argPointers[a    ] = &processArgs[a][0];
			argPointers[a + 1] = nullptr;
		}

		#ifdef WIN32
			#define EXECVP _execvp
		#else
			#define EXECVP execvp
		#endif

		if (EXECVP(args[0].c_str(), &argPointers[0]) == -1)
			LOG("[%s] error: \"%s\" %s (%d)", __func__, args[0].c_str(), (execError = strerror(errno)).c_str(), errno);

		#undef EXECVP

		return execError;
	}

} // namespace Platform
