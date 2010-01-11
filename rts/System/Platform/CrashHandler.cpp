#include "CrashHandler.h"
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

#ifdef _WIN32
// ### Windows CrashHandler START
#include "Win/CrashHandler.h"
#include "Platform/Win/seh.h"

namespace CrashHandler {

	void Install() {
		Win32::Install();
		InitializeSEH();
	};

	void Remove() {
		Win32::Remove();
	}

	void InstallHangHandler() {
		Win32::InstallHangHandler();
	}

	void UninstallHangHandler() {
		Win32::UninstallHangHandler();
	}

	void ClearDrawWDT(bool disable) {
		Win32::ClearDrawWDT(disable);
	}

	void ClearSimWDT(bool disable) {
		Win32::ClearSimWDT(disable);
	}
};

// ### Windows CrashHandler END
#elif !defined(__APPLE__) || (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_4)
// ### Unix(compliant) CrashHandler START
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <signal.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstdio>
#include <inttypes.h> // for uintptr_t
#include <boost/static_assert.hpp> // for BOOST_STATIC_ASSERT

#include "LogOutput.h"
#include "errorhandler.h"
#include "Game/GameVersion.h"
#include "Platform/Misc.h"
#include "FileSystem/FileSystemHandler.h"
#include "maindefines.h" // for SNPRINTF

/**
 * Returns the absolute version of a supplied relative path.
 * This is very simple, and can only handle "./", but not "../".
 */
static std::string createAbsolutePath(const std::string& relativePath) {

	std::string absolutePath;

	if (relativePath.length() > 0 && (relativePath[0] == '/')) {
		// is already absolute
		absolutePath = relativePath;
	} else {
		absolutePath = relativePath;
		if (absolutePath.find("./") == 0) {
			// remove initial "./"
			absolutePath = absolutePath.substr(2);
		}
		absolutePath = Platform::GetBinaryPath() + '/' + absolutePath;
	}

	return absolutePath;
}

/**
 * Returns a path to a file that most likely contains the debug symbols
 * for the supplied bianry file.
 * Always returns an absolute path.
 * Always returns an existing path, may be the input one.
 * precedence (top entries considered first):
 * 1. <bin-path>/<bin-file><bin-extension>.dbg
 * 2. <bin-path>/<bin-file>.dbg
 * 3. <debug-path><bin-path>/<bin-file><bin-extension>
 * 4. <bin-path>/<bin-file><bin-extension> (== input)
 * <debug-path> is system dependent; on debian it is: /usr/lib/debug
 * examples:
 * - "./spring"
 *   -> "/usr/games/spring"
 * - "./spring"
 *   -> "/usr/games/spring.dbg"
 * - "/usr/games/spring"
 *   -> "/usr/lib/debug/usr/games/spring"
 * - "/usr/games/spring-dedicated"
 *   -> "/usr/lib/debug/usr/games/spring-dedicated"
 * - "/usr/lib/spring/libunitsync.so"
 *   -> "/usr/lib/debug/usr/lib/spring/libunitsync.so"
 * - "/usr/lib/AI/Interfaces/Java/0.1/libAIInterface.so"
 *   -> "/usr/lib/debug/usr/lib/AI/Interfaces/Java/0.1/libAIInterface.so"
 * - "/usr/lib/AI/Skirmish/RAI/0.601/libSkirmishAI.so"
 *   -> "/usr/lib/debug/usr/lib/AI/Skirmish/RAI/0.601/libSkirmishAI.so"
 */
