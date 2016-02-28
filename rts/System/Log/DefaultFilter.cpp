/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DefaultFilter.h"

#include "Level.h"
#include "Section.h"
#include "ILog.h"

#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <stack>
#include <string>


#ifdef __cplusplus
extern "C" {
#endif


bool log_frontend_isEnabled(const char* section, int level);


extern void log_backend_record(const char* section, int level, const char* fmt, va_list arguments);
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
	typedef std::set<const char*, log_filter_section_compare> secSet_t;
	typedef std::map<const char*, int, log_filter_section_compare> secIntMap_t;

	int minLogLevel = LOG_LEVEL_ALL;

	secIntMap_t& log_filter_getSectionMinLevels() {
		static secIntMap_t sectionMinLevels;
		return sectionMinLevels;
	}

	secSet_t& log_filter_getRegisteredSections() {
		static secSet_t sections;
		return sections;
	}

	void inline log_filter_printSectionMinLevels(const char* func) {
		printf("[%s][caller=%s]\n", __FUNCTION__, func);

		const auto& secLevels = log_filter_getSectionMinLevels();

		for (auto it = secLevels.begin(); it != secLevels.end(); ++it) {
			printf("\tsectionName=\"%s\" minLevel=%d\n", it->first, it->second);
		}
	}
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


int log_filter_section_getMinLevel(const char* section)
{
	int level = -1;

	const secIntMap_t& sectionMinLevels = log_filter_getSectionMinLevels();
	const secIntMap_t::const_iterator sectionMinLevel = sectionMinLevels.find(section);

	if (sectionMinLevel == sectionMinLevels.end()) {
		level = log_filter_section_getDefaultMinLevel(section);
	} else {
		level = sectionMinLevel->second;
	}

	return level;
}

// called by LogOutput for each ENABLED section
// also by LuaUnsyncedCtrl::SetLogSectionFilterLevel
void log_filter_section_setMinLevel(const char* section, int level)
{
	log_filter_checkCompileTimeMinLevel(level);

	secSet_t& registeredSections = log_filter_getRegisteredSections();
	secIntMap_t& sectionMinLevels = log_filter_getSectionMinLevels();

	// NOTE:
	//   <section> might not be in the registered set if called from Lua
	//
	//   if we inserted the raw pointer passed from Lua (lua_to*string)
	//   which lives on the heap into the min-level map it could become
	//   dangling because of Lua garbage collection but this is not even
	//   allowed by Section.h (!)
	const auto it = registeredSections.find(section);

	if (it == registeredSections.end()) {
		LOG_L(L_WARNING, "[%s] section \"%s\" is not registered", __FUNCTION__, section);
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
	const secSet_t& registeredSections = log_filter_getRegisteredSections();

	if (index < 0)
		return NULL;
	if (index >= static_cast<int>(registeredSections.size()))
		return NULL;

	secSet_t::const_iterator si = registeredSections.begin();

	for (int curIndex = 0; curIndex < index; ++curIndex) {
		++si;
	}

	return (*si);
}

static void log_filter_record(const char* section, int level, const char* fmt, va_list arguments)
{
	assert(level > LOG_LEVEL_ALL);
	assert(level < LOG_LEVEL_NONE);

	if (!log_frontend_isEnabled(section, level))
		return;

	// format (and later store) the log record
	log_backend_record(section, level, fmt, arguments);
}


/**
 * @name logging_frontend_defaultFilter
 * ILog.h frontend implementation.
 */
///@{

bool log_frontend_isEnabled(const char* section, int level) {
	if (level < log_filter_global_getMinLevel())
		return false;
	if (level < log_filter_section_getMinLevel(section))
		return false;

	return true;
}

// see the LOG_REGISTER_SECTION_RAW macro in ILog.h
void log_frontend_register_section(const char* section) {
	if (!LOG_SECTION_IS_DEFAULT(section)) {
		secSet_t& registeredSections = log_filter_getRegisteredSections();
		secSet_t::const_iterator si = registeredSections.find(section);

		if (si == registeredSections.end()) {
			registeredSections.insert(section);
		}
	}
}

void log_frontend_register_runtime_section(const char* section_cstr_tmp, int level) {
	const char* section_cstr = log_filter_section_getSectionCString(section_cstr_tmp);

	log_frontend_register_section(section_cstr);
	log_filter_section_setMinLevel(section_cstr, level);

}

void log_frontend_record(const char* section, int level, const char* fmt, ...)
{
	assert(level > LOG_LEVEL_ALL);
	assert(level < LOG_LEVEL_NONE);

	// pass the log record on to the filter
	va_list arguments;
	va_start(arguments, fmt);
	log_filter_record(section, level, fmt, arguments);
	va_end(arguments);
}

void log_frontend_cleanup() {
	log_backend_cleanup();
}

///@}

#ifdef __cplusplus
} // extern "C"
#endif



std::set<const char*> log_filter_section_getRegisteredSet()
{
	const auto& registeredSections = log_filter_getRegisteredSections();
	std::set<const char*> outSet(registeredSections.begin(), registeredSections.end());
	return outSet;
}

const char* log_filter_section_getSectionCString(const char* section_cstr_tmp)
{
	static std::unordered_map<std::string, std::unique_ptr<const char[]>> cache;

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
