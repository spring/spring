/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#ifdef SYNCDEBUG

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sstream>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Logger.h"
#include "System/StringUtil.h"
#include "System/SafeCStrings.h"

#ifdef _MSC_VER
#include <Windows.h>
#include <imagehlp.h>
#include "System/Platform/CrashHandler.h"
#endif

#ifdef WIN32
// msvcrt doesn't have thread safe ctime_r
# define ctime_r(tm, buf) ctime(tm)
#endif


extern "C" void get_executable_name(char *output, int size);


/**
 * @brief name of the addr2line binary
 */
#define ADDR2LINE "addr2line"


/**
 * @brief name of the c++filt binary
 *
 * Only used on MinGW to work around an addr2line bug.
 */
#define CPPFILT "c++filt"


/**
 * @brief log function
 *
 * Write a printf-style formatted message to the temporary logfile buffer.
 * (which is later to be filtered through addr2line)
 *
 * The filtering is done by taking everything between { and } as being a
 * hexadecimal address into the executable. This address is literally passed
 * to addr2line, which converts it to a function, filename & line number.
 * The "{...}" string is then fully replaced by "function [filename:lineno]".
 */
void CLogger::AddLine(const char* fmt, ...)
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(logmutex);
	char buf[500]; // same size as buffer in infolog

	va_list argp;
	va_start(argp, fmt);
	VSNPRINTF(buf, sizeof(buf), fmt, argp);
	buf[sizeof(buf) - 1] = 0;
	va_end(argp);

	buffer.push_back(buf);

	// Buffer size is 2900 because that makes about the largest commandline
	// we can pass to addr2line on systems limiting the commandline to 32k
	// characters. (11 bytes per hexadecimal address gives 2900*11 = 31900)
	if (buffer.size() >= 2900)
		FlushBuffer();
}


/**
 * @brief close one logging session
 *
 * Writes the buffered data to file after filtering it through addr2line.
 */
void CLogger::CloseSession()
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(logmutex);

	if (logfile || !buffer.empty()) {
		FlushBuffer();
		fclose(logfile);
		logfile = NULL;
	}
}


/**
 * @brief flushes the buffer
 *
 * This is the actual worker function. It opens the log file if it wasn't yet
 * open, composes the addr2line commandline, pipes the buffer through it and
 * writes the output of that - after some formatting - to the real logfile.
 */
void CLogger::FlushBuffer()
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(logmutex);

	char buf1[4096], buf2[4096];
	char* nl;

	// Open logfile if it's not open.

	if (!logfile) {
		assert(filename);
		if (!(logfile = fopen(filename, "a")))
			return;

		time_t t;
		time(&t);
		char* ct = ctime_r(&t, buf1);
		if ((nl = strchr(ct, '\n'))) *nl = 0;
		fprintf(logfile, "\n===> %s <===\n", ct);
	}

	// Get executable name if we didn't have it yet.

	if (exename.empty()) {
		get_executable_name(buf1, sizeof(buf1));
		int len = strlen(buf1);
		strncpy(buf2, buf1, sizeof(buf2));
		buf2[sizeof(buf2)-1] = '\0'; // make sure the string is terminated
		if (len > 4 && buf2[len-4] == '.')
			buf2[len-4] = 0;
		// NOTE STRCAT_T: will always terminate destination with '/0'
		//   2nd param: destination buffer size
		STRCAT_T(buf2, sizeof(buf2), ".dbg");

		fprintf(logfile, "trying debug symbols file: '%s'\n", buf2);

		struct stat tmp;
		if (stat(buf2, &tmp) == 0) {
			exename = buf2;
		} else {
			exename = buf1;
		}

		if (exename.empty())
			return;
		fprintf(logfile, "executable name: '%s'\n", exename.c_str());
	}