static std::string locateSymbolFile(const std::string& binaryFile) {

	std::string symbolFile;

	static const std::string debugPath = "/usr/lib/debug";
	const std::string absBinFile = binaryFile;

	const std::string::size_type lastSlash = absBinFile.find_last_of('/');
	std::string::size_type lastPoint       = absBinFile.find_last_of('.');
	if (lastPoint < lastSlash+2) {
		lastPoint = std::string::npos;
	}
	const std::string bin_path = absBinFile.substr(0, lastSlash);                       // eg: "/usr/lib"
	const std::string bin_file = absBinFile.substr(lastSlash+1, lastPoint-lastSlash-1); // eg: "libunitsync"
	std::string bin_ext        = "";
	if (lastPoint != std::string::npos) {
		bin_ext                = absBinFile.substr(lastPoint);                          // eg: ".so"
	}

	if (bin_ext.length() > 0) {
		symbolFile = bin_path + '/' + bin_file + bin_ext + ".dbg";
		if (FileSystemHandler::IsReadableFile(symbolFile)) {
			return symbolFile;
		}
	}

	symbolFile = bin_path + '/' + bin_file + ".dbg";
	if (FileSystemHandler::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	symbolFile = debugPath + bin_path + '/' + bin_file + bin_ext;
	if (FileSystemHandler::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	symbolFile = absBinFile;

	return symbolFile;
}


/**
 * Converts a string containing a hexadecimal value to a decimal value
 * with the size of a pointer.
 * example: "0xfa" -> 26
 */
static uintptr_t hexToInt(const char* hexStr) {

	BOOST_STATIC_ASSERT(sizeof(unsigned int long) == sizeof(uintptr_t));

	unsigned long int value = 0;

	sscanf(hexStr, "%lx", &value);

	return (uintptr_t) value;
}

static int intToHex(uintptr_t num, char* hexStr, size_t hexStr_sizeMax) {
	return SNPRINTF(hexStr, hexStr_sizeMax, "%lx", (unsigned long int)num);
}
/**
 * Converts a decimal value with the size of a pointer to a hexadecimal string.
 * example: 26 -> "0xfa"
 */
static std::string intToHexString(uintptr_t num) {

	char hexStr[33];

	if (intToHex(num, hexStr, 33) < 0) {
		hexStr[0] = '\0';
	}

	return std::string(hexStr);
}

/**
 * Finds the base memory address in the running process for all the libraries
 * involved in the crash.
 */
static void findBaseMemoryAddresses(std::map<std::string,uintptr_t>& binPath_baseMemAddr) {

	// store all paths which we have to find
	std::set<std::string> paths_notFound;
	std::map<std::string,uintptr_t>::const_iterator bpbmai;
	for (bpbmai = binPath_baseMemAddr.begin(); bpbmai != binPath_baseMemAddr.end(); ++bpbmai) {
		paths_notFound.insert(bpbmai->first);
	}

	FILE* mapsFile = NULL; 
	// /proc/self/maps contains the base addresses for all loaded dynamic
	// libaries of the current process + other stuff (which we are not interested in)
	mapsFile = fopen("/proc/self/maps", "rb");
	if (mapsFile != NULL) {
		std::set<std::string>::const_iterator pi;
		// format of /proc/self/maps:
		// (column names)  address           perms offset  dev   inode      pathname
		// (example 32bit) 08048000-08056000 r-xp 00000000 03:0c 64593      /usr/sbin/gpm
		// (example 64bit) ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0   [vsyscall]
		unsigned long int mem_start;
		unsigned long int binAddr_offset;
		char              binPathName[512];

		char line[512];
		int red;
		// read all lines
		while (!paths_notFound.empty() && (fgets(line, 511, mapsFile) != NULL)) {
			// parse the line
			red = sscanf(line, "%lx-%*x %*s %lx %*s %*u %s",
					&mem_start, &binAddr_offset, binPathName);

			if (red == 3) {
				if (binAddr_offset == 0) {
					//-> start of binary's memory space
					std::string matchingPath = "";
					// go through all paths of the binaries involved in the stack trace
					for (pi = paths_notFound.begin(); pi != paths_notFound.end(); ++pi) {
						// does the current line contain this binary?
						if (*pi == binPathName) {
							matchingPath = *pi;
							break;
						}
					}
					if (matchingPath != "") {
						const std::string mem_start_str = intToHexString(mem_start);
						binPath_baseMemAddr[matchingPath] = mem_start;
						paths_notFound.erase(matchingPath);
					}
				}
			}
		}
		fclose(mapsFile);
	}
}


namespace CrashHandler {

	void HandleSignal(int signal)
	{
		static const std::string INVALID_LINE_INDICATOR = "#####";
		const std::string logFileName = logOutput.GetFilename();

		std::string error;
		std::queue<std::string> paths;
		std::queue<uintptr_t> addresses;
		std::map<std::string,uintptr_t> binPath_baseMemAddr;

		logOutput.RemoveAllSubscribers();
		{
			LogObject log;
			if (signal == SIGSEGV) {
				error = "Segmentation fault (SIGSEGV)";
			} else if (signal == SIGILL) {
				error = "Illegal instruction (SIGILL)";
			} else if (signal == SIGPIPE) {
				error = "Broken pipe (SIGPIPE)";
			} else if (signal == SIGIO) {
				error = "IO-Error (SIGIO)";
			} else if (signal == SIGABRT) {
				error = "Aborted (SIGABRT)";
			} else {
				// we should never get here
				error = "Unknown signal";
			}
			log << error << " in spring " << SpringVersion::GetFull() << "\nStacktrace:\n";

			std::vector<void*> buffer(128);
			const int numLines = backtrace(&buffer[0], buffer.size());    // stack pointers
			char** lines       = backtrace_symbols(&buffer[0], numLines); // give them meaningfull names
			if (lines == NULL) {
				log << "Unable to create stacktrace\n";
			} else {
				bool containsOglSo = false;
				for (int l = 0; l < numLines; l++) {
					const std::string line(lines[l]);
					log << line;

					// example paths: "./spring" "/usr/lib/AI/Skirmish/NTai/0.666/libSkirmishAI.so"
					std::string path;
					size_t begin = 0;
					size_t end   = line.find_last_of('('); // if there is a function name
					if (end == std::string::npos) {
						end      = line.find_last_of('['); // if there is only the memory address
						if ((end != std::string::npos) && (end > 0)) {
							end--; // to get rid of the ' ' before the '['
						}
					}
					if (end == std::string::npos) {
						path = INVALID_LINE_INDICATOR;
					} else {
						path = line.substr(begin, end-begin);
					}

					// example address: "0x89a8206"
					std::string addr;
					begin = line.find_last_of('[');
					end = std::string::npos;
					if (begin != std::string::npos) {
						end = line.find_last_of(']');
					}
					if ((begin == std::string::npos) || (end == std::string::npos)) {
						addr = INVALID_LINE_INDICATOR;
					} else {
						addr = line.substr(begin+1, end-begin-1);
					}

					if (path == "") {
						log << " # NOTE: no path -> not translating";
					} else if ((path == INVALID_LINE_INDICATOR)
							|| (addr == INVALID_LINE_INDICATOR)) {
						log << " # NOTE: invalid stack-trace line -> not translating";
					} else {
						containsOglSo = (containsOglSo || (path.find("libGLcore.so") != std::string::npos));
						const std::string absPath = createAbsolutePath(path);
						binPath_baseMemAddr[absPath] = 0;
						paths.push(absPath);
						const uintptr_t addr_num = hexToInt(addr.c_str());
						addresses.push(addr_num);
					}
					log << "\n";
				}
				delete lines;
				lines = NULL;

				if (containsOglSo) {
					log << "This stack trace indicates a problem with your graphic card driver. Please try upgrading or downgrading it.\n";
				}
			}

			log << "Translated Stacktrace:\n";
		}
		logOutput.End(); // Stop writing to log.

		findBaseMemoryAddresses(binPath_baseMemAddr);

		std::string lastPath;
		while (!paths.empty()) {
			std::ostringstream buf;
			lastPath = paths.front();
			const std::string symbolFile = locateSymbolFile(lastPath);
			const uintptr_t baseAddress = binPath_baseMemAddr[lastPath];
			buf << "addr2line " << "--exe=\"" << symbolFile << "\"";
			// add addresses as long as the path stays the same
			while (!paths.empty() && (lastPath == paths.front())) {
				uintptr_t addr_num = addresses.front();
				if (paths.front() != Platform::GetBinaryFile() && (addr_num > baseAddress)) {
					// shift the stack trace address by the value of
					// the libraries base address in process memory
					// for all binaries that are not the processes main binary
					// (which could be spring or spring-dedicated for example)
					addr_num -= baseAddress;
				}
				buf << " 0x" << intToHexString(addr_num);
				lastPath = paths.front();
				paths.pop();
				addresses.pop();
			}
			buf << " >> " << logFileName; // pipe to infolog (which will be in CWD)
			system(buf.str().c_str());
		}

		ErrorMessageBox(error, "Spring crashed", 0);
	}
	
	void Install() {
		signal(SIGSEGV, HandleSignal); // segmentation fault
		signal(SIGILL,  HandleSignal); // illegal instruction
		signal(SIGPIPE, HandleSignal); // maybe some network error
		signal(SIGIO,   HandleSignal); // who knows?
		signal(SIGABRT, HandleSignal);
	}

	void Remove() {
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL,  SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGIO,   SIG_DFL);
		signal(SIGABRT, SIG_DFL);
	}

	void InstallHangHandler() {}

	void UninstallHangHandler() {}

	void ClearDrawWDT(bool disable) {}

	void ClearSimWDT(bool disable) {}
};

// ### Unix(compliant) CrashHandler END
#else
// ### Fallback CrashHandler (old Apple) START

namespace CrashHandler {
	void HandleSignal(int signal) {}
	void Install() {}
	void Remove() {}
	void InstallHangHandler() {}
	void UninstallHangHandler() {}
	void ClearDrawWDT(bool disable) {}
	void ClearSimWDT(bool disable) {}
};

// ### Fallback CrashHandler (old Apple) END
#endif
