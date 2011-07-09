/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <map>

#include "DefaultFilter.h"

#include "Level.h"
#include "Section.h"
#include "ILog.h"


#ifdef __cplusplus
extern "C" {
#endif

bool log_frontend_isEnabled(const char* section, int level);

extern void log_sink_record(const char* section, int level, const char* fmt,
		va_list arguments);


static int minLevel = LOG_LEVEL_ALL;
static std::map<const char*, int> sectionMinLevels;


static inline int log_filter_section_getDefaultMinLevel(const char* section) {

#ifdef DEBUG
	return LOG_LEVEL_DEBUG;
#else
	if (section == LOG_SECTION_DEFAULT) {
		return LOG_LEVEL_INFO;
	} else {
		return LOG_LEVEL_WARNING;
	}
#endif
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

	const std::map<const char*, int>::const_iterator sectionMinLevel
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

	if (level == log_filter_section_getDefaultMinLevel(section)) {
		sectionMinLevels.erase(section);
	} else {
		sectionMinLevels[section] = level;
	}
}


void log_filter_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	if (!log_frontend_isEnabled(section, level)) {
		return;
	}

	// store the log record
	log_sink_record(section, level, fmt, arguments);
}


/**
 * ILog.h frontend implementation.
 * @group logging_frontend_defaultFilter
 * @{
 */

bool log_frontend_isEnabled(const char* section, int level) {

	return ((level >= log_filter_global_getMinLevel())
			&& (level >= log_filter_section_getMinLevel(section)));
}

void log_frontend_record(const char* section, int level, const char* fmt,
		...)
{
	// pass the log record on to the filter
	va_list arguments;
	va_start(arguments, fmt);
	log_filter_record(section, level, fmt, arguments);
	va_end(arguments);
}

/** @} */ // group logging_frontend_defaultFilter

#ifdef __cplusplus
} // extern "C"
#endif

