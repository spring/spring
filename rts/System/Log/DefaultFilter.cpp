/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <memory>
#include <stack>
#include <string>

#include <map>
#include <set>

#include "DefaultFilter.h"
#include "Level.h"
#include "Section.h"
#include "ILog.h"
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


namespace {
	static int minLogLevel = LOG_LEVEL_ALL;
	static int repeatLimit = 1;

	std::map<const char*, int, log_filter_section_compare>& log_filter_getSectionMinLevels() {
		static std::map<const char*, int, log_filter_section_compare> sectionMinLevels;
		return sectionMinLevels;
	}

	std::set<const char*, log_filter_section_compare>& log_filter_getRegisteredSections() {
		static std::set<const char*, log_filter_section_compare> sections;
		return sections;
	}
/*
	void inline log_filter_printSectionMinLevels(const char* func) {
		printf("[%s][caller=%s]\n", __func__, func);

		for (const auto& p: log_filter_getSectionMinLevels()) {
			printf("\tsectionName=\"%s\" minLevel=%d\n", p.first, p.second);
		}
	}
*/
}

#ifdef __cplusplus
extern "C" {
#endif


static inline int log_filter_section_getDefaultMinLevel(const char* section) {

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

static inline void log_filter_checkCompileTimeMinLevel(int level) {

	if (level >= _LOG_LEVEL_MIN)
		return;

	// to prevent an endless recursion
	if (_LOG_LEVEL_MIN > LOG_LEVEL_WARNING)
		return;

	LOG_L(L_WARNING,
		"Tried to set minimum log level %i, but it was set to"
		" %i at compile-time -> effective min-level is %i.",
		level, _LOG_LEVEL_MIN, _LOG_LEVEL_MIN
	);
}



int log_filter_global_getMinLevel() { return minLogLevel; }
void log_filter_global_setMinLevel(int level) { log_filter_checkCompileTimeMinLevel(level); minLogLevel = level; }

int log_filter_getRepeatLimit() { return repeatLimit; }
void log_filter_setRepeatLimit(int limit) { repeatLimit = limit; }



int log_filter_section_getMinLevel(const char* section)
{
	int level = -1;

	const auto& sectionMinLevels = log_filter_getSectionMinLevels();
	const auto sectionMinLevel = sectionMinLevels.find(section);

	if (sectionMinLevel == sectionMinLevels.end()) {
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

	auto& registeredSections = log_filter_getRegisteredSections();
	auto& sectionMinLevels = log_filter_getSectionMinLevels();

	// NOTE:
	//   <section> might not be in the registered set if called from Lua
	//
	//   if we inserted the raw pointer passed from Lua (lua_to*string)
	//   which lives on the heap into the min-level map it could become
	//   dangling because of Lua garbage collection but this is not even
	//   allowed by Section.h (!)
	const auto it = registeredSections.find(section);

	if (it == registeredSections.end()) {
		LOG_L(L_WARNING, "[%s] section \"%s\" is not registered", __func__, section);
		return;
	}

	// take the pointer from the registered set
	// (same string but will not become garbage)
	section = *it;

	if (level == log_filter_section_getDefaultMinLevel(section)) {
		sectionMinLevels.erase(section);
	} else {
		sectionMinLevels[section] = level;
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
	return (log_filter_getRegisteredSections().size());
}

const char* log_filter_section_getRegisteredIndex(int index) {
	const auto& registeredSections = log_filter_getRegisteredSections();

	if (index < 0)
		return NULL;
	if (index >= static_cast<int>(registeredSections.size()))
		return NULL;

	auto si = registeredSections.begin();

	std::advance(si, index);
	return (*si);
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

	auto& registeredSections = log_filter_getRegisteredSections();
	auto si = registeredSections.find(section);

	if (si != registeredSections.end())
		return;

	registeredSections.insert(section);
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

	for (const auto& key: log_filter_getRegisteredSections()) {
		outSet.insert(key);
	}

	return outSet;
}

const char* log_filter_section_getSectionCString(const char* section_cstr_tmp)
{
	static spring::unordered_map<std::string, std::unique_ptr<const char[]>> cache;

	const auto str = std::string(section_cstr_tmp);
	const auto it = cache.find(str);

	if (it != cache.end())
		return (it->second.get());

	char* section_cstr = new char[str.size() + 1];

	strcpy(&section_cstr[0], section_cstr_tmp);
	section_cstr[str.size()] = '\0';

	cache[str].reset(const_cast<const char*>(section_cstr));
	return section_cstr;
}

