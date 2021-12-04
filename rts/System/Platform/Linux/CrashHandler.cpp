/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

#include <cinttypes> // uintptr_t
#include <cstring> // strnlen
#include <cstdlib>
#include <cstdio>
#include <string>

#include <array>
#include <deque>
#include <vector>
#include <new>

#include <csignal>
#include <execinfo.h>
#include <SDL_events.h>
#include <sys/resource.h> // getrlimits
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <dlfcn.h>

#include "Game/GameVersion.h"
#include "System/FileSystem/FileSystem.h"
#include "System/SpringExitCode.h"
#include "System/Log/ILog.h"
#include "System/Log/LogSinkHandler.h"
#include "System/LogOutput.h"
#include "System/StringUtil.h"
#include "System/Misc/SpringTime.h"
#include "System/Platform/Misc.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"

#ifndef DEDICATED
#include "System/Platform/Watchdog.h"
#endif


#if !defined(__APPLE__)
#define ADDR2LINE "addr2line"
#else
// NB: Mac/CrashHandler.cpp #include's this compilation unit
#define ADDR2LINE "atos"
#endif

#if (defined(__FreeBSD__))
// show function names, demangle
#define ADDR2LINE_ARGS " -f -C"
#else
// unwind inlines, show addresses, see above
#define ADDR2LINE_ARGS " -i -a  -f -C"
#endif


static constexpr int MAX_STACKTRACE_DEPTH = 100;
static constexpr uintptr_t INVALID_ADDR_INDICATOR = 0xFFFFFFFF;
static const char* INVALID_LINE_INDICATOR = "#####";

static const char* glDriverNames[] = {
	"libGLcore.so",
	"psb_dri.so",
	"i965_dri.so",
	"fglrx_dri.so",
	"amdgpu_dri.so",
	"libnvidia-glcore.so"
};
static const char* saiLibWarning = "This stacktrace indicates a problem with a skirmish AI.";
static const char* iaiLibWarning = "This stack trace indicates a problem with an AI Interface library.";
static const char* oglLibWarning =
	"This stacktrace indicates a problem with your graphics card driver, please try "
	"upgrading it. Specifically recommended is the latest version; do not forget to "
	"use a driver removal utility first.";

struct StackEntry {
	std::string fileline;
	std::string funcname;
	std::string filelineAbbrev;
	std::string funcnameAbbrev;
	bool inlined = true;
};

struct StackFrame {
	int                     level = 0;    // level in the original unwinding (inlined functions share the same level as their "caller")
	void*                   ip = nullptr; // instruction pointer from libunwind or backtrace()
	uintptr_t               addr = 0;     // translated address / load address for OS X

	std::string             mangled;      // mangled name retrieved from libunwind (not printed, memoized for debugging)
	std::string             symbol;       // backtrace_symbols output
	std::string             path;         // translated library or module path
	std::vector<StackEntry> entries;      // function names and lines (possibly several inlined) retrieved from addr2line
};

typedef std::vector<StackFrame> StackTrace;
typedef std::pair<std::string, uintptr_t> PathAddrPair;

static int reentrances = 0;

static void TranslateStackTrace(StackTrace& stacktrace, const int logLevel);
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

	std::string absolutePath = std::move(UnQuote(relativePath));

	if (absolutePath.empty() || absolutePath[0] != '/') {
		// remove initial "./"
		if (absolutePath.find("./") == 0)
			absolutePath = absolutePath.substr(2);

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

	const std::string binPath = FileSystem::GetDirectory(binaryFile);
	const std::string binFile = FileSystem::GetFilename(binaryFile);
	// const std::string binExt  = FileSystem::GetExtension(binaryFile);

	if (FileSystem::IsReadableFile(symbolFile = binPath + binFile + ".dbg"))
		return symbolFile;

	if (FileSystem::IsReadableFile(symbolFile = debugPath + binPath + binFile))
		return symbolFile;

	return binaryFile;
}


static bool HaveAddr2Line()
{
	static int i = -1;

	if (i == -1) {
		FILE* f = popen(ADDR2LINE " --help", "r");

		if ((i = (f == nullptr)) == 1)
			return false;

		pclose(f);
	}

	return (i == 0);
}

