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
#include <boost/bind.hpp>
#include <boost/static_assert.hpp> // for BOOST_STATIC_ASSERT
#include <boost/thread.hpp>
#include <SDL_events.h>
#include <sys/resource.h> //for getrlimits

#include "thread_backtrace.h"
#include "System/FileSystem/FileSystem.h"
#include "Game/GameVersion.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/LogOutput.h"
#include "System/maindefines.h" // for SNPRINTF
#include "System/Util.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include <new>


#ifdef __APPLE__
#define ADDR2LINE "gaddr2line"
#else
#define ADDR2LINE "addr2line"
#endif


static const int MAX_STACKTRACE_DEPTH = 100;
static const std::string INVALID_LINE_INDICATOR = "#####";
static const uintptr_t INVALID_ADDR_INDICATOR = 0xFFFFFFFF;


/**
 * Returns the absolute version of a supplied relative path.
 * This is very simple, and can only handle "./", but not "../".
 */
static std::string CreateAbsolutePath(const std::string& relativePath)
{
	if (relativePath.empty())
		return relativePath;

	std::string absolutePath = UnQuote(relativePath);

	if (absolutePath.length() > 0 && (absolutePath[0] == '/')) {
		//! is already absolute
	} else {
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
 * for the supplied binary file.
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

	const std::string bin_path = FileSystem::GetDirectory(binaryFile);
	const std::string bin_file = FileSystem::GetFilename(binaryFile);
	const std::string bin_ext  = FileSystem::GetExtension(binaryFile);

	if (!bin_ext.empty()) {
		symbolFile = bin_path + '/' + bin_file + bin_ext + ".dbg";
		if (FileSystem::IsReadableFile(symbolFile)) {
			return symbolFile;
		}
	}

	symbolFile = bin_path + '/' + bin_file + ".dbg";
	if (FileSystem::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	symbolFile = debugPath + bin_path + '/' + bin_file + bin_ext;
	if (FileSystem::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	symbolFile = binaryFile;

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
		FILE* cmdOut = popen(ADDR2LINE " --help", "r");
		if (cmdOut == NULL) {
			addr2line_found = false;
		} else {
			addr2line_found = true;
			pclose(cmdOut);
		}
	}
	if (!addr2line_found) {
		LOG_L(L_WARNING, " addr2line not found!");
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
		const std::string& libName = it->first;
		const uintptr_t&   libAddr = it->second;
		const std::string symbolFile = LocateSymbolFile(libName);

		std::ostringstream buf;
		buf << ADDR2LINE << " --exe=\"" << symbolFile << "\"";

		// insert requested addresses that should be translated by addr2line
		std::queue<size_t> indices;
		for (size_t i = 0; i < paths.size(); ++i) {
			const std::pair<std::string,uintptr_t>& pt = paths[i];
			if (pt.first == libName) {
				// Put it twice in the queue.
				// Depending on sys, compilation & lobby settings the libaddr doesn't need to be cut!
				// The detection of these situations is more complexe than just dropping the line that fails
				// (likely only one of the addresses will give something unequal to "??:0").
				buf << " " << std::hex << pt.second;
				indices.push(i);

				buf << " " << std::hex << (pt.second - libAddr);
				indices.push(i);
			}
		}

		// execute command addr2line, read stdout and write to log-file
		buf << " 2>/dev/null"; // hide error output from spring's pipe
		FILE* cmdOut = popen(buf.str().c_str(), "r");
		if (cmdOut != NULL) {
			const size_t line_sizeMax = 2048;
			char line[line_sizeMax];
			while (fgets(line, line_sizeMax, cmdOut) != NULL) {
				const size_t i = indices.front();
				indices.pop();
				if (strcmp(line,"??:0\n") != 0) {
					(*lines)[i] = std::string(line, strlen(line) - 1); // exclude the line-ending
				}
			}
			pclose(cmdOut);
		}
	}
}


static void ForcedExitAfterFiveSecs() {
	boost::this_thread::sleep(boost::posix_time::seconds(5));
	exit(-1);
}

static void ForcedExitAfterTenSecs() {
	boost::this_thread::sleep(boost::posix_time::seconds(10));
	std::_Exit(-1);
}


typedef struct sigaction sigaction_t;

static sigaction_t& GetSigAction(void (*s_hand)(int))
{
	static sigaction_t sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = s_hand;
	return sa;
}


namespace CrashHandler
{
	static void Stacktrace(bool* keepRunning, pthread_t* hThread = NULL, const char* threadName = NULL)
	{
		if (threadName) {
			LOG_L(L_ERROR, "Stacktrace (%s):", threadName);
		} else {
			LOG_L(L_ERROR, "Stacktrace:");
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
			if (hThread && Threading::GetCurrentThread() != *hThread) {
				LOG_L(L_ERROR, "  (Note: This stacktrace is not 100%% accurate! It just gives an impression.)");
				LOG_CLEANUP();
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
			LOG_L(L_ERROR, "  Unable to create stacktrace");
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
				LOG_L(L_ERROR, "This stack trace indicates a problem with your graphic card driver. "
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
				LOG_L(L_ERROR, "This stack trace indicates a problem with an AI Interface library.");
				*keepRunning = true;
			}
			if (containedSkirmishAISo) {
				LOG_L(L_ERROR, "This stack trace indicates a problem with a Skirmish AI library.");
				*keepRunning = true;
			}

			LOG_CLEANUP();
		}

		//! Translate it
		TranslateStackTrace(&stacktrace, symbols);

		//! Print out the translated StackTrace
		unsigned numLine = 0;
		for (std::vector<std::string>::iterator it = stacktrace.begin(); it != stacktrace.end(); ++it) {
			LOG_L(L_ERROR, "  <%u> %s", numLine++, it->c_str());
		}
	}


	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName)
	{
		//TODO Our custom thread_backtrace() only works on the mainthread.
		//     Use to gdb's libthread_db to get the stacktraces of all threads.
		if (!Threading::IsMainThread(thread) && Threading::GetCurrentThread() != thread) {
			LOG_L(L_ERROR, "Stacktrace (%s):", threadName.c_str());
			LOG_L(L_ERROR, "  No Stacktraces for non-MainThread.");
			return;
		}
		Stacktrace(NULL, &thread, threadName.c_str());
	}

	void PrepareStacktrace() {}

	void CleanupStacktrace() {
		LOG_CLEANUP();
	}

	void HandleSignal(int signal)
	{
		if (signal == SIGINT) {
			//! ctrl+c = kill
			LOG("caught SIGINT, aborting");

			//! first try a clean exit
			SDL_Event event;
			event.type = SDL_QUIT;
			SDL_PushEvent(&event);

			//! abort after 5sec
			boost::thread(boost::bind(&ForcedExitAfterFiveSecs));
			boost::thread(boost::bind(&ForcedExitAfterTenSecs));
			return;
		}

		logSinkHandler.SetSinking(false);

		std::string error = strsignal(signal);
		// append the signal name (it seems there is no OS function to map signum to signame :<)
		if (signal == SIGSEGV) {
			error += " (SIGSEGV)";
		} else if (signal == SIGILL) {
			error += " (SIGILL)";
		} else if (signal == SIGPIPE) {
			error += " (SIGPIPE)";
		} else if (signal == SIGIO) {
			error += " (SIGIO)";
		} else if (signal == SIGABRT) {
			error += " (SIGABRT)";
		} else if (signal == SIGFPE) {
			error += " (SIGFPE)";
		} else if (signal == SIGBUS) {
			error += " (SIGBUS)";
		}
		LOG_L(L_ERROR, "%s in spring %s", error.c_str(), SpringVersion::GetFull().c_str());

		//! print stacktrace
		bool keepRunning = false;

		PrepareStacktrace();
		Stacktrace(&keepRunning);
		CleanupStacktrace();

		//! don't try to keep on running after these signals
		if (keepRunning &&
		    (signal != SIGSEGV) &&
		    (signal != SIGILL) &&
		    (signal != SIGPIPE) &&
		    (signal != SIGABRT) &&
		    (signal != SIGBUS)) {
			keepRunning = false;
		}

		//! try to clean up
		if (keepRunning) {
			bool cleanupOk = false;
			{
				//! try to cleanup
				/*if (!containedAIInterfaceSo.empty()) {
					//LOG_L(L_ERROR, "Trying to kill AI Interface library only ...\n");
					// TODO
					//cleanupOk = true;
				} else if (!containedSkirmishAISo.empty()) {
					//LOG_L(L_ERROR, "Trying to kill Skirmish AI library only ...\n");
					// TODO
					//cleanupOk = true;
				}*/
			}

			if (cleanupOk) {
				logSinkHandler.SetSinking(true);
			} else {
				keepRunning = false;
			}
		}

		//! exit app if we catched a critical one
		if (!keepRunning) {
			//! don't handle any further signals when exiting
			Remove();
			
			std::ostringstream buf;
			buf << "Spring has crashed:\n"
				<< error << ".\n\n"
				<< "A stacktrace has been written to:\n"
				<< "  " << logOutput.GetFilePath();
			ErrorMessageBox(buf.str(), "Spring crashed", MBF_OK | MBF_CRASH); //! this also calls exit()
		}
	}

	void OutputStacktrace() {
		bool keepRunning = true;
		PrepareStacktrace();
		Stacktrace(&keepRunning);
		CleanupStacktrace();
	}

	void NewHandler() {
		LOG_L(L_ERROR, "Failed to allocate memory"); // make sure this ends up in the log also

		OutputStacktrace();

		ErrorMessageBox("Failed to allocate memory", "Spring: Fatal Error", MBF_OK | MBF_CRASH);
	}

	void Install() {
		struct rlimit limits;
		if ((getrlimit(RLIMIT_CORE, &limits) == 0) && (limits.rlim_cur > 0)) {
			LOG("Core dumps enabled, not installing signal handler");
			LOG("see /proc/sys/kernel/core_pattern where it gets written");
			return;
		}
		const sigaction_t& sa = GetSigAction(&HandleSignal);

		sigaction(SIGSEGV, &sa, NULL); // segmentation fault
		sigaction(SIGILL,  &sa, NULL); // illegal instruction
		sigaction(SIGPIPE, &sa, NULL); // maybe some network error
		sigaction(SIGIO,   &sa, NULL); // who knows?
		sigaction(SIGFPE,  &sa, NULL); // div0 and more
		sigaction(SIGABRT, &sa, NULL);
		sigaction(SIGINT,  &sa, NULL);
		sigaction(SIGBUS,  &sa, NULL); // on macosx EXC_BAD_ACCESS (mach exception) is translated to SIGBUS

		std::set_new_handler(NewHandler);
	}

	void Remove() {
		const sigaction_t& sa = GetSigAction(SIG_DFL);

		sigaction(SIGSEGV, &sa, NULL); // segmentation fault
		sigaction(SIGILL,  &sa, NULL); // illegal instruction
		sigaction(SIGPIPE, &sa, NULL); // maybe some network error
		sigaction(SIGIO,   &sa, NULL); // who knows?
		sigaction(SIGFPE,  &sa, NULL); // div0 and more
		sigaction(SIGABRT, &sa, NULL);
		sigaction(SIGINT,  &sa, NULL);
		sigaction(SIGBUS,  &sa, NULL); // on macosx EXC_BAD_ACCESS (mach exception) is translated to SIGBUS

		std::set_new_handler(NULL);
	}
};
