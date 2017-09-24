/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimpleProfiler.h"

#include <sstream>
#include <string>
#include <map>
#include <string.h>

#include "TimeUtil.h"
#include "Util.h"
#include "System/MainDefines.h"
#include "System/SafeCStrings.h"


Profiler Profiler::def("");

Profiler* Profiler::GetDefault() {
	return &Profiler::def;
}

Profiler::Profiler(const char* const name)
	: name(name)
{
}

const char* Profiler::GetName() const {
	return name;
}

void Profiler::AddTime(const char* const part, unsigned long time) {
	parts[part] += time;
}

const std::map<const char* const, unsigned long>& Profiler::GetParts() {
	return parts;
}

std::string Profiler::ToString() const {

    std::ostringstream res;

	static const size_t line_sizeMax = 256;
	char line[line_sizeMax];

	SNPRINTF(line, line_sizeMax, "%35s |%20s\n", "Part", "Total Time");
	res << line;
	std::map<const char* const, unsigned long>::const_iterator pi;
	for (pi = parts.begin(); pi != parts.end(); ++pi) {
		SNPRINTF(line, line_sizeMax, "%35s  %16.3fs\n", pi->first, pi->second / 1000.f);
		res << line;
	}

	return res.str();
}



ScopedTimer::ScopedTimer(const char* const part, Profiler* profiler)
	: part(part)
	, profiler(profiler ? profiler : Profiler::GetDefault())
	, startTime(timeUtil_getCurrentTimeMillis())
{
}

ScopedTimer::~ScopedTimer()
{
	const unsigned long stopTime = timeUtil_getCurrentTimeMillis();
	profiler->AddTime(part, stopTime - startTime);
}




void          simpleProfiler_addTime(const char* const part, unsigned time) {
	Profiler::GetDefault()->AddTime(part, time);
}

unsigned long simpleProfiler_getTime(const char* const part) {
	return Profiler::GetDefault()->GetParts().find(part)->second;
}

unsigned      simpleProfiler_getParts() {
	return Profiler::GetDefault()->GetParts().size();
}

unsigned      simpleProfiler_getPartNames(const char** parts, const unsigned parts_sizeMax) {

	unsigned p = 0;

	std::map<const char* const, unsigned long>::const_iterator pi;
	for (pi = Profiler::GetDefault()->GetParts().begin(); (pi != Profiler::GetDefault()->GetParts().end()) && (p < parts_sizeMax); ++pi) {
		parts[p++] = pi->first;
	}

	return p;
}

char*         simpleProfiler_getSummaryString() {

	const std::string& summaryStr = Profiler::GetDefault()->ToString();
	const int length = summaryStr.length();
	char* summary = util_allocStr(length);

	STRCPY_T(summary, length, summaryStr.c_str());

	return summary;
}