static const char* ReadPipe(FILE* file, char* line, int maxLength)
{
	const char* res = fgets(line, maxLength, file);

	if (res == nullptr) {
        line[0] = 0;
		return res;
	}

	const int sz = strnlen(line, maxLength);

	// exclude the line-ending
	if (line[sz - 1] == '\n')
		line[sz - 1] = 0;

    LOG_L(L_DEBUG, "[%s] addr2line: %s", __func__, line);
    return res;
}

/**
 * Converts a string containing a hexadecimal value to a decimal value
 * with the size of a pointer.
 * example: "0xfa" -> 26
 */
static uintptr_t HexToInt(const char* hexStr)
{
	static_assert(sizeof(unsigned int long) == sizeof(uintptr_t), "sizeof(unsigned int long) != sizeof(uintptr_t)");
	unsigned long int value = 0;
	sscanf(hexStr, "%lx", &value);
	return (uintptr_t) value;
}

/**
 * Consumes (and frees) the lines produced by backtrace_symbols and puts these into the StackTrace fields
 */
static void ExtractSymbols(char** lines, StackTrace& stacktrace)
{
	int l = 0;
	auto fit = stacktrace.begin();

	while (fit != stacktrace.end()) {
		LOG_L(L_DEBUG, "[%s] backtrace_symbols[%d]=%s", __func__, l, lines[l]);

		if (strncmp(lines[l], "[(nil)]", 20) != 0) {
			fit->symbol = lines[l];
			fit++;
		} else {
			// preserve ordering of remaining symbols
			fit = stacktrace.erase(fit);
		}

		l++;
	}

	free(lines);
}

static int CommonStringLength(const std::string& str1, const std::string& str2)
{
	const size_t m = std::min(str1.length(), str2.length());
	size_t n = 0;

	while (n < m && str1[n] == str2[n]) {
		n++;
	}

	return n;
}




#if !defined(__APPLE__)

/**
 * Finds the base memory address in the running process for all the libraries
 * involved in the crash.
 */
