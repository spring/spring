/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

#include <string>
#include <string.h> // strnlen
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <new>
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
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <dlfcn.h>

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

#ifndef DEDICATED
#include "System/Platform/Watchdog.h"
#endif

#if !defined(__APPLE__)
#define ADDR2LINE "addr2line"
#endif

static const int MAX_STACKTRACE_DEPTH = 100;
static const std::string INVALID_LINE_INDICATOR = "#####";
static const uintptr_t INVALID_ADDR_INDICATOR = 0xFFFFFFFF;

struct StackFunction {
    std::string fileline;
    std::string funcname;
	std::string abbrev_fileline;
	std::string abbrev_funcname;
    bool inLine;
    StackFunction() : inLine(true) {}
};

struct StackFrame {
	int                   level;    // level in the original unwinding (inlined functions share the same level as their "caller")
	void*                 ip;       // instruction pointer from libunwind or backtrace()
	std::string           mangled;  // mangled name retrieved from libunwind (not printed, memoized for debugging)
	std::string           symbol;   // backtrace_symbols output
	uintptr_t             addr;     // translated address / load address for OS X
	std::string           path;     // translated library or module path
	std::list<StackFunction> entries;  // function names and lines (possibly several inlined) retrieved from addr2line
	StackFrame() :
		level(0),
		ip(0),
		addr(0) { }
};

typedef std::vector<StackFrame> StackTrace;

static int reentrances = 0;

static void TranslateStackTrace(bool* aiCrash, StackTrace& stacktrace, const int logLevel);
static void LogStacktrace(const int logLevel, StackTrace& stacktrace);


static std::string GetBinaryLocation()
{
#if  defined(UNITSYNC)
	return Platform::GetModulePath();
#else
	return Platform::GetProcessExecutablePath();
#endif
}

#define LOG_SECTION_CRASHHANDLER "CrashHandler"

/* Initialized before main() */
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_CRASHHANDLER);

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
        #undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_CRASHHANDLER


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
		// is already absolute
	} else {
		if (absolutePath.find("./") == 0) {
			// remove initial "./"
			absolutePath = absolutePath.substr(2);
		}

		absolutePath = FileSystemAbstraction::EnsurePathSepAtEnd(GetBinaryLocation()) + absolutePath;
	}

	if (!FileSystem::FileExists(absolutePath))
		return relativePath;

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

	symbolFile = bin_path + bin_file + ".dbg";
	if (FileSystem::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	symbolFile = debugPath + bin_path + bin_file;
	if (FileSystem::IsReadableFile(symbolFile)) {
		return symbolFile;
	}

	return binaryFile;
}


