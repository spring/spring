/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

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

#include "FileSystem/FileSystemHandler.h"
#include "Game/GameVersion.h"
#include "System/LogOutput.h"
#include "System/maindefines.h" // for SNPRINTF
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"

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
		absolutePath = Platform::GetModulePath() + '/' + absolutePath;
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
		const std::string logFile = logOutput.GetFilePath();

		std::string error;
		std::queue<std::string> paths;
		std::queue<uintptr_t> addresses;
		std::map<std::string,uintptr_t> binPath_baseMemAddr;

		bool keepRunning   = false;
		bool containsOglSo = false; // OpenGL lib -> graphic problem
		std::string containedAIInterfaceSo = "";
		std::string containedSkirmishAISo  = "";

		logOutput.SetSubscribersEnabled(false);
		{
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
			logOutput.Print("%s in spring %s\n", error.c_str(), SpringVersion::GetFull().c_str());
			logOutput.Print("Stacktrace:\n");

			// process and analyse the raw stack trace
			std::vector<void*> buffer(128);
			const int numLines = backtrace(&buffer[0], buffer.size());    // stack pointers
			char** lines       = backtrace_symbols(&buffer[0], numLines); // give them meaningfull names
			if (lines == NULL) {
				logOutput.Print("Unable to create stacktrace\n");
			} else {
				for (int l = 0; l < numLines; l++) {
					const std::string line(lines[l]);
					logOutput.Print("%s\n", line.c_str());
					logOutput.Flush();

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
						logOutput.Print("# NOTE: above line shows no path -> not translating\n");
					} else if ((path == INVALID_LINE_INDICATOR)
							|| (addr == INVALID_LINE_INDICATOR)) {
						logOutput.Print("# NOTE: above line is invalid -> not translating\n");
					} else {
						containsOglSo = (containsOglSo || (path.find("libGLcore.so") != std::string::npos));
						containsOglSo = (containsOglSo || (path.find("psb_dri.so") != std::string::npos));
						containsOglSo = (containsOglSo || (path.find("i965_dri.so") != std::string::npos));
						containsOglSo = (containsOglSo || (path.find("fglrx_dri.so") != std::string::npos));
						const std::string absPath = createAbsolutePath(path);
						if (containedAIInterfaceSo.empty() && (absPath.find("Interfaces") != std::string::npos)) {
							containedAIInterfaceSo = absPath;
						}
						if (containedSkirmishAISo.empty() && (absPath.find("Skirmish") != std::string::npos)) {
							containedSkirmishAISo = absPath;
						}
						binPath_baseMemAddr[absPath] = 0;
						paths.push(absPath);
						const uintptr_t addr_num = hexToInt(addr.c_str());
						addresses.push(addr_num);
					}
				}
				free(lines);
				lines = NULL;

				// if stack trace contains AI and AI Interface frames,
				// it is very likely that the problem lies in the AI only
				if (!containedSkirmishAISo.empty()) {
					containedAIInterfaceSo = "";
				}

				if (containsOglSo) {
					logOutput.Print("This stack trace indicates a problem with your graphic card driver. "
					                "Please try upgrading or downgrading it. "
					                "Specifically recommended is the latest driver, and one that is as old as your graphic card. "
					                "Also try lower graphic details and disabling Lua widgets in spring-settings.\n");
					logOutput.Flush();
				}
				if (!containedAIInterfaceSo.empty()) {
					logOutput.Print("This stack trace indicates a problem with an AI Interface library.\n");
					logOutput.Flush();
					keepRunning = true;
				}
				if (!containedSkirmishAISo.empty()) {
					logOutput.Print("This stack trace indicates a problem with a Skirmish AI library.\n");
					logOutput.Flush();
					keepRunning = true;
				}
			}
			logOutput.Flush();
		}

		// translate the stack trace, and write it to the log file
		logOutput.Print("Translated Stacktrace:\n");
		logOutput.Flush();
		findBaseMemoryAddresses(binPath_baseMemAddr);
		std::string lastPath;
		const size_t line_sizeMax = 2048;
		char line[line_sizeMax];
		while (!paths.empty()) {
			std::ostringstream buf;
			lastPath = paths.front();
			const std::string symbolFile = locateSymbolFile(lastPath);
			const uintptr_t baseAddress = binPath_baseMemAddr[lastPath];
			buf << "addr2line " << "--exe=\"" << symbolFile << "\"";
			// add addresses as long as the path stays the same
			while (!paths.empty() && (lastPath == paths.front())) {
				uintptr_t addr_num = addresses.front();
				if (paths.front() != Platform::GetModuleFile() && (addr_num > baseAddress)) {
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

			// execute command addr2line, read stdout and write to log-file
			FILE* cmdOut = popen(buf.str().c_str(), "r");
			if (cmdOut != NULL) {
				while (fgets(line, line_sizeMax, cmdOut) != NULL) {
					logOutput.Print("%s", line);
				}
				pclose(cmdOut);
			}
		}

		// only try to keep on running for these signals
		if (keepRunning &&
		    (signal != SIGSEGV) &&
		    (signal != SIGILL) &&
		    (signal != SIGPIPE) &&
		    (signal != SIGABRT)) {
			keepRunning = false;
		}

		// try to clean up
		if (keepRunning) {
			bool cleanupOk = false;
			{
				// try to cleanup
				if (!containedAIInterfaceSo.empty()) {
					//logOutput.Print("Trying to kill AI Interface library only ...\n");
					// TODO
					//cleanupOk = true;
				} else if (!containedSkirmishAISo.empty()) {
					//logOutput.Print("Trying to kill Skirmish AI library only ...\n");
					// TODO
					//cleanupOk = true;
				}
			}

			if (cleanupOk) {
				logOutput.SetSubscribersEnabled(true);
			} else {
				keepRunning = false;
			}
		}

		if (!keepRunning) {
			logOutput.End();
			// this also calls exit()
			ErrorMessageBox(error, "Spring crashed", 0);
		}
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

	void GameLoading(bool) {}
};
