/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <string>

#include <algorithm>
#include <array>
#include <stack>
#include <vector>

#include "DefaultFilter.h"
#include "Level.h"
#include "Section.h"
#include "ILog.h"
#include "System/StringHash.h"
#include "System/UnorderedMap.hpp"



#ifdef __cplusplus
extern "C" {
#endif


bool log_frontend_isEnabled(int level, const char* section);


extern void log_backend_record(int level, const char* section, const char* fmt, va_list arguments);
extern void log_backend_cleanup();


struct log_filter_section_compare {
	inline bool operator()(const char* const& section1, const char* const& section2) const
	{
		return LOG_SECTION_COMPARE(section1, section2);
	}
};
#ifdef __cplusplus
}
#endif


namespace log_filter {
	static int minLogLevel = LOG_LEVEL_ALL;
	static int repeatLimit = 1;

	#if 0
	void inline printSectionMinLevels(const char* func) {
		printf("[%s][caller=%s]\n", __func__, func);

		for (const auto& p: sectionMinLevels) {
			printf("\tsectionName=\"%s\" minLevel=%d\n", p.first, p.second);
		}
	}
	#endif

	// both sorted according to log_filter_section_compare
	static std::vector< std::pair<const char*, int> > sectionMinLevels;
	static std::vector<           const char*       > registeredSections;
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
	const auto sectionMinLevel = std::lower_bound(sectionMinLevels.begin(), sectionMinLevels.end(), P{section, 0}, sectionComparer);

	if (sectionMinLevel == sectionMinLevels.end() || strcmp(sectionMinLevel->first, section) != 0) {
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

	auto& registeredSections = log_filter::registeredSections;
	auto& sectionMinLevels = log_filter::sectionMinLevels;

	// NOTE:
	//   <section> might not be in the registered set if called from Lua
	//
	//   if we inserted the raw pointer passed from Lua (lua_to*string)
	//   which lives on the heap into the min-level map it could become
	//   dangling because of Lua garbage collection but this is not even
	//   allowed by Section.h (!)
	const auto registeredSection = std::lower_bound(registeredSections.begin(), registeredSections.end(), section, log_filter_section_compare());

	if (registeredSection == registeredSections.end() || strcmp(*registeredSection, section) != 0) {
		LOG_L(L_WARNING, "[%s] section \"%s\" is not registered", __func__, section);
		return;
	}

	// take the pointer from the registered set
	// (same string but will not become garbage)
	section = *registeredSection;

	if (level == log_filter_section_getDefaultMinLevel(section)) {
		using P = decltype(log_filter::sectionMinLevels)::value_type;

		const auto sectionComparer = [](const P& a, const P& b) { return (log_filter_section_compare()(a.first, b.first)); };
		const auto sectionMinLevel = std::lower_bound(sectionMinLevels.begin(), sectionMinLevels.end(), P{section, 0}, sectionComparer);

		if (sectionMinLevel == sectionMinLevels.end() || strcmp(sectionMinLevel->first, section) != 0)
			return;

		// erase
		for (size_t i = sectionMinLevel - sectionMinLevels.begin(), j = sectionMinLevels.size() - 1; i < j; i++) {
			sectionMinLevels[i].first  = std::move(sectionMinLevels[i + 1].first );
			sectionMinLevels[i].second = std::move(sectionMinLevels[i + 1].second);
		}

		sectionMinLevels.pop_back();
		return;
	}

	// no better place for this (see also log_frontend_register_section)
	if (sectionMinLevels.empty())
		sectionMinLevels.reserve(8);

	sectionMinLevels.emplace_back(section, level);

	// swap into position
	for (size_t i = sectionMinLevels.size() - 1; i > 0; i--) {
		if (log_filter_section_compare()(sectionMinLevels[i - 1].first, sectionMinLevels[i].first))
			break;

		std::swap(sectionMinLevels[i - 1], sectionMinLevels[i]);
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
	return (log_filter::registeredSections.size());
}

const char* log_filter_section_getRegisteredIndex(int index) {
	const auto& registeredSections = log_filter::registeredSections;

	if (index < 0)
		return nullptr;
	if (index >= static_cast<int>(registeredSections.size()))
		return nullptr;

	return registeredSections[index];
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

	auto& registeredSections = log_filter::registeredSections;
	auto registeredSection = std::lower_bound(registeredSections.begin(), registeredSections.end(), section, log_filter_section_compare());

	if (registeredSection != registeredSections.end() && strcmp(*registeredSection, section) == 0)
		return;

	// no better place for this (see also log_filter_section_setMinLevel)
	if (registeredSections.empty())
		registeredSections.reserve(8);

	registeredSections.emplace_back(section);

	// swap into position
	for (size_t i = registeredSections.size() - 1; i > 0; i--) {
		if (log_filter_section_compare()(registeredSections[i - 1], registeredSections[i]))
			break;

		std::swap(registeredSections[i - 1], registeredSections[i]);
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



spring::unsynced_set<const char*> log_filter_section_getRegisteredSet()
{
	spring::unsynced_set<const char*> outSet;
	outSet.reserve(log_filter::registeredSections.size());

	for (const auto& key: log_filter::registeredSections) {
		outSet.insert(key);
	}

	return outSet;
}

const char* log_filter_section_getSectionCString(const char* section_cstr_tmp)
{
	// cache for log_frontend_register_runtime_section; services LuaUnsyced
	static std::array<char[1024], 64> cache;
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

