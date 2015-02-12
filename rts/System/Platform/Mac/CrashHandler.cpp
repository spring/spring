/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/CrashHandler.h"

#if defined(__APPLE__) 
// ### Unix(compliant) CrashHandler START

#include <AvailabilityMacros.h>

//! Same as Linux
#include "System/Platform/Linux/CrashHandler.cpp"
#include <libproc.h>
#include <limits.h>

#define ADDR2LINE "atos"

/**
 * @brief TranslateStackTrace
 * @param stacktrace These are the lines and addresses produced by backtrace_symbols()
 * Translates the module and address information from backtrace symbols into a vector of StackFrame objects,
 *   each with its own set of entries representing the function call and any inlined functions for that call.
 */
static void TranslateStackTrace(bool* aiCrash, StackTrace& stacktrace, const int logLevel)
{
	LOG_L(L_DEBUG, "TranslateStackTrace[1]");

	pid_t pid = getpid();
	char path[PATH_MAX + 1];
	for (auto it = stacktrace.begin(); it != stacktrace.end(); ++it) {
		Dl_info info;
		it->addr = (dladdr(it->ip, &info) != 0) ? (uintptr_t)info.dli_fbase : INVALID_ADDR_INDICATOR;

		// Find library path
		// http://stackoverflow.com/questions/8240436/filename-of-memory-mapped-libraries-osx
		// Parsing output of vmmap will be a pain as it doesn't flush buffers (fgets hangs).
		// Painful workaround http://www.programmingforums.org/thread36915-2.html
		int count = proc_regionfilename(pid, (uintptr_t)it->ip, path, PATH_MAX);
		if (count > 0) {
			path[count] = '\0';
			it->path = path;
		} else {
			// dyld_shared_cache ? whats that ?
			it->path = "";
		}

		LOG_L(L_DEBUG, "symbol = \"%s\", path = \"%s\", addr = 0x%lx", it->symbol.c_str(), path, it->ip);
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[2]");

	// Check if atos is available
	static int addr2line_found = -1;
	if (addr2line_found < 0)
	{
		FILE* cmdOut = popen(ADDR2LINE " --help 2>/dev/null", "r");
		if (cmdOut == NULL) {
			addr2line_found = false;
		} else {
			addr2line_found = true;
			pclose(cmdOut);
		}
	}
	if (!addr2line_found) {
		LOG_L(L_WARNING, " %s not found!", ADDR2LINE);
		return;
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[3]");

	std::map<uintptr_t,std::string> addr_path;
	for (auto it = stacktrace.cbegin(); it != stacktrace.cend(); ++it) {
		if (it->path.empty()) {
			StackFunction entry;
			entry.funcname = it->symbol;
			stacktrace[it->level].entries.push_back(entry);
		} else {
			addr_path[it->addr] = it->path;
		}
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[4]");

	// Finally translate it:
	//   This is nested so that the outer loop covers all the entries for one library -- this means fewer atos calls.
	for (auto it = addr_path.cbegin(); it != addr_path.cend(); ++it) {
		const std::string& modulePath = it->second;

		LOG_L(L_DEBUG, "modulePath: %s", modulePath.c_str());

		std::ostringstream buf;
		buf << ADDR2LINE << " -o " << modulePath << " -arch x86_64 -l " << std::hex << it->first;

		// insert requested addresses that should be translated by atos
		std::queue<size_t> indices;
		int i=0;
		for (auto fit = stacktrace.cbegin(); fit != stacktrace.cend(); fit++) {
			if (fit->path == modulePath) {
				buf << " " << std::hex << fit->ip;
				indices.push(i);
			}
			i++;
		}

		// execute command atos, read stdout and write to log-file
		buf << " 2>/dev/null"; // hide error output from spring's pipe
		LOG_L(L_DEBUG, "> %s", buf.str().c_str());
		FILE* cmdOut = popen(buf.str().c_str(), "r");
		if (cmdOut != NULL) {
			const size_t maxLength = 2048;
			char line[maxLength];
			while (fgets_addr2line(line, maxLength, cmdOut) != NULL) {
				i = indices.front();
				indices.pop();

				StackFunction entry;
				entry.funcname = std::string(line);

				stacktrace[i].entries.push_back(entry);
			}
			pclose(cmdOut);
		}
	}

	LOG_L(L_DEBUG, "TranslateStackTrace[5]");

	return;
}

static void LogStacktrace(const int logLevel, StackTrace& stacktrace)
{
	// Print out the translated StackTrace
	unsigned numLine = 0;
	unsigned hiddenLines = 0;

	for (auto fit = stacktrace.cbegin(); fit != stacktrace.cend(); fit++) {
		for (auto eit = fit->entries.begin(); eit != fit->entries.end(); eit++) {
			LOG_I(logLevel, "[%02u]   %s", fit->level, eit->funcname.c_str());
		}
	}
}

// ### Unix(compliant) CrashHandler END
#else
// ### Fallback CrashHandler (old Apple) START

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
