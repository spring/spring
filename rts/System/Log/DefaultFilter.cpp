/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdio>
#include <cstdarg>
#include <set>

#include "DefaultFilter.h"


#ifdef __cplusplus
extern "C" {
#endif

bool log_frontend_isLevelEnabled(int level);
bool log_frontend_isSectionEnabled(const char* section);

extern void log_sink_record(const char* section, int level, const char* fmt,
		va_list arguments);


static int minLevel = 0; // LOG_LEVEL_ALL
static std::set<const char*> disabledSections;


void log_filter_setMinLevel(int level) {
	minLevel = level;
}

int log_filter_getMinLevel() {
	return minLevel;
}


void log_filter_setSectionEnabled(const char* section, bool enabled) {
	
	if (enabled) {
		disabledSections.erase(section);
	} else {
		disabledSections.insert(section);
	}
}

bool log_filter_isSectionEnabled(const char* section) {
	return (disabledSections.find(section) == disabledSections.end());
}


void log_filter_record(const char* section, int level, const char* fmt,
		va_list arguments)
{
	if (!log_frontend_isLevelEnabled(level)
			|| !log_frontend_isSectionEnabled(section))
	{
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

inline bool log_frontend_isLevelEnabled(int level) {
	return (level >= log_filter_getMinLevel());
}

inline bool log_frontend_isSectionEnabled(const char* section) {
	return log_filter_isSectionEnabled(section);
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

