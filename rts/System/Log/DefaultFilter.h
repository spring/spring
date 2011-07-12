/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _LOG_DEFAULT_FILTER_H
#define _LOG_DEFAULT_FILTER_H

/**
 * A simple filter implementation for the ILog.h logging API.
 * It routes passing logging messages to the backend, and allows to set and get,
 * what is logged and what not.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @group logging_filter_defaultFilter_control
 * @{
 */

/**
 * Sets the minimum level to log.
 * This is only the runtime limit; there might already have been one defined at
 * compile-time.
 * When you choose a lower level at runtime then was chosen at compile-time,
 * the low level messages will NOT be logged.
 * @see #log_filter_getMinLevel
 */
void log_filter_setMinLevel(int level);
/**
 * Returns the minimum level to log.
 * @see #log_filter_getMinLevel
 */
int  log_filter_getMinLevel();

/**
 * Sets whether log messages for a certain section are logged or not.
 * All sections are enabled by default.
 * CAUTION: you may only use strings defined at compile-time.
 * @see #log_filter_isSectionEnabled
 */
void log_filter_setSectionEnabled(const char* section, bool enabled);
/**
 * Returns whether log messages for a certain section are logged or not.
 * All sections are enabled by default.
 * CAUTION: you may only use strings defined at compile-time.
 * @see #log_filter_setSectionEnabled
 */
bool log_filter_isSectionEnabled(const char* section);

/** @} */ // group logging_filter_defaultFilter_control

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _LOG_DEFAULT_FILTER_H