static unsigned int FindBaseMemoryAddresses(const StackTrace& stacktrace, std::array<PathAddrPair, 1024>& baseMemAddrPaths)
{
	// store all paths which we have to find
	std::array<PathAddrPair, 1024> notFoundPaths;

	unsigned int numAddrPairs = 0;

	const auto AddressPred = [&](const StackFrame& sf) { baseMemAddrPaths[numAddrPairs++] = {sf.path, 0}; };
	const auto SortCompare = [](const PathAddrPair& a, const PathAddrPair& b) { return (a.first >  b.first); };
	const auto FindCompare = [](const PathAddrPair& a, const PathAddrPair& b) { return (a.first <  b.first); };
	const auto  UniquePred = [](const PathAddrPair& a, const PathAddrPair& b) { return (a.first == b.first); };

	std::for_each(stacktrace.begin(), stacktrace.begin() + std::min(stacktrace.size(), baseMemAddrPaths.size()), AddressPred);
	// sort in descending order, then reverse s.t. empty strings do not end up in front
	std::sort(baseMemAddrPaths.begin(), baseMemAddrPaths.begin() + numAddrPairs, SortCompare);
	std::reverse(baseMemAddrPaths.begin(), baseMemAddrPaths.begin() + numAddrPairs);

	const auto begUniquePaths = baseMemAddrPaths.begin();
	const auto endUniquePaths = std::unique(begUniquePaths, begUniquePaths + numAddrPairs, UniquePred);

	std::for_each(begUniquePaths, begUniquePaths + (numAddrPairs = endUniquePaths - begUniquePaths), [&](const PathAddrPair& p) { notFoundPaths[&p - begUniquePaths] = p; });

	// /proc/self/maps contains the base addresses for all loaded dynamic libs
	// of the current process + other stuff (which we are not interested in)
	FILE* mapsFile = fopen("/proc/self/maps", "rb");

	if (mapsFile == nullptr)
		return 0;

	// format of /proc/self/maps:
	// (column names)  address           perms offset  dev   inode      pathname
	// (example 32bit) 08048000-08056000 r-xp 00000000 03:0c 64593      /usr/sbin/gpm
	// (example 64bit) ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0   [vsyscall]
	unsigned long int memStartAddr = 0;
	unsigned long int binAddrOffset = 0;

	char binPathName[512];
	char binAddrLine[512];

	// read and parse all lines
	while (numAddrPairs > 0 && fgets(binAddrLine, sizeof(binAddrLine) - 1, mapsFile) != nullptr) {
		if (sscanf(binAddrLine, "%lx-%*x %*s %lx %*s %*u %s", &memStartAddr, &binAddrOffset, binPathName) != 3)
			continue;

		if (binAddrOffset != 0)
			continue;


		// go through all paths of the binaries involved in the stacktrace
		// for each, check if the current line contains this binary
		const auto end = notFoundPaths.begin() + (endUniquePaths - begUniquePaths);
		const auto iter = std::lower_bound(notFoundPaths.begin(), end, PathAddrPair{binPathName, 0}, FindCompare);

		if (iter == end || iter->second == uintptr_t(-1) || iter->first != binPathName)
			continue;

		baseMemAddrPaths[iter - notFoundPaths.begin()].second = memStartAddr;
		notFoundPaths[iter - notFoundPaths.begin()].second = uintptr_t(-1);

		numAddrPairs -= 1;
	}

	fclose(mapsFile);
	return (endUniquePaths - begUniquePaths);
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

	size_t end = line.find_last_of('(');

	if (end == std::string::npos) {
		// if there is only a memory address, get rid of the ' ' before the '['
		end = line.find_last_of('[');
		end -= ((end != std::string::npos) && (end > 0));
	}

	if (end == std::string::npos)
		return INVALID_LINE_INDICATOR;

	return (line.substr(0, end - 0));
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

	if (begin != std::string::npos)
		end = line.find_last_of(']');

	if ((begin != std::string::npos) && (end != std::string::npos))
		addr = HexToInt(line.substr(begin+1, end-begin-1).c_str());

	begin = line.find_last_of('(');
	end = line.find_last_of(')');

	if ((end - begin) != 1) {
		Dl_info info;

		if (dladdr(frame.ip, &info) != 0)
			return (uintptr_t)frame.ip - (uintptr_t)info.dli_fbase;
	}

	return addr;
}

static bool ContainsDriverLib(const std::string& path)
{
	bool ret = false;

	for (size_t i = 0, n = sizeof(glDriverNames) / sizeof(glDriverNames[0]); i < n && !ret; i++) {
		ret = (strstr(path.c_str(), glDriverNames[i]) != nullptr);
	}

	return ret;
}


/**
 * @brief TranslateStackTrace
 * @param stacktrace These are the lines and addresses produced by backtrace_symbols()
 * Translates the module and address information from backtrace symbols into a vector of StackFrame objects,
 *   each with its own set of entries representing the function call and any inlined functions for that call.
 */
