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
#include <SDL_events.h>

#include "thread_backtrace.h"
#include "FileSystem/FileSystemHandler.h"
#include "Game/GameVersion.h"
#include "System/LogOutput.h"
#include "System/maindefines.h" // for SNPRINTF
#include "System/myTime.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"


static const int MAX_STACKTRACE_DEPTH = 10;
static const std::string INVALID_LINE_INDICATOR = "#####";
static const uintptr_t INVALID_ADDR_INDICATOR = 0xFFFFFFFF;


/**
 * Returns the absolute version of a supplied relative path.
 * This is very simple, and can only handle "./", but not "../".
 */
static std::string CreateAbsolutePath(const std::string& relativePath)
{
	std::string absolutePath;

	if (relativePath.length() > 0 && (relativePath[0] == '/')) {
		//! is already absolute
		absolutePath = relativePath;
	} else {
		absolutePath = relativePath;
		if (absolutePath.find("./") == 0) {
			//! remove initial "./"
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
static std::string LocateSymbolFile(const std::string& binaryFile)
{
	std::string symbolFile;

	static const std::string debugPath = "/usr/lib/debug";
	const std::string absBinFile = binaryFile;

	const std::string::size_type lastSlash = absBinFile.find_last_of('/');
	std::string::size_type lastPoint       = absBinFile.find_last_of('.');
	if (lastPoint < lastSlash+2) {
		lastPoint = std::string::npos;
	}
	const std::string bin_path = absBinFile.substr(0, lastSlash);                       //! eg: "/usr/lib"
	const std::string bin_file = absBinFile.substr(lastSlash+1, lastPoint-lastSlash-1); //! eg: "libunitsync"
	std::string bin_ext        = "";
	if (lastPoint != std::string::npos) {
		bin_ext                = absBinFile.substr(lastPoint);                          //! eg: ".so"
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
static uintptr_t HexToInt(const char* hexStr)
{
	BOOST_STATIC_ASSERT(sizeof(unsigned int long) == sizeof(uintptr_t));
	unsigned long int value = 0;
	sscanf(hexStr, "%lx", &value);
	return (uintptr_t) value;
}


/**
 * Finds the base memory address in the running process for all the libraries
 * involved in the crash.
 */
static void FindBaseMemoryAddresses(std::map<std::string,uintptr_t>& binPath_baseMemAddr)
{
	//! store all paths which we have to find
	std::set<std::string> paths_notFound;
	std::map<std::string,uintptr_t>::const_iterator bpbmai;
	for (bpbmai = binPath_baseMemAddr.begin(); bpbmai != binPath_baseMemAddr.end(); ++bpbmai) {
		paths_notFound.insert(bpbmai->first);
	}

	FILE* mapsFile = NULL;
	//! /proc/self/maps contains the base addresses for all loaded dynamic
	//! libaries of the current process + other stuff (which we are not interested in)
	mapsFile = fopen("/proc/self/maps", "rb");
	if (mapsFile != NULL) {
		std::set<std::string>::const_iterator pi;
		//! format of /proc/self/maps:
		//! (column names)  address           perms offset  dev   inode      pathname
		//! (example 32bit) 08048000-08056000 r-xp 00000000 03:0c 64593      /usr/sbin/gpm
		//! (example 64bit) ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0   [vsyscall]
		unsigned long int mem_start;
		unsigned long int binAddr_offset;
		char              binPathName[512];

		char line[512];
		int red;
		//! read all lines
		while (!paths_notFound.empty() && (fgets(line, 511, mapsFile) != NULL)) {
			//! parse the line
			red = sscanf(line, "%lx-%*x %*s %lx %*s %*u %s",
					&mem_start, &binAddr_offset, binPathName);

			if (red == 3) {
				if (binAddr_offset == 0) {
					//!-> start of binary's memory space
					std::string matchingPath = "";
					//! go through all paths of the binaries involved in the stack trace
					for (pi = paths_notFound.begin(); pi != paths_notFound.end(); ++pi) {
						// does the current line contain this binary?
						if (*pi == binPathName) {
							matchingPath = *pi;
							break;
						}
					}
					if (matchingPath != "") {
						binPath_baseMemAddr[matchingPath] = mem_start;
						paths_notFound.erase(matchingPath);
					}
				}
			}
		}
		fclose(mapsFile);
	}
}


/**
 * extracts the library/binary paths from the output of backtrace_symbols()
 */
static std::string ExtractPath(const std::string& line)
{
	//! example paths: "./spring" "/usr/lib/AI/Skirmish/NTai/0.666/libSkirmishAI.so"
	std::string path;
	size_t end   = line.find_last_of('('); //! if there is a function name
	if (end == std::string::npos) {
		end = line.find_last_of('['); //! if there is only the memory address
		if ((end != std::string::npos) && (end > 0)) {
			end--; //! to get rid of the ' ' before the '['
		}
	}
	if (end == std::string::npos) {
		path = INVALID_LINE_INDICATOR;
	} else {
		size_t begin = 0;
		path = line.substr(begin, end-begin);
	}
	return path;
}


/**
 * extracts the debug addr's from the output of backtrace_symbols()
 */
static uintptr_t ExtractAddr(const std::string& line)
{
	//! example address: "0x89a8206"
	uintptr_t addr = INVALID_ADDR_INDICATOR;
	size_t begin = line.find_last_of('[');
	size_t end = std::string::npos;
	if (begin != std::string::npos) {
		end = line.find_last_of(']');
	}
	if ((begin != std::string::npos) && (end != std::string::npos)) {
		addr = HexToInt(line.substr(begin+1, end-begin-1).c_str());
	}
	return addr;
}


static void TranslateStackTrace(std::vector<std::string>* lines, const std::vector< std::pair<std::string,uintptr_t> >& paths)
{
	//! Check if addr2line is available
	static int addr2line_found = -1;
	if (addr2line_found < 0)
	{
		FILE* cmdOut = popen("addr2line --help", "r");
		if (cmdOut == NULL) {
			addr2line_found = false;
		} else {
			addr2line_found = true;
			pclose(cmdOut);
		}
	}
	if (!addr2line_found) {
		logOutput.Print(" addr2line not found!");
		logOutput.Flush();
		return;
	}

	//! Detect BaseMemoryAddresses of all Lib's found in the stacktrace
	std::map<std::string,uintptr_t> binPath_baseMemAddr;
	for (std::vector< std::pair<std::string,uintptr_t> >::const_iterator it = paths.begin(); it != paths.end(); ++it) {
		binPath_baseMemAddr[it->first] = 0;
	}
	FindBaseMemoryAddresses(binPath_baseMemAddr);

	//! Finally translate it
	for (std::map<std::string,uintptr_t>::const_iterator it = binPath_baseMemAddr.begin(); it != binPath_baseMemAddr.end(); ++it) {
		const std::string symbolFile = LocateSymbolFile(it->first);
		std::ostringstream buf;
		buf << "addr2line " << "--exe=\"" << symbolFile << "\"";
		const uintptr_t& baseLibAddr = it->second;

		//! insert requested addresses that should be translated by addr2line
		std::queue<size_t> indices;
		for (size_t i = 0; i < paths.size(); ++i) {
			const std::pair<std::string,uintptr_t>& pt = paths[i];
			if (pt.first == it->first) {
				buf << " " << std::hex << (pt.second - baseLibAddr);
				indices.push(i);
			}
		}

		//! execute command addr2line, read stdout and write to log-file
		buf << " 2>/dev/null"; //! hide error output from spring's pipe
		FILE* cmdOut = popen(buf.str().c_str(), "r");
		if (cmdOut != NULL) {
			const size_t line_sizeMax = 2048;
			char line[line_sizeMax];
			while (fgets(line, line_sizeMax, cmdOut) != NULL) {
				const size_t i = indices.front();
				indices.pop();
				if (strcmp(line,"??:0\n") != 0) {
					(*lines)[i] = std::string(line);
				}
			}
			pclose(cmdOut);
		}
	}
}



namespace CrashHandler
{
	static void Stacktrace(bool* keepRunning, pthread_t* hThread = NULL, const char* threadName = NULL)
	{
		if (threadName) {
			logOutput.Print("Stacktrace (%s):", threadName);
		} else {
			logOutput.Print("Stacktrace:");
		}

		bool _keepRunning = false;
		if (!keepRunning)
			keepRunning = &_keepRunning;
		*keepRunning = false;
		bool containsOglSo = false; //! OpenGL lib -> graphic problem
		bool containedAIInterfaceSo = false;
		bool containedSkirmishAISo  = false;

		std::vector<std::string> stacktrace;
		std::vector< std::pair<std::string,uintptr_t> > symbols;

		//! Get untranslated stacktrace symbols
		{
			//! process and analyse the raw stack trace
			std::vector<void*> buffer(MAX_STACKTRACE_DEPTH + 2);
			int numLines;
			if (hThread) {
				logOutput.Print("  (Note: This stacktrace is not 100%% accurate! It just gives an impression.)");
				logOutput.Flush();
				numLines = thread_backtrace(*hThread, &buffer[0], buffer.size());    //! stack pointers
			} else {
				numLines = backtrace(&buffer[0], buffer.size());    //! stack pointers
				buffer.erase(buffer.begin()); //! pop Stacktrace()
				buffer.erase(buffer.begin()); //! pop HandleSignal()
				numLines -= 2;
			}
			numLines = (numLines > MAX_STACKTRACE_DEPTH) ? MAX_STACKTRACE_DEPTH : numLines;
			char** lines = backtrace_symbols(&buffer[0], numLines); //! give them meaningfull names

			stacktrace.reserve(numLines);
			for (int l = 0; l < numLines; l++) {
				stacktrace.push_back(lines[l]);
			}
			free(lines);
		}

		if (stacktrace.empty()) {
			logOutput.Print("  Unable to create stacktrace");
			return;
		}

		//! Extract important data from backtrace_symbols' output
		{
			for (std::vector<std::string>::iterator it = stacktrace.begin(); it != stacktrace.end(); ++it) {
				std::pair<std::string,uintptr_t> data;

				//! prepare for TranslateStackTrace()
				const std::string path    = ExtractPath(*it);
				const std::string absPath = CreateAbsolutePath(path);
				data.first  = absPath;
				data.second = ExtractAddr(*it);
				symbols.push_back(data);

				//! check if there are known sources of fail on the stack
				containsOglSo = (containsOglSo || (path.find("libGLcore.so") != std::string::npos));
				containsOglSo = (containsOglSo || (path.find("psb_dri.so") != std::string::npos));
				containsOglSo = (containsOglSo || (path.find("i965_dri.so") != std::string::npos));
				containsOglSo = (containsOglSo || (path.find("fglrx_dri.so") != std::string::npos));
				if (!containedAIInterfaceSo && (absPath.find("Interfaces") != std::string::npos)) {
					containedAIInterfaceSo = true;
				}
				if (!containedSkirmishAISo && (absPath.find("Skirmish") != std::string::npos)) {
					containedSkirmishAISo = true;
				}
			}

			//! Linux Graphic drivers are known to fail with moderate OpenGL usage
			if (containsOglSo) {
				logOutput.Print("This stack trace indicates a problem with your graphic card driver. "
						"Please try upgrading or downgrading it. "
						"Specifically recommended is the latest driver, and one that is as old as your graphic card. "
						"Also try lower graphic details and disabling Lua widgets in spring-settings.\n");
			}

			//! if stack trace contains AI and AI Interface frames,
			//! it is very likely that the problem lies in the AI only
			if (containedSkirmishAISo) {
				containedAIInterfaceSo = false;
			}
			if (containedAIInterfaceSo) {
				logOutput.Print("This stack trace indicates a problem with an AI Interface library.");
				*keepRunning = true;
			}
			if (containedSkirmishAISo) {
				logOutput.Print("This stack trace indicates a problem with a Skirmish AI library.");
				*keepRunning = true;
			}

			logOutput.Flush();
		}

		//! Translate it
		TranslateStackTrace(&stacktrace, symbols);

		//! Print out the StackTrace
		unsigned numLine = 0;
		for (std::vector<std::string>::iterator it = stacktrace.begin(); it != stacktrace.end(); ++it) {
			logOutput.Print("  <%u> %s", numLine++, it->c_str());
		}
		logOutput.Flush();
	}


	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName)
	{
		if (!Threading::IsMainThread(thread)) {
			logOutput.Print("Stacktrace (%s):", threadName.c_str());
			logOutput.Print("  No Stacktraces for non-MainThread.");
			return;
		}
		Stacktrace(NULL, &thread, threadName.c_str());
	}

	void PrepareStacktrace() {}
	void CleanupStacktrace() {}

	void HandleSignal(int signal)
	{
		if (signal == SIGINT) {
			static bool inExit = false;
			static spring_time exitTime = spring_notime;
			if (inExit) {
				//! give Spring 3 seconds to cleanly exit
				if (spring_gettime() > exitTime + spring_secs(3)) {
					exit(-1);
				}
			} else {
				inExit = true;
				exitTime = spring_gettime();
				SDL_Event event;
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);
			}
			return;
		}
		logOutput.SetSubscribersEnabled(false);

		std::string error;
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
			//! we should never get here
			error = "Unknown signal";
		}
		logOutput.Print("%s in spring %s\n", error.c_str(), SpringVersion::GetFull().c_str());

		//! print stacktrace
		bool keepRunning = false;
		Stacktrace(&keepRunning);

		//! only try to keep on running for these signals
		if (keepRunning &&
		    (signal != SIGSEGV) &&
		    (signal != SIGILL) &&
		    (signal != SIGPIPE) &&
		    (signal != SIGABRT)) {
			keepRunning = false;
		}

		//! try to clean up
		if (keepRunning) {
			bool cleanupOk = false;
			{
				//! try to cleanup
				/*if (!containedAIInterfaceSo.empty()) {
					//logOutput.Print("Trying to kill AI Interface library only ...\n");
					// TODO
					//cleanupOk = true;
				} else if (!containedSkirmishAISo.empty()) {
					//logOutput.Print("Trying to kill Skirmish AI library only ...\n");
					// TODO
					//cleanupOk = true;
				}*/
			}

			if (cleanupOk) {
				logOutput.SetSubscribersEnabled(true);
			} else {
				keepRunning = false;
			}
		}

		//! exit app if we catched a critical one
		if (!keepRunning) {
			std::ostringstream buf;
			buf << "Spring has crashed:\n"
				<< error << ".\n\n"
				<< "A stacktrace has been written to:\n"
				<< "  " << logOutput.GetFilePath();
			ErrorMessageBox(buf.str(), "Spring crashed", MBF_CRASH); //! this also calls exit()
		}
	}

	void Install() {
		signal(SIGSEGV, HandleSignal); //! segmentation fault
		signal(SIGILL,  HandleSignal); //! illegal instruction
		signal(SIGPIPE, HandleSignal); //! maybe some network error
		signal(SIGIO,   HandleSignal); //! who knows?
		signal(SIGABRT, HandleSignal);
		signal(SIGINT,  HandleSignal);
	}

	void Remove() {
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL,  SIG_DFL);
		signal(SIGPIPE, SIG_DFL);
		signal(SIGIO,   SIG_DFL);
		signal(SIGABRT, SIG_DFL);
		signal(SIGINT,  SIG_DFL);
	}
};
