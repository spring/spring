/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOGGER_H
#define LOGGER_H

#ifdef SYNCDEBUG

#include <stdio.h>
#include <string>
#include <vector>
#include "System/Threading/SpringThreading.h"

/**
 * @brief logging class for sync debugger
 *
 * Logging class with the special feature that it can replace "{hex address}"
 * with "function [filename:lineno]".
 *
 * Specifically written for & used by the sync debugger.
 */
class CLogger {

	public:

		CLogger() : filename(NULL), logfile(NULL) {}

		void AddLine(const char* fmt, ...)
#ifdef __GNUC__
				__attribute__((format(printf, 2, 3)))
#endif
				;
		void CloseSession();

		void SetFilename(const char* fn) { filename = fn; }
		void FlushBuffer();

	private:

		// no copying
		CLogger(const CLogger&);
		CLogger& operator=(const CLogger&);

		static void CppFilt(char* sym, int size);

		spring::recursive_mutex logmutex;
		const char* filename;
		FILE* logfile;
		std::vector<std::string> buffer;
		std::string exename;
};

#endif // SYNCDEBUG

#endif // LOGGER_H