static void TranslateStackTrace(StackTrace& stacktrace, const int logLevel)
{
	// Extract important data from backtrace_symbols' output
	bool oglLibFound = false;
	bool iaiLibFound = false;
	bool saiLibFound = false;

	LOG_L(L_DEBUG, "[%s][1]", __func__);

	for (StackFrame& sf: stacktrace) {
		// prepare for addr2line()
		const std::string relPath = ExtractPath(sf.symbol);
		const std::string absPath = CreateAbsolutePath(relPath);

		sf.path = absPath;
		sf.addr = ExtractAddr(sf);

		LOG_L(L_DEBUG, "symbol=\"%s\" relPath=\"%s\" absPath=\"%s\" addr=0x%lx", sf.symbol.c_str(), relPath.c_str(), absPath.c_str(), sf.addr);

		// check if there are known sources of fail on the stack
		if (!oglLibFound)
			oglLibFound |= ContainsDriverLib(relPath);

		if (!iaiLibFound)
			iaiLibFound |= (absPath.find("Interfaces") != std::string::npos);

		if (!saiLibFound)
			saiLibFound |= (absPath.find("Skirmish") != std::string::npos);

	}

	LOG_L(L_DEBUG, "[%s][2]", __func__);


	// if stacktrace contains both AI and AI Interface frames,
	// it is very likely that the problem lies in the AI only
	iaiLibFound &= !saiLibFound;

	if (oglLibFound)
		LOG_I(logLevel, "%s", oglLibWarning);
	if (iaiLibFound)
		LOG_I(logLevel, "%s", iaiLibWarning);
	if (saiLibFound)
		LOG_I(logLevel, "%s", saiLibWarning);

	LOG_L(L_DEBUG, "[%s][3]", __func__);

	// check if addr2line is available
	if (!HaveAddr2Line())
		LOG_L(L_WARNING, "[%s] addr2line not found!", __func__);

	LOG_L(L_DEBUG, "[%s][4]", __func__);


	// detect base memory-addresses of all libs found in the stacktrace
	std::array<PathAddrPair, 1024> baseMemAddrPaths;
	std::deque<size_t> stackFrameIndices;

	std::string       execCommandString;
	std::stringstream execCommandBuffer;

	LOG_L(L_DEBUG, "[%s][5]", __func__);

	// finally translate it; nested s.t. the outer loop covers
	// all the entries for one library (fewer addr2line calls)
	for (unsigned int j = 0, numAddrPairs = FindBaseMemoryAddresses(stacktrace, baseMemAddrPaths); j < numAddrPairs; j++) {
		const PathAddrPair& baseMemAddrPair = baseMemAddrPaths[j];

		const std::string& modulePath = baseMemAddrPair.first;
		const std::string symbolFile = LocateSymbolFile(modulePath);

		if (modulePath.empty()) {
			LOG_L(L_ERROR, "[%s][6] addrPair=%u/%u modulePath=N/A (addr: 0x%lx) symbolFile=%s", __func__, j, numAddrPairs, baseMemAddrPair.second, symbolFile.c_str());
		} else {
			LOG_L(L_DEBUG, "[%s][6] addrPair=%u/%u modulePath=%s (addr: 0x%lx) symbolFile=%s", __func__, j, numAddrPairs, modulePath.c_str(), baseMemAddrPair.second, symbolFile.c_str());
		}

		{
			stackFrameIndices.clear();
			execCommandString.clear();
			execCommandBuffer.str("");

			execCommandBuffer << ADDR2LINE;
			execCommandBuffer << ADDR2LINE_ARGS;
			execCommandBuffer << " -e \"" << symbolFile << "\"";

			// insert requested addresses that should be translated by addr2line
			for (size_t i = 0, n = stacktrace.size(); i < n; i++) {
				const StackFrame& sf = stacktrace[i];

				if (sf.path == modulePath) {
					execCommandBuffer << " " << std::hex << sf.addr;
					stackFrameIndices.push_back(i);
				}
			}

			// hide error output from spring's pipe
			execCommandBuffer << " 2>/dev/null";

			execCommandString = std::move(execCommandBuffer.str());
		}

		FILE* cmdOutputPipe = popen(execCommandString.c_str(), "r");

		// execute addr2line, read stdout via pipe and write to log-file
		LOG_L(L_DEBUG, "> %s", execCommandString.c_str());

		if (cmdOutputPipe == nullptr)
			continue;


		char lineBuf[2048] = {0};
		char lineCtr = 0;

		StackFrame* stackFrame = &stacktrace[stackFrameIndices.front()];
		StackEntry stackEntry;

		while (ReadPipe(cmdOutputPipe, lineBuf, sizeof(lineBuf)) != nullptr) {
			if (lineBuf[0] == '0' && lineBuf[1] == 'x') {
				lineCtr = 0;

				if (!stackFrame->entries.empty()) {
					stackFrame->entries.back().inlined = false;
				} else {
					stackFrame->entries.reserve(8);
				}

				// new address encountered; switch to corresponding frame
				if (!stackFrameIndices.empty()) {
					stackFrame = &stacktrace[stackFrameIndices.front()];
					stackFrameIndices.pop_front();
				}

				uintptr_t parsedAddr = 0;
				uintptr_t stackAddr = stackFrame->addr;

				if (sscanf(lineBuf, "0x%lx", &parsedAddr) != 1 || parsedAddr != stackAddr) {
					LOG_L(L_WARNING, "[%s] mismatched address from addr2line for line \"%s\": 0x%lx != 0x%lx", __func__, lineBuf, parsedAddr, stackAddr);
					break;
				}
			}

			// consecutive pairs of lines after an address form an entry
			switch (lineCtr) {
				case 0: {
					lineCtr = 1;
				} break;
				case 1: {
					stackEntry.funcname = lineBuf;
					stackEntry.funcname = stackEntry.funcname.substr(0, stackEntry.funcname.rfind(" (discriminator"));
					lineCtr = 2;
				} break;
				case 2: {
					stackEntry.fileline = lineBuf;
					stackFrame->entries.push_back(stackEntry);

					// minor hack; store an <fileline, address> entry as well
					snprintf(lineBuf, sizeof(lineBuf), "0x%lx", stackFrame->addr);

					stackEntry.funcname = stackEntry.fileline;
					stackEntry.fileline = lineBuf;
					stackFrame->entries.push_back(stackEntry);

					// next line can be either an address or a function-name
					lineCtr = 1;
				} break;
				default: {
					assert(false);
				} break;
			}
		}

		pclose(cmdOutputPipe);
	}

	LOG_L(L_DEBUG, "[%s][7]", __func__);
}


