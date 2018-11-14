/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <string>

#include <algorithm>
#include <stack>

#include "DefaultFilter.h"
#include "Level.h"
#include "Section.h"
#include "ILog.h"
#include "System/StringHash.h"
#include "System/UnorderedMap.hpp"

#define MAX_LOG_SECTIONS 64


#ifdef __cplusplus
extern "C" {
#endif

extern void log_backend_record(int level, const char* section, const char* fmt, va_list arguments);
extern void log_backend_cleanup();

struct log_filter_section_compare {
	inline bool operator()(const char* const& section1, const char* const& section2) const
	{
		return (LOG_SECTION_COMPARE_LESS(section1, section2));
	}
};
#ifdef __cplusplus
}
#endif


namespace log_filter {
	static int minLogLevel = LOG_LEVEL_ALL;
	static int repeatLimit = 1;

	static size_t numLevels = 0;
	static size_t numSections = 0;

	// both sorted according to log_filter_section_compare
	static std::array< std::pair<const char*, int> , MAX_LOG_SECTIONS> sectionMinLevels;
	static std::array<           const char*       , MAX_LOG_SECTIONS> registeredSections;

	#if 0
	void inline printSectionMinLevels(const char* func) {
		printf("[%s][caller=%s]\n", __func__, func);

		for (const auto& p: sectionMinLevels) {
			printf("\tsectionName=\"%s\" minLevel=%d\n", p.first, p.second);
		}
	}
	#endif
}


