/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(__linux__)
	#include <arpa/inet.h>
	#include <ifaddrs.h>
	#include <net/if.h>
	#include <netinet/in.h>
	#include <netpacket/packet.h>

	#include <sys/ioctl.h>
	#include <sys/socket.h>

#elif defined(WIN32)
	#include <io.h>
	#include <direct.h>
	#include <process.h>
	#include <shlobj.h>
	#include <shlwapi.h>
	#include <iphlpapi.h>

	#ifndef SHGFP_TYPE_CURRENT
		#define SHGFP_TYPE_CURRENT 0
	#endif

#elif defined(__APPLE__)
	#include <cstdlib>
	#include <climits> // for PATH_MAX

	#include <mach-o/dyld.h>

#elif defined( __FreeBSD__)
	#include <sys/sysctl.h>

#else

#endif



#if !defined(WIN32)
#include <dlfcn.h> // for dladdr(), dlopen()
#include <pwd.h> // for getpw*()
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/utsname.h> // for uname()
#include <unistd.h>
#endif



#include <cstring>
#include <cerrno>
#include <fstream>
#include <vector>

#include "Misc.h"
#include "System/CRC.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileSystem.h"
#if defined(WIN32)
#include "System/Platform/Win/WinVersion.h"
#endif
#include "System/Sync/SHA512.hpp"



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
	const char* home = getenv("LOCALAPPDATA");
	#else
	const char* home = getenv("HOME");
	#endif

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
	static std::string GetRealPath(const std::string& path)
	{
		char realPath[65536] = {0};
		char* realPathPtr = realpath(path.c_str(), &realPath[0]);

		if (realPathPtr == nullptr)
			memcpy(realPath, path.c_str(), std::min(path.size(), sizeof(realPath)));

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
		const char* procExeFilePath = "";
		// error will only be used if procExeFilePath stays empty
		const char* error = nullptr;

		#if defined(__linux__)
		char file[512];
		const int ret = readlink("/proc/self/exe", file, sizeof(file) - 1);

		if (ret >= 0) {
			file[ret] = '\0';
			procExeFilePath = file;
		} else {
			error = "[linux] failed to read /proc/self/exe";
		}


		#elif defined(WIN32)
		TCHAR procExeFile[MAX_PATH + 1];

		// main process executable handle
		const HMODULE hModule = GetModuleHandle(nullptr);
		const int ret = ::GetModuleFileName(hModule, procExeFile, sizeof(procExeFile));

		if ((ret != 0) && (ret != sizeof(procExeFile))) {
			procExeFilePath = procExeFile;
		} else {
			error = "[win32] unknown";
		}


		#elif defined(__APPLE__)
		uint32_t pathlen = PATH_MAX;
		char path[PATH_MAX];

		if (_NSGetExecutablePath(path, &pathlen) == 0)
			procExeFilePath = path;


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

		if (procExeFilePath[0] == 0)
			LOG_L(L_WARNING, "[%s] could not get process executable file path, reason: %s", __func__, error);

		#if defined(__APPLE__)
		return GetRealPath(procExeFilePath);
		#else
		return procExeFilePath;
		#endif
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



	std::string GetOSNameStr()
	{
		#if defined(WIN32)
		// "Windows Vista"
		return (windows::GetDisplayString(true, false, false));
		#else
		// "Linux 3.16.0-45-generic"
		// "Darwin 17.7.0"
		struct utsname info;
		if (uname(&info) == 0)
			return (std::string(info.sysname) + " " + info.release);
		#endif

		return {};
	}

	std::string GetOSVersionStr()
	{
		#if defined(WIN32)
		// "Ultimate Edition, 32-bit Service Pack 1 (build 7601)"
		return (windows::GetDisplayString(false, true, true));
		#else
		struct utsname info;
		// "#60~14.04.1-Ubuntu SMP Fri Jul 24 21:16:23 UTC 2015 (x86_64)"
		// "Darwin Kernel Version 17.7.0: Wed Sep 19 21:20:59 PDT 2018; root:xnu-4570.71.8~4/RELEASE_X86_64 (x86_64)"
		if (uname(&info) == 0)
			return (std::string(info.version) + " (" + info.machine + ")");

		return {};
		#endif
	}

	std::string GetOSDisplayStr() { return (GetOSNameStr() + " " + GetOSVersionStr()); }
	std::string GetOSFamilyStr()
	{
		#if defined(WIN32)
		return "Windows";
		#elif defined(__linux__)
		return "Linux";
		#elif defined(__FreeBSD__)
		return "FreeBSD";
		#elif defined(__APPLE__)
		return "MacOS";
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

	#if (defined(WIN32))
	std::string GetHardwareStr() { return (windows::GetHardwareString()); }
	#else
	std::string GetHardwareStr() {
		std::string ret;

		FILE* cpuInfo = fopen("/proc/cpuinfo", "r");
		FILE* memInfo = fopen("/proc/meminfo", "r");

		char buf[1024];
		char tmp[1024];

		if (cpuInfo != nullptr) {
			while (fgets(buf, sizeof(buf), cpuInfo) != nullptr) {
				if (strstr(buf, "model name") != nullptr) {
					const char* s = strstr(buf, ": ") + 2;
					const char* e = strstr(s, "\n");

					memset(tmp, 0, sizeof(tmp));
					memcpy(tmp, s, e - s);

					ret += (std::string(tmp) + "; ");
					break;
				}
			}

			fclose(cpuInfo);
		}

		if (memInfo != nullptr) {
			while (fgets(buf, sizeof(buf), memInfo) != nullptr) {
				if (strstr(buf, "MemTotal") != nullptr) {
					const char* s = strstr(buf, ": ") + 2;
					const char* e = s;

					for (     ; !std::isdigit(*s); s++) {}
					for (e = s;  std::isdigit(*e); e++) {}

					memset(tmp, 0, sizeof(tmp));
					memcpy(tmp, s, e - s);

					// sufficient up to 4TB
					uint32_t kb = 0;

					sscanf(tmp, "%u", &kb);
					sprintf(tmp, "%u", kb / 1024);

					ret += (std::string(tmp) + "MB RAM");
					break;
				}
			}

			fclose(memInfo);
		}

		return ret;
	}
	#endif


	std::string GetSysInfoHash() {
		std::vector<uint8_t> sysInfo;

		sha512::raw_digest rawHash;
		sha512::hex_digest hexHash;

		const std::string& oss = GetOSDisplayStr();
		const std::string& hws = GetHardwareStr();
		const std::string& wss = GetWordSizeStr();

		sysInfo.clear();
		sysInfo.resize((oss.size() + 1) + (hws.size() + 1) + (wss.size() + 1), 0);

		std::snprintf(reinterpret_cast<char*>(sysInfo.data()), sysInfo.size(), "%s\n%s\n%s\n", oss.data(), hws.data(), wss.data());

		sha512::calc_digest(sysInfo, rawHash);
		sha512::dump_digest(rawHash, hexHash);

		return {hexHash.data(), hexHash.data() + hexHash.size()};
	}

	std::string GetMacAddrHash() {
		const std::array<uint8_t, 6>& rawAddr = GetRawMacAddr();
		const std::string& hashStr = IntToString(CRC::CalcDigest(rawAddr.data(), rawAddr.size()), "%u");
		return hashStr;
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


	const char* ExecuteProcess(std::array<std::string, 32>& args, bool asSubprocess)
	{
		static char execError[1024];

		memset(execError, 0, sizeof(execError));
		strcpy(execError, "ExecuteProcess failure");

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

		if (EXECVP(args[0].c_str(), &argPointers[0]) == -1) {
			STRNCPY(execError, strerror(errno), sizeof(execError) - 1);
			LOG("[%s] error: \"%s\" %s (%d)", __func__, args[0].c_str(), execError, errno);
		}

		#undef EXECVP

		return execError;
	}







	#if defined(WIN32) || defined(_WIN32)
	bool GetMacType(std::array<uint8_t, 6>& macAddr, uint32_t macType) {
		IP_ADAPTER_INFO adapterInfo[16];

		DWORD dwBufLen = sizeof(adapterInfo);
		DWORD dwStatus = GetAdaptersInfo(adapterInfo, &dwBufLen);

		if (dwStatus == ERROR_BUFFER_OVERFLOW)
			return false;
		if (dwStatus != NO_ERROR)
			return false;

		for (size_t i = 0, n = dwBufLen / sizeof(adapterInfo); i < n; i++) {
			if ((macType != 0) && (adapterInfo[i].Type != macType))
				continue;
			if (adapterInfo[i].AddressLength != macAddr.size())
				continue;

			memcpy(macAddr.data(), adapterInfo[i].Address, adapterInfo[i].AddressLength);

			if (std::find_if(macAddr.begin(), macAddr.end(), [](uint8_t byte) { return (byte != 0); }) != macAddr.end())
				return true;
		}

		return false;
	}

	std::array<uint8_t, 6> GetRawMacAddr() {
		std::array<uint8_t, 6> macAddr = {{0, 0, 0, 0, 0, 0}};

		if (GetMacType(macAddr, MIB_IF_TYPE_ETHERNET))
			return macAddr;
		if (GetMacType(macAddr, IF_TYPE_IEEE80211))
			return macAddr;

		return (GetMacType(macAddr, 0), macAddr);
	}

	#elif defined(__APPLE__)

	std::array<uint8_t, 6> GetRawMacAddr() {
		// TODO: http://lists.freebsd.org/pipermail/freebsd-hackers/2004-June/007415.html
		return {{0, 0, 0, 0, 0, 0}};
	}

	#else

	std::array<uint8_t, 6> GetRawMacAddr() {
		std::array<uint8_t, 6> macAddr = {{0, 0, 0, 0, 0, 0}};

		ifaddrs* ifap = nullptr;

		if (getifaddrs(&ifap) != 0)
			return macAddr;

		for (ifaddrs* iter = ifap; iter != nullptr; iter = iter->ifa_next) {
			const sockaddr_ll* sal = reinterpret_cast<sockaddr_ll*>(iter->ifa_addr);

			if (sal->sll_family != AF_PACKET)
				continue;
			if (sal->sll_halen != macAddr.size())
				continue;

			memcpy(macAddr.data(), sal->sll_addr, sal->sll_halen);

			if (std::find_if(macAddr.begin(), macAddr.end(), [](uint8_t byte) { return (byte != 0); }) != macAddr.end())
				break;
		}

		freeifaddrs(ifap);
		return macAddr;
	}
	#endif

	std::array<char, 18> GetHexMacAddr() {
		std::array<uint8_t,  6> rawAddr = GetRawMacAddr();
		std::array<   char, 18> hexAddr;

		memset(hexAddr.data(), 0, hexAddr.size());
		snprintf(hexAddr.data(), hexAddr.size(), "%02x:%02x:%02x:%02x:%02x:%02x", rawAddr[0], rawAddr[1], rawAddr[2], rawAddr[3], rawAddr[4], rawAddr[5]);
		return hexAddr;
	}
} // namespace Platform