static void LogStacktrace(const int logLevel, StackTrace& stacktrace)
{
	size_t colFileLine = 0;

	const std::string& exePath = Platform::GetProcessExecutablePath();
	const std::string& cwdPath = Platform::GetOrigCWD();

	for (StackFrame& sf: stacktrace) {
		for (StackEntry& se: sf.entries) {
			std::string fileline = se.fileline;

			// case "??:?" and ":?"; print raw backtrace_symbol
			if (fileline[1] == '?')
				fileline = sf.symbol;

			se.filelineAbbrev = fileline;
			se.funcnameAbbrev = se.funcname;

			size_t abbrevStart = 0;

			if (fileline[0] == '/') {
				// see if we can shorten the file/line bit by removing the common path, i.e. one char for first '/'
				if ((abbrevStart = CommonStringLength(fileline, exePath)) > 1) {
					se.filelineAbbrev = std::string(".../") + fileline.substr(abbrevStart, std::string::npos);
				} else if ((abbrevStart = CommonStringLength(fileline, cwdPath)) > 1) {
					se.filelineAbbrev = std::string("./") + fileline.substr(abbrevStart, std::string::npos);
				}
			}

			colFileLine = std::max(colFileLine, se.filelineAbbrev.length());
		}
	}

	// print out the translated trace
	for (const StackFrame& sf: stacktrace) {
		for (const StackEntry& se: sf.entries) {
			if (se.inlined) {
				LOG_I(logLevel, "  <%02u> %*s  %s", sf.level, int(colFileLine), se.filelineAbbrev.c_str(), se.funcnameAbbrev.c_str());
			} else {
				LOG_I(logLevel, "[%02u]   %*s  %s", sf.level, int(colFileLine), se.filelineAbbrev.c_str(), se.funcnameAbbrev.c_str());
			}
		}
	}
}

#endif  // !(__APPLE__)




static void ForcedExit(int secs)
{
	std::function<void()> func = [secs]() {
		spring::this_thread::sleep_for(std::chrono::seconds(secs));
		std::exit(spring::EXIT_CODE_KILLED);
	};
	spring::thread thread{func};

	assert(thread.joinable());
	thread.detach();
}

typedef struct sigaction sigaction_t;
typedef void (*sigact_handler_t)(int, siginfo_t*, void*);

static sigaction_t& GetSigAction(sigact_handler_t sigact_handler)
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

		unw_cursor_t cursor;

#if (defined(__arm__) || defined(__APPLE__))
		// ucontext_t and unw_context_t are not aliases here
		unw_context_t thisctx;
		unw_getcontext(&thisctx);
#else
		// Effective ucontext_t. If uc not supplied, use unw_getcontext
		// locally. This is appropriate inside signal handlers.
		ucontext_t thisctx;

		if (uc == nullptr) {
			unw_getcontext(&thisctx);
			uc = &thisctx;
		}