#ifdef _MSC_VER
	for (auto it = buffer.begin(); it != buffer.end(); ++it) {
		const int open = it->find('{');
		const int close = it->find('}', open + 1);
		std::stringstream ss;
		if (open != std::string::npos && close != std::string::npos) {
			void* p;
			ss << std::hex << it->substr(open + 1, close - open - 1) << std::dec;
			ss >> p;

			const int SYMLENGTH = 4096;
			char symbuf[sizeof(SYMBOL_INFO) + SYMLENGTH];
			PSYMBOL_INFO pSym = reinterpret_cast<SYMBOL_INFO*>(symbuf);
			pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
			pSym->MaxNameLen = SYMLENGTH;

			if (CrashHandler::InitImageHlpDll() &&
				SymFromAddr(GetCurrentProcess(), (DWORD64)p, nullptr, pSym)) {

				IMAGEHLP_LINE64 line;
				DWORD displacement;
				line.SizeOfStruct = sizeof(line);
				SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64) p, &displacement, &line);

				fprintf(logfile, "%s%s:%d [%p]%s\n", it->substr(0, open).c_str(), pSym->Name, line.LineNumber, p, it->substr(close + 1).c_str());
			}
		} else {
			fprintf(logfile, "%s\n", it->c_str());
		}
	}
#else
	// Compose addr2line command.

	std::stringstream command;
	std::vector<std::string>::iterator it;
	bool runTheCommand = false;

	command << ADDR2LINE << " \"--exe=" << exename << "\" --functions --demangle --inline";

	for (it = buffer.begin(); it != buffer.end(); ++it) {
		const int open = it->find('{');
		const int close = it->find('}', open + 1);
		if (open != std::string::npos && close != std::string::npos) {
			command << " " << it->substr(open + 1, close - open - 1);
			runTheCommand = true;
		}
	}

	if (runTheCommand) {
		// We got some addresses, so run the addr2line command.
		// (This is usually the branch taken by the host)

		fprintf(logfile, "addr2line command : '%s'\n", command.str().c_str());

		// Open pipe to the addr2line command.

		FILE* p = popen(command.str().c_str(), "r");

		if (!p) {
			fprintf(logfile, "  %s\n", strerror(errno));
			runTheCommand = false;
		} else {
			// Pipe the buffer through the addr2line command.
			for (it = buffer.begin(); it != buffer.end(); ++it) {
				const int open = it->find('{');
				const int close = it->find('}', open + 1);
				if (open != std::string::npos && close != std::string::npos) {
					fgets(buf1, sizeof(buf1), p);
					fgets(buf2, sizeof(buf2), p);
					CppFilt(buf1, sizeof(buf1));
					if ((nl = strchr(buf1, '\n'))) *nl = 0;
					if ((nl = strchr(buf2, '\n'))) *nl = 0;
					fprintf(logfile, "%s%s [%s]%s\n", it->substr(0, open).c_str(), buf1, buf2, it->substr(close + 1).c_str());
				} else {
					fprintf(logfile, "%s\n", it->c_str());
				}
			}

			// Close pipe & clear buffer.
			pclose(p);
		}
	}

	if (!runTheCommand) {
		// Just dump the buffer to the file.
		// (this is usually the branch taken by the clients)
		for (it = buffer.begin(); it != buffer.end(); ++it) {
			fprintf(logfile, "%s\n", it->c_str());
		}
	}
#endif

	buffer.clear();
}


/**
 * @brief demangles sym
 *
 * On MinGW, checks if sym needs demangling by some very simple heuristic and
 * filters it through c++filt if it needs to be demangled.
 *
 * It's a workaround for MinGW GNU addr2line 2.15.91, which somehow strips
 * underscores from the symbol before trying to demangle them, resulting
 * no demangling ever happening.
 */
void CLogger::CppFilt(char* sym, int size)
{
#ifdef __MINGW32__

	// The heuristic :-)
	if (sym[0] != 'Z')
		return;

	// Compose command & add the missing underscore
	std::stringstream command;
	command << CPPFILT << " \"_" << sym << '"';

	// Do the filtering. Silently ignore any errors.
	FILE* p = popen(command.str().c_str(), "r");
	if (p) {
		fgets(sym, size, p);
		pclose(p);
	}

#endif // __MINGW32__
}

#endif // SYNCDEBUG