#ifdef __cplusplus
extern "C" {
#endif


static inline int log_filter_section_getDefaultMinLevel(const char* section)
{
	if (LOG_SECTION_IS_DEFAULT(section)) {
#ifdef DEBUG
		return LOG_LEVEL_DEBUG;
	} else {
		return LOG_LEVEL_INFO;
#else
		return LOG_LEVEL_INFO;
	} else {
		return LOG_LEVEL_NOTICE;
#endif
	}
}

static inline void log_filter_checkCompileTimeMinLevel(int level)
{
	if (level >= _LOG_LEVEL_MIN)
		return;

	// to prevent an endless recursion
	if (_LOG_LEVEL_MIN > LOG_LEVEL_WARNING)
		return;

	LOG_L(L_WARNING,
		"[%s] tried to set minimum log level %i, but it was set to"
		" %i at compile-time -> effective min-level is %i.",
		__func__, level, _LOG_LEVEL_MIN, _LOG_LEVEL_MIN
	);
}



int log_filter_global_getMinLevel() { return log_filter::minLogLevel; }
void log_filter_global_setMinLevel(int level) { log_filter_checkCompileTimeMinLevel(log_filter::minLogLevel = level); }

int log_filter_getRepeatLimit() { return log_filter::repeatLimit; }
void log_filter_setRepeatLimit(int limit) { log_filter::repeatLimit = limit; }



int log_filter_section_getMinLevel(const char* section)
{
	int level = -1;

	using P = decltype(log_filter::sectionMinLevels)::value_type;

	const auto& sectionMinLevels = log_filter::sectionMinLevels;
	const auto sectionComparer = [](const P& a, const P& b) { return (log_filter_section_compare()(a.first, b.first)); };
	const auto sectionMinLevel = std::lower_bound(sectionMinLevels.begin(), sectionMinLevels.begin() + log_filter::numLevels, P{section, 0}, sectionComparer);

	if (sectionMinLevel == (sectionMinLevels.begin() + log_filter::numLevels) || strcmp(sectionMinLevel->first, section) != 0) {
		level = log_filter_section_getDefaultMinLevel(section);
	} else {
		level = sectionMinLevel->second;
	}

	return level;
}

// called by LogOutput for each ENABLED section
// also by LuaUnsyncedCtrl::SetLogSectionFilterLevel
void log_filter_section_setMinLevel(int level, const char* section)
{
	log_filter_checkCompileTimeMinLevel(level);

	auto& regSecs = log_filter::registeredSections;
	auto& secLvls = log_filter::sectionMinLevels;

	// NOTE:
	//   <section> might not be in the registered set if called from Lua
	//
	//   if we inserted the raw pointer passed from Lua (lua_to*string)
	//   which lives on the heap into the min-level map it could become
	//   dangling because of Lua garbage collection but this is not even
	//   allowed by Section.h (!)
	const auto registeredSection = std::lower_bound(regSecs.begin(), regSecs.begin() + log_filter::numSections, section, log_filter_section_compare());

	if (log_filter::numLevels >= secLvls.size()) {
		LOG_L(L_WARNING, "[%s] too many section-levels", __func__);
		return;
	}
	if (registeredSection == (regSecs.begin() + log_filter::numSections) || strcmp(*registeredSection, section) != 0) {
		LOG_L(L_WARNING, "[%s] section \"%s\" is not registered", __func__, section);
		return;
	}

	if (log_filter::numLevels == 0)
		secLvls.fill({"", 0});

	// take the pointer from the registered set
	// (same string but will not become garbage)
	section = *registeredSection;

	if (level == log_filter_section_getDefaultMinLevel(section)) {
		using P = decltype(log_filter::sectionMinLevels)::value_type;

		const auto sectionComparer = [](const P& a, const P& b) { return (log_filter_section_compare()(a.first, b.first)); };
		const auto sectionMinLevel = std::lower_bound(secLvls.begin(), secLvls.begin() + log_filter::numLevels, P{section, 0}, sectionComparer);

		if (sectionMinLevel == (secLvls.begin() + log_filter::numLevels) || strcmp(sectionMinLevel->first, section) != 0)
			return;

		// erase
		for (size_t i = sectionMinLevel - secLvls.begin(), j = --log_filter::numLevels; i < j; i++) {
			secLvls[i].first  = secLvls[i + 1].first;
			secLvls[i].second = secLvls[i + 1].second;
		}

		return;
	}

	secLvls[log_filter::numLevels++] = {section, level};

	// swap into position
	for (size_t i = log_filter::numLevels - 1; i > 0; i--) {
		if (log_filter_section_compare()(secLvls[i - 1].first, secLvls[i].first))
			break;

		std::swap(secLvls[i - 1], secLvls[i]);
	}
}


void log_enable_and_disable(const bool enable)
{
	static std::stack<int> oldLevels;
	int newLevel;

	if (enable) {
		assert(!oldLevels.empty());
		newLevel = oldLevels.top();
		oldLevels.pop();
	} else {
		oldLevels.push(log_filter_global_getMinLevel());
		newLevel = LOG_LEVEL_FATAL;
	}

	log_filter_global_setMinLevel(newLevel);
}

int log_filter_section_getNumRegisteredSections() {
	return log_filter::numSections;
}

const char* log_filter_section_getRegisteredIndex(int index) {
	if (index < 0)
		return nullptr;
	if (index >= static_cast<int>(log_filter::numSections))
		return nullptr;

	return log_filter::registeredSections[index];
}

static void log_filter_record(int level, const char* section, const char* fmt, va_list arguments)
{
	assert(level > LOG_LEVEL_ALL);
	assert(level < LOG_LEVEL_NONE);

	if (!log_frontend_isEnabled(level, section))
		return;

	// format (and later store) the log record
	log_backend_record(level, section, fmt, arguments);
}


/**
 * @name logging_frontend_defaultFilter
 * ILog.h frontend implementation.
 */
///@{

bool log_frontend_isEnabled(int level, const char* section) {
	if (level < log_filter_global_getMinLevel())
		return false;
	if (level < log_filter_section_getMinLevel(section))
		return false;

	return true;
}

// see the LOG_REGISTER_SECTION_RAW macro in ILog.h
void log_frontend_register_section(const char* section) {
	if (LOG_SECTION_IS_DEFAULT(section))
		return;

	auto& regSecs = log_filter::registeredSections;
	auto regSec = std::lower_bound(regSecs.begin(), regSecs.begin() + log_filter::numSections, section, log_filter_section_compare());

	// too many sections
	if (log_filter::numSections >= regSecs.size())
		return;
	// filter duplicates
	if (regSec != (regSecs.begin() + log_filter::numSections) && strcmp(*regSec, section) == 0)
		return;

	if (log_filter::numSections == 0)
		regSecs.fill("");

	regSecs[log_filter::numSections++] = section;

	// swap into position
	for (size_t i = log_filter::numSections - 1; i > 0; i--) {
		if (log_filter_section_compare()(regSecs[i - 1], regSecs[i]))
			break;

		std::swap(regSecs[i - 1], regSecs[i]);
	}
}

void log_frontend_register_runtime_section(int level, const char* section_cstr_tmp) {
	const char* section_cstr = log_filter_section_getSectionCString(section_cstr_tmp);

	log_frontend_register_section(section_cstr);
	log_filter_section_setMinLevel(level, section_cstr);

}

void log_frontend_record(int level, const char* section, const char* fmt, ...)
{
	assert(level > LOG_LEVEL_ALL);
	assert(level < LOG_LEVEL_NONE);

	// pass the log record on to the filter
	va_list arguments;
	va_start(arguments, fmt);
	log_filter_record(level, section, fmt, arguments);
	va_end(arguments);
}

void log_frontend_cleanup() {
	log_backend_cleanup();
}


///@}

#ifdef __cplusplus
} // extern "C"
#endif



const char** log_filter_section_getRegisteredSet()
{
	return &log_filter::registeredSections[0];
}

const char* log_filter_section_getSectionCString(const char* section_cstr_tmp)
{
	// cache for log_frontend_register_runtime_section; services LuaUnsyced
	static std::array<char[1024], MAX_LOG_SECTIONS> cache;
	static spring::unordered_map<std::string, size_t> index;

	// see if str is already mapped to a cache-index
	const auto str = std::string(section_cstr_tmp);
	const auto iter = index.find(str);

	static_assert(sizeof(cache[0]) == 1024, "");

	if (iter != index.end())
		return cache[iter->second];

	// too many sections
	if (index.size() == cache.size())
		return "";
	// too long section-name
	if (str.size() >= sizeof(cache[0]))
		return "";

	if (index.empty())
		index.reserve(cache.size());

	strncpy(&cache[index.size()][0], section_cstr_tmp, sizeof(cache[0]));
	index.emplace(str, index.size());

	return &cache[index.size() - 1][0];
}