#endif


		char procbuffer[1024];

		stacktrace.clear();
		stacktrace.reserve(MAX_STACKTRACE_DEPTH);
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

#if (defined(__arm__) || defined(__APPLE__))
		const int err = unw_init_local(&cursor, &thisctx);
#else
		const int err = unw_init_local(&cursor, uc);
#endif

		if (err != 0) {
			LOG_L(L_ERROR, "unw_init_local returned %d", err);
			return 0;
		}

		for (int i = 0; i < MAX_STACKTRACE_DEPTH && unw_step(&cursor); i++) {
			unw_word_t ip;
			unw_word_t offp;
			unw_get_reg(&cursor, UNW_REG_IP, &ip);

			stacktrace.emplace_back();
			StackFrame& frame = stacktrace.back();

			frame.ip = (iparray[i] = reinterpret_cast<void*>(ip));
			frame.level = i;

			if (!unw_get_proc_name(&cursor, procbuffer, sizeof(procbuffer) - 1, &offp)) {
				frame.mangled = std::string(procbuffer);
			} else {
				frame.mangled = std::string("UNW_ENOINFO");
			}
		}

		return (int(stacktrace.size()));
	}


	static void Stacktrace(pthread_t* hThread = nullptr, const char* threadName = nullptr, const int logLevel = LOG_LEVEL_ERROR)
	{
#if !(DEDICATED || UNIT_TEST)
		Watchdog::ClearTimer();
#endif

		if (threadName != nullptr) {
			LOG_I(logLevel, "Stacktrace (%s) for Spring %s:", threadName, (SpringVersion::GetFull()).c_str());
		} else {
			LOG_I(logLevel, "Stacktrace for Spring %s:", (SpringVersion::GetFull()).c_str());
		}

		StackTrace stacktrace;

		{
			// process and analyse the raw stack trace
			void* iparray[MAX_STACKTRACE_DEPTH];
			const int numLines = thread_unwind(nullptr, iparray, stacktrace);

			if (numLines > MAX_STACKTRACE_DEPTH)
				LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");

			// get untranslated stacktrace symbols; give them meaningful names
			ExtractSymbols(backtrace_symbols(iparray, std::min(numLines, MAX_STACKTRACE_DEPTH)), stacktrace);
		}

		if (stacktrace.empty()) {
			LOG_I(logLevel, "  Unable to create stacktrace");
			return;
		}

		TranslateStackTrace(stacktrace, logLevel);
		LogStacktrace(logLevel, stacktrace);
	}


	/**
	 * FIXME: Needs cleaning.
	 * Doesn't use same parameters as "classic" stack trace call that has been used until at least 96.0.
	 */
	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName, const int logLevel)
	{
		//TODO Our custom thread_backtrace() only works on the mainthread.
		//     Use gdb's libthread_db to get the stacktraces of all threads.
		if (!Threading::IsMainThread(thread) && Threading::GetCurrentThread() != thread) {
			LOG_I(logLevel, "Stacktrace (%s):", threadName.c_str());
			LOG_I(logLevel, "  No Stacktraces for non-MainThread.");
			return;
		}

		PrepareStacktrace();
		Stacktrace(&thread, threadName.c_str(), logLevel);
		CleanupStacktrace();
	}


	/**
	 *
     * This entry point is tailored for the Watchdog module.
     * Since the thread to be traced may be running, it requires a ThreadControls object in order to suspend/resume the thread.
     * @brief RemoteStacktrace
     */
    void SuspendedStacktrace(Threading::ThreadControls* ctls, const char* threadName)
    {
#if !(DEDICATED || UNIT_TEST)
		Watchdog::ClearTimer();
#endif
		assert(ctls != nullptr);
		assert(ctls->handle != 0);
		assert(threadName[0] != 0);

		LOG_L(L_WARNING, "Suspended-thread Stacktrace (%s) for Spring %s:", threadName, (SpringVersion::GetFull()).c_str());
		LOG_L(L_DEBUG, "[%s][1]", __func__);

		StackTrace stacktrace;

		{
			// process and analyse the raw stack trace
			void* iparray[MAX_STACKTRACE_DEPTH];

			ctls->Suspend();
			const int numLines = thread_unwind(&ctls->ucontext, iparray, stacktrace);
			ctls->Resume();

			LOG_L(L_DEBUG, "[%s][2]", __func__);

			if (numLines > MAX_STACKTRACE_DEPTH)
				LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");

			// get untranslated stacktrace symbols; give them meaningful names
			ExtractSymbols(backtrace_symbols(iparray, numLines), stacktrace);
		}

		if (stacktrace.empty()) {
			LOG_L(L_WARNING, "  Unable to create suspended stacktrace");
			return;
		}

		LOG_L(L_DEBUG, "[%s][3]", __func__);

		// Translate symbols into code line numbers
		TranslateStackTrace(stacktrace, LOG_LEVEL_WARNING);

		LOG_L(L_DEBUG, "[%s][4]", __func__);

		LogStacktrace(LOG_LEVEL_WARNING, stacktrace);
    }


	/**
	 * This stack trace is tailored for the SIGSEGV / SIGILL / SIGFPE etc signal handler.
	 * The thread to be traced is usually in a halted state, but the signal handler can
	 * provide siginfo_t and ucontext_t structures to help produce the trace using libunwind.
	 * @brief PrepareStacktrace
	 */
    void HaltedStacktrace(siginfo_t* siginfo, ucontext_t* ucontext, const char* signame)
    {
		LOG_L(L_ERROR, "Halted Stacktrace for Spring %s (%s) using libunwind:", (SpringVersion::GetFull()).c_str(), signame);

		assert(siginfo != nullptr);
		assert(ucontext != nullptr);

		StackTrace stacktrace;

		LOG_L(L_DEBUG, "[%s][1]", __func__);

		{
			// process and analyse the raw stack trace
			void* iparray[MAX_STACKTRACE_DEPTH];
			const int numLines = thread_unwind(nullptr, iparray, stacktrace);

			LOG_L(L_DEBUG, "[%s][2]", __func__);

			if (numLines > MAX_STACKTRACE_DEPTH)
				LOG_L(L_ERROR, "thread_unwind returned more lines than we allotted space for!");

			// get untranslated stacktrace symbols; give them meaningful names
			ExtractSymbols(backtrace_symbols(iparray, std::min(numLines, MAX_STACKTRACE_DEPTH)), stacktrace);
        }

		LOG_L(L_DEBUG, "[%s][3]", __func__);

		if (stacktrace.empty()) {
			LOG_I(LOG_LEVEL_ERROR, "  Unable to create stacktrace");
			return;
		}

		TranslateStackTrace(stacktrace, LOG_LEVEL_ERROR);

		LOG_L(L_DEBUG, "[%s][4]", __func__);

		// Print out the translated StackTrace. Ignore the frames that occur inside the signal handler (before its line in the trace) -- they are likely some kind of padding or just garbage.
		LogStacktrace(LOG_LEVEL_ERROR, stacktrace);
    }


	void PrepareStacktrace(const int logLevel) {}
	void CleanupStacktrace(const int logLevel) { LOG_CLEANUP(); }


	void HandleSignal(int signal, siginfo_t* siginfo, void* pctx)
	{
		switch (signal) {
			case SIGINT: {
				// ctrl+c = kill
				LOG("[%s] caught SIGINT, aborting", __func__);

				// first try a clean exit
				SDL_Event event;
				event.type = SDL_QUIT;
				SDL_PushEvent(&event);

				// force an exit if no such luck
				ForcedExit(5);
				return;
			} break;
			case SIGCONT: {
				#ifndef DEDICATED
				Watchdog::ClearTimers(false, false);
				#endif
				LOG("[%s] caught SIGCONT, resuming", __func__);
				return;
			} break;
			default: {
			} break;
		}


		// turn off signal handling for this signal temporarily in order to disable recursive events (e.g. SIGSEGV)
		if ((++reentrances) >= 2) {
			sigaction_t& sa = GetSigAction(nullptr);
			sigaction(signal, &sa, nullptr);
		}

		logSinkHandler.SetSinking(false);


		ucontext_t* uctx = reinterpret_cast<ucontext_t*>(pctx);

		const char* sigdesc = strsignal(signal);
		const char* signame = "";

		// append the signal name (no OS function to map signum to signame)
		switch (signal) {
			case SIGSEGV: { signame = "SIGSEGV"; } break;
			case SIGILL : { signame = "SIGILL" ; } break;
			case SIGPIPE: { signame = "SIGPIPE"; } break;
			case SIGIO  : { signame = "SIGIO"  ; } break;
			case SIGFPE : { signame = "SIGFPE" ; } break; // causes endless loop, process never gets past the signal trigger
			case SIGABRT: { signame = "SIGABRT"; } break; // same
			case SIGBUS : { signame = "SIGBUS" ; } break;
			default     : {                      } break;
		}

		LOG_L(L_ERROR, "%s in Spring %s", sigdesc, (SpringVersion::GetFull()).c_str());

		// print stacktrace
		PrepareStacktrace();
		HaltedStacktrace(siginfo, uctx, signame);
		CleanupStacktrace();

		if (signal != SIGIO) {
			char buf[8192];
			char* ptr = buf;

			ptr += snprintf(buf, sizeof(buf) - (ptr - buf), "%s", "Spring has crashed:\n");
			ptr += snprintf(buf, sizeof(buf) - (ptr - buf), "%s.\n\n", sigdesc);
			ptr += snprintf(buf, sizeof(buf) - (ptr - buf), "%s.\n\n", "A stacktrace has been written to:\n");
			ptr += snprintf(buf, sizeof(buf) - (ptr - buf), "%s.\n\n", (logOutput.GetFilePath()).c_str());

			// fatal signal, try to clean up
			Remove();
			// exit if we cought a critical signal; don't handle any further signals when exiting
			ErrorMessageBox(buf, "Spring crashed", MBF_OK | MBF_CRASH);
		} else {
			logSinkHandler.SetSinking(true);
		}

		// re-enable signal handling for this signal
		// FIXME: reentrances should be implemented using __thread
		if (reentrances >= 2) {
			sigaction_t& sa = GetSigAction(&HandleSignal);
			sigaction(signal, &sa, nullptr);
		}
	}

	void OutputStacktrace() {
		PrepareStacktrace();
		Stacktrace(nullptr);
		CleanupStacktrace();
	}

	void NewHandler() {
		std::set_new_handler(nullptr); // prevent recursion; OST or EMB might perform hidden allocs
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

		sigaction(SIGSEGV, &sa, nullptr); // segmentation fault
		sigaction(SIGILL,  &sa, nullptr); // illegal instruction
		sigaction(SIGPIPE, &sa, nullptr); // maybe some network error
		sigaction(SIGIO,   &sa, nullptr); // who knows?
		sigaction(SIGFPE,  &sa, nullptr); // div0 and more
		sigaction(SIGABRT, &sa, nullptr);
		sigaction(SIGINT,  &sa, nullptr);
		// sigaction(SIGSTOP, &sa, nullptr); // cannot be caught
		sigaction(SIGCONT, &sa, nullptr);
		sigaction(SIGBUS,  &sa, nullptr); // on macosx EXC_BAD_ACCESS (mach exception) is translated to SIGBUS

		std::set_new_handler(NewHandler);
	}

	void Remove() {
		//const sigaction_t& sa = GetSigAction(SIG_DFL);
		const sigaction_t& sa = GetSigAction(nullptr);

		sigaction(SIGSEGV, &sa, nullptr);
		sigaction(SIGILL,  &sa, nullptr);
		sigaction(SIGPIPE, &sa, nullptr);
		sigaction(SIGIO,   &sa, nullptr);
		sigaction(SIGFPE,  &sa, nullptr);
		sigaction(SIGABRT, &sa, nullptr);
		sigaction(SIGINT,  &sa, nullptr);
		// sigaction(SIGSTOP, &sa, nullptr);
		sigaction(SIGCONT, &sa, nullptr);
		sigaction(SIGBUS,  &sa, nullptr);

		std::set_new_handler(nullptr);
	}
}