static char* fgets_addr2line(char* line, int maxLength, FILE* cmdOut)
{
    char* res = fgets(line, maxLength, cmdOut);
    if (res) {
        int sz = strnlen(line, maxLength);
        if (line[sz-1] == '\n') {
            line[sz-1] = 0;// exclude the line-ending
        }
    } else {
        line[0] = 0;
    }
    LOG_L(L_DEBUG, "addr2line: %s", line);

    return res;
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
 * Consumes (and frees) the lines produced by backtrace_symbols and puts these into the StackTrace fields
 */
static void ExtractSymbols(char** lines, StackTrace& stacktrace)
{
	int l=0;
	auto fit = stacktrace.begin();
	while (fit != stacktrace.end()) {
		LOG_L(L_DEBUG, "backtrace_symbols: %s", lines[l]);
		if (strncmp(lines[l], "[(nil)]", 20) != 0) {
			fit->symbol = lines[l];
			fit++;
		} else {
			fit = stacktrace.erase(fit);
		}
		l++;
	}
	free(lines);
}

static int CommonStringLength(const std::string& str1, const std::string& str2, int* len)
{
	int n=0, m = std::min(str1.length(), str2.length());
	while (n < m && str1[n] == str2[n]) { n++; }
	if (len != nullptr) {
		*len = n;
	}
	return n;
}


#if !defined(__APPLE__)

/**
 * Finds the base memory address in the running process for all the libraries
 * involved in the crash.
 */
static void FindBaseMemoryAddresses(std::map<std::string,uintptr_t>& binPath_baseMemAddr)
{
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

		// read all lines
		while (!paths_notFound.empty() && (fgets(line, 511, mapsFile) != NULL)) {
			// parse the line
			const int red = sscanf(line, "%lx-%*x %*s %lx %*s %*u %s",
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
	// line examples:
	//
	// ./spring() [0x84b7b5]
	// /usr/lib/libc.so.6(+0x33b20) [0x7fc022c68b20]
	// /usr/lib/libstdc++.so.6(_ZN9__gnu_cxx27__verbose_terminate_handlerEv+0x16d) [0x7fc023553fcd]

	std::string path;
	size_t end   = line.find_last_of('('); // if there is a function name
	if (end == std::string::npos) {
		end = line.find_last_of('['); // if there is only the memory address
		if ((end != std::string::npos) && (end > 0)) {
			end--; // to get rid of the ' ' before the '['
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
static uintptr_t ExtractAddr(const StackFrame& frame)
{
	// frame.symbol examples:
	//
	// ./spring() [0x84b7b5]
	// /usr/lib/libc.so.6(abort+0x16a) [0x7fc022c69e6a]
	// /usr/lib/libstdc++.so.6(+0x5eea1) [0x7fc023551ea1]

	uintptr_t addr = INVALID_ADDR_INDICATOR;
	const std::string& line = frame.symbol;
	size_t begin = line.find_last_of('[');
	size_t end = std::string::npos;
	if (begin != std::string::npos) {
		end = line.find_last_of(']');
	}
	if ((begin != std::string::npos) && (end != std::string::npos)) {
		addr = HexToInt(line.substr(begin+1, end-begin-1).c_str());
	}

	begin = line.find_last_of('(');
	end = line.find_last_of(')');
	if (end - begin != 1) {
		Dl_info info;
		if (dladdr(frame.ip, &info) != 0) {
			addr = (uintptr_t)frame.ip - (uintptr_t)info.dli_fbase;
		}
	}

	return addr;
}

/**
 * @brief TranslateStackTrace
 * @param stacktrace These are the lines and addresses produced by backtrace_symbols()
 * Translates the module and address information from backtrace symbols into a vector of StackFrame objects,
 *   each with its own set of entries representing the function call and any inlined functions for that call.
 */
static void TranslateStackTrace(bool* aiCrash, StackTrace& stacktrace, const int logLevel)
{
	// Extract important data from backtrace_symbols' output
	bool containsDriverSo = false; // OpenGL lib -> graphic problem
	bool containsAIInterfaceSo = false;
	bool containsSkirmishAISo  = false;

	LOG_L(L_DEBUG, "TranslateStackTrace[1]");

	for (auto it = stacktrace.begin(); it != stacktrace.end(); ++it) {
		// prepare for addr2line()
		const std::string path    = ExtractPath(it->symbol);
		const std::string absPath = CreateAbsolutePath(path);
		it->path = absPath;
		it->addr = ExtractAddr(*it);

		LOG_L(L_DEBUG, "symbol = \"%s\", path = \"%s\", absPath = \"%s\", addr = 0x%lx", it->symbol.c_str(), path.c_str(), absPath.c_str(), it->addr);

		// check if there are known sources of fail on the stack
		containsDriverSo = (containsDriverSo || (path.find("libGLcore.so") != std::string::npos));
		containsDriverSo = (containsDriverSo || (path.find("psb_dri.so") != std::string::npos));
		containsDriverSo = (containsDriverSo || (path.find("i965_dri.so") != std::string::npos));
		containsDriverSo = (containsDriverSo || (path.find("fglrx_dri.so") != std::string::npos));
		if (!containsAIInterfaceSo && (absPath.find("Interfaces") != std::string::npos)) {
			containsAIInterfaceSo = true;
		}
		if (!containsSkirmishAISo && (absPath.find("Skirmish") != std::string::npos)) {
			containsSkirmishAISo = true;
		}
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[2]");

	// Linux Graphic drivers are known to fail with moderate OpenGL usage
	if (containsDriverSo) {
		LOG_I(logLevel, "This stack trace indicates a problem with your graphic card driver. "
			  "Please try upgrading or downgrading it. "
			  "Specifically recommended is the latest driver, and one that is as old as your graphic card. "
			  "Also try lower graphic details and disabling Lua widgets in spring-settings.\n");
	}

	// if stack trace contains AI and AI Interface frames,
	// it is very likely that the problem lies in the AI only
	if (containsSkirmishAISo) {
		containsAIInterfaceSo = false;
	}
	if (containsAIInterfaceSo) {
		LOG_I(logLevel, "This stack trace indicates a problem with an AI Interface library.");
		if (aiCrash) *aiCrash = true;
	}
	if (containsSkirmishAISo) {
		LOG_I(logLevel, "This stack trace indicates a problem with a Skirmish AI library.");
		if (aiCrash) *aiCrash = true;
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[3]");

	// Check if addr2line is available
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

	LOG_L(L_DEBUG, "TranslateStackTrace[4]");

	// Detect BaseMemoryAddresses of all Lib's found in the stacktrace
	std::map<std::string,uintptr_t> binPath_baseMemAddr;
	for (auto it = stacktrace.cbegin(); it != stacktrace.cend(); it++) {
		binPath_baseMemAddr[it->path] = 0;
	}
	FindBaseMemoryAddresses(binPath_baseMemAddr);

	LOG_L(L_DEBUG, "TranslateStackTrace[5]");

	// Finally translate it:
	//   This is nested so that the outer loop covers all the entries for one library -- this means fewer addr2line calls.
	for (auto it = binPath_baseMemAddr.cbegin(); it != binPath_baseMemAddr.cend(); it++) {
		const std::string& modulePath = it->first;
		//const uintptr_t&   moduleAddr = it->second;
		const std::string symbolFile = LocateSymbolFile(modulePath);

		LOG_L(L_DEBUG, "modulePath: %s, symbolFile: %s", modulePath.c_str(), symbolFile.c_str());

		std::ostringstream buf;
		buf << ADDR2LINE << " -i -a -f -C --exe=\"" << symbolFile << "\"";

		// insert requested addresses that should be translated by addr2line
		std::queue<size_t> indices;
		int i=0;
		for (auto fit = stacktrace.cbegin(); fit != stacktrace.cend(); fit++) {
			if (fit->path == modulePath) {
				buf << " " << std::hex << fit->addr;
				indices.push(i);
			}
			i++;
		}

		// execute command addr2line, read stdout and write to log-file
		buf << " 2>/dev/null"; // hide error output from spring's pipe
		LOG_L(L_DEBUG, "> %s", buf.str().c_str());
		FILE* cmdOut = popen(buf.str().c_str(), "r");
		if (cmdOut != NULL) {
			const size_t maxLength = 2048;
			char line[maxLength];
			char* res = fgets_addr2line(line, maxLength, cmdOut);
			while (res != NULL) {
				i = indices.front();
				indices.pop();
				// First scan the address and ensure that it matches the one we were expecting
				uintptr_t parsedAddr = 0;
				int matched = sscanf(line, "0x%lx", &parsedAddr);
				if (matched != 1 || parsedAddr != stacktrace[i].addr) {
					LOG_L(L_WARNING, "Mismatched address received from addr2line: 0x%lx != 0x%lx", parsedAddr, stacktrace[i].addr);
					break;
				}
				res = fgets_addr2line(line, maxLength, cmdOut);
				while (res != NULL) {

					if (line[0] == '0' && line[1] == 'x') {
						break; // This is the start of a new address
					}

					StackFunction entry;
					entry.inLine = true; // marked false later if the last entry
					std::string funcname = std::string(line);
					entry.funcname  = funcname.substr(0, funcname.rfind(" (discriminator"));

					if (fgets_addr2line(line, maxLength, cmdOut) == nullptr) { break; }
					entry.fileline = std::string(line);

					stacktrace[i].entries.push_back(entry);

					res = fgets_addr2line(line, maxLength, cmdOut);
				}
				if (!stacktrace[i].entries.empty()) { // Ensure that the last entry in a sequence of results is marked as "not inlined"
					stacktrace[i].entries.rbegin()->inLine = false;
				}
			}
			pclose(cmdOut);
		}
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[6]");

	return;
}

static void LogStacktrace(const int logLevel, StackTrace& stacktrace)
{
	int colFileline = 0;
	const std::string& exe_path = Platform::GetProcessExecutablePath();
	const std::string& cwd_path = Platform::GetOrigCWD();
	for (auto fit = stacktrace.begin(); fit != stacktrace.end(); fit++) {
		for (auto eit = fit->entries.begin(); eit != fit->entries.end(); eit++) {
			eit->abbrev_funcname = eit->funcname;
			std::string fileline = eit->fileline;
			if (fileline[1] == '?') {  // case "??:?", ":?"
				fileline = fit->symbol;  // print raw backtrace_symbol
			}
			eit->abbrev_fileline = fileline;
			int abbrev_start = 0;
			if (fileline[0] == '/') { // See if we can shorten the file/line bit by removing the common path
				if (CommonStringLength(fileline, exe_path, &abbrev_start) > 1) { // i.e. one char for first '/'
					eit->abbrev_fileline = std::string(".../") + fileline.substr(abbrev_start, std::string::npos);
				} else if (CommonStringLength(fileline, cwd_path, &abbrev_start) > 1) {
					eit->abbrev_fileline = std::string("./") + fileline.substr(abbrev_start, std::string::npos);
				}
			}

			if (eit->abbrev_funcname.size() > 100) {
				eit->abbrev_funcname.resize(94);
				eit->abbrev_funcname.append(" [...]");
			}

			colFileline = std::max(colFileline, (int)eit->abbrev_fileline.length());
		}
	}

	// Print out the translated StackTrace
	for (auto fit = stacktrace.cbegin(); fit != stacktrace.cend(); fit++) {
		for (auto eit = fit->entries.begin(); eit != fit->entries.end(); eit++) {
			if (eit->inLine) {
				LOG_I(logLevel, "  <%02u> %*s  %s", fit->level, colFileline, eit->abbrev_fileline.c_str(), eit->abbrev_funcname.c_str());
			} else {
				LOG_I(logLevel, "[%02u]   %*s  %s", fit->level, colFileline, eit->abbrev_fileline.c_str(), eit->abbrev_funcname.c_str());
			}
		}
	}
}

#endif  // !(__APPLE__)


__FORCE_ALIGN_STACK__
static void ForcedExitAfterFiveSecs() {
	boost::this_thread::sleep(boost::posix_time::seconds(5));
	std::exit(-1);
}

__FORCE_ALIGN_STACK__
static void ForcedExitAfterTenSecs() {
	boost::this_thread::sleep(boost::posix_time::seconds(10));
#if defined(__GNUC__)
	std::_Exit(-1);
#else
	std::quick_exit(-1);
#endif
}


typedef struct sigaction sigaction_t;

static sigaction_t& GetSigAction(void (*sigact_handler)(int, siginfo_t*, void*))
{
	static sigaction_t sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	if (sigact_handler == nullptr) {
		// the default signal handler uses the old sa_handler interface, so just identify this case with sigact_handler == null
		sa.sa_handler = SIG_DFL;
	} else {
		sa.sa_flags |= SA_SIGINFO;
		sa.sa_sigaction = sigact_handler;
	}
	return sa;
}


namespace CrashHandler
{
	/**
	 * This is used to obtain the list of symbols using a ucontext_t structure.
	 * So, it is used by both the HaltedStacktrace and the SuspendedStacktrace.
	 * Since this method is pure implementation, it's
	 */
	int thread_unwind(ucontext_t* uc, void** iparray, StackTrace& stacktrace)
	{
		assert(iparray != nullptr);
		assert(&stacktrace != nullptr);

		unw_cursor_t cursor;
		// Effective ucontext_t. If uc not supplied, use unw_getcontext locally. This is appropriate inside signal handlers.
#if defined(__APPLE__)
		unw_context_t thisctx;
		unw_getcontext(&thisctx);
#else
		ucontext_t thisctx;
		if (uc == nullptr) {
			unw_getcontext(&thisctx);
			uc = &thisctx;
		}
#endif
		const int BUFR_SZ = 1000;
		char procbuffer[BUFR_SZ];
		stacktrace.clear();
		stacktrace.reserve(120);
		/*
		 * Note: documentation seems to indicate that uc_link contains a pointer to a "successor" context
		 * that is to be resumed after the current one (as you might expect in a signal handler).
		 * In practice however, the Linux OS seems to re-use the existing context of a thread, and so this approach
		 * does not work. The uc_link is sometimes non-NULL however, but I do not know what the relation is to the
		 * target context (uc).
		 *
		while (uc->uc_link) {
			uc = uc->uc_link;
			LOG_L(L_DEBUG, "Dereferencing uc_link");
		}
		*/
#if defined(__APPLE__)
		int err = unw_init_local(&cursor, &thisctx);
#else
		int err = unw_init_local(&cursor, uc);
#endif
		if (err) {
			LOG_L(L_ERROR, "unw_init_local returned %d", err);
			return 0;
		}
		int i=0;
		while (i < MAX_STACKTRACE_DEPTH && unw_step(&cursor)) {
			StackFrame frame;
			unw_word_t ip;
			unw_word_t offp;
			unw_get_reg(&cursor, UNW_REG_IP, &ip);
			frame.ip = reinterpret_cast<void*>(ip);
			frame.level = i;
			iparray[i] = frame.ip;
			if (!unw_get_proc_name(&cursor, procbuffer, BUFR_SZ-1, &offp)) {
				frame.mangled = std::string(procbuffer);
			} else {
				frame.mangled = std::string("UNW_ENOINFO");
			}
			stacktrace.push_back(frame);
			i++;
		}
		stacktrace.resize(i);
		LOG_L(L_DEBUG, "thread_unwind returned %d frames", i);
		return i;
	}

	static void Stacktrace(bool* aiCrash, pthread_t* hThread = NULL, const char* threadName = NULL, const int logLevel = LOG_LEVEL_ERROR)
	{
#if !(DEDICATED || UNIT_TEST)
		Watchdog::ClearTimer();
#endif

		if (threadName != NULL) {
			LOG_I(logLevel, "Stacktrace (%s) for Spring %s:", threadName, (SpringVersion::GetFull()).c_str());
		} else {
			LOG_I(logLevel, "Stacktrace for Spring %s:", (SpringVersion::GetFull()).c_str());
		}

		StackTrace stacktrace;

		// Get untranslated stacktrace symbols
		{
			// process and analyse the raw stack trace
			void* iparray[MAX_STACKTRACE_DEPTH];
			int numLines = thread_unwind(nullptr, iparray, stacktrace);
			if (numLines > MAX_STACKTRACE_DEPTH) {
				LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");
			}
			char** lines = backtrace_symbols(iparray, numLines); // give them meaningfull names

			ExtractSymbols(lines, stacktrace);
		}

		if (stacktrace.empty()) {
			LOG_I(logLevel, "  Unable to create stacktrace");
			return;
		}

		// Translate it
		TranslateStackTrace(aiCrash, stacktrace, logLevel);

		LogStacktrace(logLevel, stacktrace);
	}

	/**
	 * FIXME: Needs cleaning.
	 * Doesn't use same parameters as "classic" stack trace call that has been used until at least 96.0.
	 */
	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName, const int logLevel)
	{
		//TODO Our custom thread_backtrace() only works on the mainthread.
		//     Use to gdb's libthread_db to get the stacktraces of all threads.
		if (!Threading::IsMainThread(thread) && Threading::GetCurrentThread() != thread) {
			LOG_I(logLevel, "Stacktrace (%s):", threadName.c_str());
			LOG_I(logLevel, "  No Stacktraces for non-MainThread.");
			return;
		}
		PrepareStacktrace();
		Stacktrace(NULL, &thread, threadName.c_str(), logLevel);
		CleanupStacktrace();
	}

	/**
	 *
     * This entry point is tailored for the Watchdog module.
     * Since the thread to be traced may be running, it requires a ThreadControls object in order to suspend/resume the thread.
     * @brief RemoteStacktrace
     */
    void SuspendedStacktrace(Threading::ThreadControls* ctls, const std::string& threadName)
    {
#if !(DEDICATED || UNIT_TEST)
        Watchdog::ClearTimer();
#endif
        assert(ctls != nullptr);
        assert(ctls->handle != 0);
        assert(threadName.size() > 0);

        LOG_L(L_WARNING, "Suspended-thread Stacktrace (%s) for Spring %s:", threadName.c_str(), (SpringVersion::GetFull()).c_str());

        LOG_L(L_DEBUG, "SuspendedStacktrace[1]");

        StackTrace stacktrace;

        // Get untranslated stacktrace symbols
        {
            // process and analyse the raw stack trace
            void* iparray[MAX_STACKTRACE_DEPTH];

			ctls->Suspend();

			const int numLines = thread_unwind(&ctls->ucontext, iparray, stacktrace);

			ctls->Resume();

            LOG_L(L_DEBUG, "SuspendedStacktrace[2]");

            if(numLines > MAX_STACKTRACE_DEPTH) {
                LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");
            }
            char** lines = backtrace_symbols(iparray, numLines); // give them meaningfull names

            ExtractSymbols(lines, stacktrace);
        }

        if (stacktrace.empty()) {
            LOG_L(L_WARNING, "  Unable to create suspended stacktrace");
            return;
        }

        LOG_L(L_DEBUG, "SuspendedStacktrace[3]");

        // Translate symbols into code line numbers
        TranslateStackTrace(NULL, stacktrace, LOG_LEVEL_WARNING);

        LOG_L(L_DEBUG, "SuspendedStacktrace[4]");

        // Print out the translated StackTrace
        LogStacktrace(LOG_LEVEL_WARNING, stacktrace);

    }


    /**
     * This stack trace is tailored for the SIGSEGV / SIGILL / SIGFPE etc signal handler.
     * The thread to be traced is usually in a halted state, but the signal handler can provide siginfo_t and ucontext_t structures to help produce the trace using libunwind.
     * @brief PrepareStacktrace
     */
    void HaltedStacktrace(const std::string& errstr, siginfo_t* siginfo, ucontext_t* ucontext)
    {
        LOG_L(L_ERROR, "Halted Stacktrace for Spring %s using libunwind:", (SpringVersion::GetFull()).c_str());

        assert(siginfo != nullptr);
        assert(ucontext != nullptr);

        StackTrace stacktrace;

        LOG_L(L_DEBUG, "HaltedStacktrace[1]");

        // Get untranslated stacktrace symbols
        {
            // process and analyse the raw stack trace
            void* iparray[MAX_STACKTRACE_DEPTH];

            const int numLines = thread_unwind(nullptr, iparray, stacktrace);

            LOG_L(L_DEBUG, "HaltedStacktrace[2]");

            if(numLines > MAX_STACKTRACE_DEPTH) {
                LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");
            }

            char** lines = backtrace_symbols(iparray, numLines); // give them meaningfull names

            ExtractSymbols(lines, stacktrace);
        }

        LOG_L(L_DEBUG, "HaltedStacktrace[3]");

        if (stacktrace.empty()) {
            LOG_I(LOG_LEVEL_ERROR, "  Unable to create stacktrace");
            return;
        }

        // Translate it
        TranslateStackTrace(NULL, stacktrace, LOG_LEVEL_ERROR);

        LOG_L(L_DEBUG, "HaltedStacktrace[4]");

        // Print out the translated StackTrace. Ignore the frames that occur inside the signal handler (before its line in the trace) -- they are likely some kind of padding or just garbage.
        LogStacktrace(LOG_LEVEL_ERROR, stacktrace);
    }


	void PrepareStacktrace(const int logLevel) {}

	void CleanupStacktrace(const int logLevel) {
		LOG_CLEANUP();
	}

	void HandleSignal(int signal, siginfo_t* siginfo, void* pctx)
	{
		if (signal == SIGINT) {
			// ctrl+c = kill
			LOG("caught SIGINT, aborting");

#ifndef HEADLESS
			// first try a clean exit
			SDL_Event event;
			event.type = SDL_QUIT;
			SDL_PushEvent(&event);
#endif

			// abort after 5sec
			boost::thread(boost::bind(&ForcedExitAfterFiveSecs));
			boost::thread(boost::bind(&ForcedExitAfterTenSecs));
			return;
		}

		// Turn off signal handling for this signal temporarily in order to disable recursive events (e.g. SIGSEGV)
		reentrances++;

		if (reentrances >= 2) {
			sigaction_t& sa = GetSigAction(NULL);
			sigaction(signal, &sa, NULL);
		}

		ucontext_t* uctx = reinterpret_cast<ucontext_t*> (pctx);

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
		LOG_L(L_ERROR, "%s in spring %s", error.c_str(), (SpringVersion::GetFull()).c_str());

		const bool nonFatalSignal = false;
		const bool fatalSignal =
			(signal == SIGSEGV) ||
			(signal == SIGILL)  ||
			(signal == SIGPIPE) ||
			(signal == SIGFPE)  || // causes a endless loop, and process never gets far the causing cmd :<
			(signal == SIGABRT) || // same
			(signal == SIGBUS);

		bool keepRunning = false;
		bool aiCrash = false;

		if (fatalSignal)
			keepRunning = false;
		if (nonFatalSignal)
			keepRunning = true;

		// print stacktrace
		PrepareStacktrace();
		HaltedStacktrace(error, siginfo, uctx);
		CleanupStacktrace();

		// try to clean up
		if (keepRunning) {
			bool cleanupOk = true;

			// try to cleanup AIs
			if (aiCrash) {
				cleanupOk = false;
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

		// exit app if we catched a critical one
		if (!keepRunning) {
			// don't handle any further signals when exiting
			Remove();

			std::ostringstream buf;
			buf << "Spring has crashed:\n"
				<< error << ".\n\n"
				<< "A stacktrace has been written to:\n"
				<< "  " << logOutput.GetFilePath();
			ErrorMessageBox(buf.str(), "Spring crashed", MBF_OK | MBF_CRASH); // this also calls exit()
		}

        // Re-enable signal handling for this signal
		// FIXME: reentrances should be implemented using boost::thread_specific_ptr
        if (reentrances >= 2) {
            sigaction_t& sa = GetSigAction(&HandleSignal);
            sigaction(signal, &sa, NULL);
        }

        reentrances--;

	}

	void OutputStacktrace() {
		PrepareStacktrace();
		Stacktrace(NULL);
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
		//const sigaction_t& sa = GetSigAction(SIG_DFL);
		const sigaction_t& sa = GetSigAction(nullptr);

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
}
