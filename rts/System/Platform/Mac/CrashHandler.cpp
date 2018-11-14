/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

#if defined(__APPLE__)
// ### Unix(compliant) CrashHandler START

#include <AvailabilityMacros.h>

//! Same as Linux, so just include the .cpp
#include "System/Platform/Linux/CrashHandler.cpp"

#include <libproc.h>
#include <limits.h>
#include <unistd.h>

#ifndef ADDR2LINE
#error "ADDR2LINE undefined"
#endif


static bool HaveAddr2LineMac()
{
	static int i = -1;

	if (i == -1) {
		FILE* f = popen(ADDR2LINE " --help 2>/dev/null", "r");

		if ((i = (f == nullptr)) == 1)
			return false;

		pclose(f);
	}

	return (i == 0);
}

/**
 * @brief TranslateStackTrace
 * @param stacktrace These are the lines and addresses produced by backtrace_symbols()
 * Translates the module and address information from backtrace symbols into a vector of StackFrame objects,
 *   each with its own set of entries representing the function call and any inlined functions for that call.
 */
static void TranslateStackTrace(StackTrace& stacktrace, const int logLevel)
{
	LOG_L(L_DEBUG, "[%s][1] #stacktrace=%u", __func__, static_cast<unsigned int>(stacktrace.size()));

	const pid_t pid = getpid();

	typedef std::pair<uintptr_t, std::string> AddrPathPair;
	// typedef std::function<bool(const AddrPathPair&, const AddrPathPair&)> AddrPathComp;

	std::array<AddrPathPair, 1024> addrPathMap;
	std::deque<size_t> stackFrameIndices;
	std::ostringstream execCommandBuffer;
	std::string execCommandString;

	for (StackFrame& stackFrame: stacktrace) {
		Dl_info info;
		stackFrame.addr = (dladdr(stackFrame.ip, &info) != 0) ? (uintptr_t)info.dli_fbase : INVALID_ADDR_INDICATOR;

		// Find library path
		// http://stackoverflow.com/questions/8240436/filename-of-memory-mapped-libraries-osx
		// Parsing output of vmmap will be a pain as it doesn't flush buffers (fgets hangs).
		// Painful workaround http://www.programmingforums.org/thread36915-2.html
		char path[PATH_MAX + 1];
		const int count = proc_regionfilename(pid, (uintptr_t)stackFrame.ip, path, sizeof(path) - 1);
		if (count > 0) {
			path[count] = '\0';
			stackFrame.path = path;
		} else {
			// dyld_shared_cache ? whats that ?
			stackFrame.path = "";
		}

		LOG_L(L_DEBUG, "\tsymbol = \"%s\", path = \"%s\", addr = 0x%lx", stackFrame.symbol.c_str(), path, stackFrame.ip);
	}

	LOG_L(L_DEBUG, "[%s][2]", __func__);

	// check if atos is available
	if (!HaveAddr2LineMac())
		LOG_L(L_WARNING, " %s not found!", ADDR2LINE);

	LOG_L(L_DEBUG, "[%s][3]", __func__);

	unsigned int numAddrPairs = 0;
	for (const StackFrame& stackFrame: stacktrace) {
		if (stackFrame.path.empty()) {
			StackEntry entry;
			entry.funcname = stackFrame.symbol;
			stacktrace[stackFrame.level].entries.push_back(entry);
			continue;
		}

		if (numAddrPairs >= addrPathMap.size())
			continue;

		addrPathMap[numAddrPairs++] = {stackFrame.addr, stackFrame.path};
	}

	// sort by address
	std::sort(addrPathMap.begin(), addrPathMap.begin() + numAddrPairs, [](const AddrPathPair& a, const AddrPathPair& b) { return (a.first < b.first); });

	LOG_L(L_DEBUG, "[%s][4]", __func__);

	// Finally translate it:
	//   This is nested so that the outer loop covers all the entries for one library -- this means fewer atos calls.
	for (unsigned int j = 0; j < numAddrPairs; j++) {
		const AddrPathPair& addrPathPair = addrPathMap[j];
		const std::string& modulePath = addrPathPair.second;

		LOG_L(L_DEBUG, "\tmodulePath: %s", modulePath.c_str());

		execCommandBuffer.str("");
		execCommandString.clear();
		stackFrameIndices.clear();

		execCommandBuffer << ADDR2LINE << " -o " << modulePath << " -arch x86_64 -l " << std::hex << addrPathPair.first;

		// insert requested addresses that should be translated by atos
		int i = 0;

		for (const StackFrame& stackFrame: stacktrace) {
			if (stackFrame.path == modulePath) {
				execCommandBuffer << " " << std::hex << stackFrame.ip;
				stackFrameIndices.push_back(i);
			}
			i++;
		}

		// execute command atos, read stdout and write to log-file
		// hide error output from spring's pipe
		execCommandBuffer << " 2>/dev/null";
		execCommandString = std::move(execCommandBuffer.str());

		LOG_L(L_DEBUG, "\t> %s", execCommandString.c_str());

		FILE* cmdOut = popen(execCommandString.c_str(), "r");

		if (cmdOut != nullptr) {
			char line[2048];

			while (fgets(line, sizeof(line), cmdOut) != nullptr) {
				i = stackFrameIndices.front();
				stackFrameIndices.pop_front();

				StackEntry entry;
				entry.funcname = std::string(line);

				stacktrace[i].entries.push_back(entry);
			}

			pclose(cmdOut);
		}
	}

	LOG_L(L_DEBUG, "[%s][5]", __func__);
}

static void LogStacktrace(const int logLevel, StackTrace& stacktrace)
{
	// Print out the translated StackTrace
	for (const StackFrame& stackFrame: stacktrace) {
		for (const StackEntry& stackEntry: stackFrame.entries) {
			LOG_I(logLevel, "[%02u]   %s", stackFrame.level, stackEntry.funcname.c_str());
		}
	}
}

// ### Unix(compliant) CrashHandler END
#else
// ### Fallback CrashHandler (old Apple) START

#warning Fallback CrashHandler used

namespace CrashHandler {
	void Install() {}
	void Remove() {}

	void Stacktrace(Threading::NativeThreadHandle thread, const std::string& threadName) {}
	void PrepareStacktrace() {}
	void CleanupStacktrace() {}

	void OutputStacktrace() {}
};

// ### Fallback CrashHandler (old Apple) END
#endif
