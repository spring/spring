/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DefaultFilter.h"

#include "Level.h"
#include "Section.h"
#include "ILog.h"

#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <map>
#include <set>
#include <stack>


#ifdef __cplusplus
extern "C" {
#endif


bool log_frontend_isEnabled(const char* section, int level);


extern void log_backend_record(const char* section, int level,
		const char* fmt, va_list arguments);

extern void log_backend_cleanup();


struct log_filter_section_compare {
	inline bool operator()(const char* const& section1,
			const char* const& section2) const
	{
		return LOG_SECTION_COMPARE(section1, section2);
	}
};

namespace {
	typedef std::set<const char*, log_filter_section_compare> secSet_t;
	typedef std::map<const char*, int, log_filter_section_compare> secIntMap_t;

	int minLevel = LOG_LEVEL_ALL;

	secIntMap_t& log_filter_getSectionMinLevels() {
		static secIntMap_t sectionMinLevels;
		return sectionMinLevels;
	}

	secSet_t& log_filter_getRegisteredSections() {
		static secSet_t sections;
		return sections;
	}
}


static inline int log_filter_section_getDefaultMinLevel(const char* section) {

	if (LOG_SECTION_IS_DEFAULT(section)) {
#ifdef DEBUG
		return LOG_LEVEL_DEBUG;
	} else {
		return LOG_LEVEL_INFO;
#else
		return LOG_LEVEL_INFO;
	} else {
		return LOG_LEVEL_WARNING;
#endif
	}
}

static inline void log_filter_checkCompileTimeMinLevel(int level) {

	if (level < _LOG_LEVEL_MIN) {
		// to prevent an endless recursion
		if (_LOG_LEVEL_MIN <= LOG_LEVEL_WARNING) {
			LOG_L(L_WARNING,
					"Tried to set minimum log level %i, but it was set to %i"
					" at compile-time -> effective min-level is %i.",
					level, _LOG_LEVEL_MIN, _LOG_LEVEL_MIN);
		}
	}
}


int log_filter_global_getMinLevel() {
	return minLevel;
}

void log_filter_global_setMinLevel(int level) {

	log_filter_checkCompileTimeMinLevel(level);

	minLevel = level;
}


int log_filter_section_getMinLevel(const char* section) {

	int level = -1;

	const secIntMap_t& sectionMinLevels = log_filter_getSectionMinLevels();
	const secIntMap_t::const_iterator sectionMinLevel
			= sectionMinLevels.find(section);
	if (sectionMinLevel == sectionMinLevels.end()) {
		level = log_filter_section_getDefaultMinLevel(section);
	} else {
		level = sectionMinLevel->second;
	}

	return level;
}

void log_filter_section_setMinLevel(const char* section, int level) {

	log_filter_checkCompileTimeMinLevel(level);

	secIntMap_t& sectionMinLevels = log_filter_getSectionMinLevels();
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

int log_filter_section_getRegistered() {
	return log_filter_getRegisteredSections().size();
}

const char* log_filter_section_getRegisteredIndex(int index) {

	const char* section = NULL;

	const secSet_t& registeredSections = log_filter_getRegisteredSections();
	if ((index >= 0) && (index < (int)registeredSections.size())) {
		secSet_t::const_iterator si = registeredSections.begin();
		for (int curIndex = 0; curIndex < index; ++curIndex) {
			si = ++si;
		}
		section = *si;
	}

	return section;
}

static void log_filter_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	assert(level > LOG_LEVEL_ALL);
	assert(level < LOG_LEVEL_NONE);

	if (!log_frontend_isEnabled(section, level)) {
		return;
	}

	// format (and later store) the log record
	log_backend_record(section, level, fmt, arguments);
}


/**
 * @name logging_frontend_defaultFilter
 * ILog.h frontend implementation.
 */
///@{

bool log_frontend_isEnabled(const char* section, int level) {

	return ((level >= log_filter_global_getMinLevel())
			&& (level >= log_filter_section_getMinLevel(section)));
}

void log_frontend_registerSection(const char* section) {

	if (!LOG_SECTION_IS_DEFAULT(section)) {
		secSet_t& registeredSections = log_filter_getRegisteredSections();
		secSet_t::const_iterator si = registeredSections.find(section);
		if (si == registeredSections.end()) {
			registeredSections.insert(section);
		}
	}
}

void log_frontend_record(const char* section, int level, const char* fmt,
		...)
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

std::set<const char*> log_filter_section_getRegisteredSet() {

	std::set<const char*> outSet;

	const secSet_t& registeredSections = log_filter_getRegisteredSections();
	secSet_t::const_iterator si;
	for (si = registeredSections.begin(); si != registeredSections.end();
			++si)
	{
		outSet.insert(*si);
	}

	return outSet;
}

