/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _I_LOG_H
#define _I_LOG_H

/*
 * Logging API
 * -----------
 * Leight-weight & flexible logging API.
 *
 * Aims:
 * - Support a fixed set of severities levels:
 *   * L_DEBUG   : fine-grained information that is most useful to debug
 *   * L_INFO    : informational messages that highlight runtime progress of
 *   * L_WARNING : potentially harmful situations
 *   * L_ERROR   : errors that might still allow the application to keep running
 *   * L_FATAL   : very severe errors that will lead the application to abort
 *   (the "L_" prefix is required for disambiguation from other symbols)
 * - Support arbitrary sections (strings).
 * - Allow to set the minimum severity level
 *   * ... at project configure stage (no overhead at runtime)
 *   * ... at runtime (keep overhead low)
 * - Only allow printf-style message formatting (no streams), because:
 *   * it is C compatible
 *   * it is type safe
 *   * it is compatible with internationalization (i18n)
 *   * it is simple and a commonly known standard
 * - The API used by clients consists of pre-processor macros only.
 * - Can be used from C.
 * - Keeps the API between sub-systems minimal:
 *   frontend <-> filter <-> backend(sinks)
 *
 * There is a grpahical diagram, explaining the basic structure of this API
 * under doc/logApi.dia (Dia diagram editor file).
 */

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Connection to the non-pre-processor part of the frontend

/**
 * Allows the sink to manage a minimal log level too.
 * This will only ever be called for levels higher then the minimal level set
 * during compile-time (_LOG_LEVEL).
 */
extern bool log_frontend_isLevelEnabled(int level);

/**
 * Allows the sink to manage a minimal log section too.
 * This will only ever be called if the section is not already disabled
 * during compile-time.
 */
extern bool log_frontend_isSectionEnabled(const char* section);


// format string error checking
#ifdef __GNUC__
#define FORMAT_STRING(n) __attribute__((format(printf, n, n + 1)))
#else
#define FORMAT_STRING(n)
#endif

/**
 * Where all log messages get directed to after having passed the frontend.
 * The main connection to the backend/sink.
 * This will receive all log messages that are not disabled at compile-time
 * already, so it has to check internally, whether the criteria for logging
 * are really met, so it will have to call log_frontend_isLevelEnabled() and
 * log_frontend_isSectionEnabled() internally.
 */
extern void log_frontend_record(const char* section, int level, const char* fmt,
		...) FORMAT_STRING(3);

#undef FORMAT_STRING


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Level

/**
 * Only stuff from in this doc-group should ever be used by clients of this API.
 *
 * @group logging_api
 * @{
 */

#define LOG_LEVEL_ALL       0

#define LOG_LEVEL_DEBUG    20
#define LOG_LEVEL_INFO     30
#define LOG_LEVEL_WARNING  40
#define LOG_LEVEL_ERROR    50
#define LOG_LEVEL_FATAL    60

#define LOG_LEVEL_NONE    255

/** @} */ // group logging_api


#define _LOG_LEVEL_DEFAULT LOG_LEVEL_ALL

/**
 * Minimal log level (compile-time).
 * This is the compile-time version of this setting;
 * there might be a more restrictive one at runtime.
 * Initialize to the default (may be overriden with -DLOG_LEVEL=X).
 */
#ifndef _LOG_LEVEL
	#define _LOG_LEVEL _LOG_LEVEL_DEFAULT
#endif


#define _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	(LOG_LEVE##level >= _LOG_LEVEL)

#define _LOG_IS_ENABLED_LEVEL_RUNTIME(level) \
	log_frontend_isLevelEnabled(LOG_LEVE##level)


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Section

#define LOG_SECTION_DEFAULT NULL

/**
 * The current log section.
 * Initialize to the default.
 * You might set a custom section by defining LOG_SECTION before including
 * this header, or by redefining LOG_SECTION (#undef & #define).
 */
#ifndef LOG_SECTION
	#define LOG_SECTION LOG_SECTION_DEFAULT
#endif

// enable all log sections at compile-time
#define _LOG_IS_ENABLED_SECTION_STATIC(section) \
	true
#define _LOG_IS_ENABLED_SECTION_DEFINED_STATIC(section) \
	_LOG_IS_ENABLED_SECTION_STATIC(LOG_SECTION)

#define _LOG_IS_ENABLED_SECTION_RUNTIME(section) \
	log_frontend_isSectionEnabled(section)
#define _LOG_IS_ENABLED_SECTION_DEFINED_RUNTIME(section) \
	_LOG_IS_ENABLED_SECTION_RUNTIME(LOG_SECTION)


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Log pre-processing

/*
 * For a discussion about variadic macros, see:
 * http://stackoverflow.com/questions/679979/c-c-how-to-make-a-variadic-macro-variable-number-of-arguments
 * The "##" before __VA_ARGS__ allows the var-args list to be empty.
 */

/*
 * This is where we might add all sorts of additional info,
 * like the LOG_SECTION, __FILE__, __LINE__ or a stack-trace.
 * In theory, we could also use this to compleetly disable logging at
 * compile-time already.
 */

/// Redirect to runtime processing
#define _LOG_RECORD(section, level, fmt, ...) \
	log_frontend_record(section, LOG_LEVE##level, fmt, ##__VA_ARGS__)

// TODO get rid of this, once the backend supports section and level
//#define _LOG_SECTION_EXTENDED(section, level, fmt, ...)
//	_LOG_RECORD(section, level, "Level:%i Section:\"%s\" " fmt, level, (section ? section : ""), ##__VA_ARGS__)

#define _LOG_FILTERED(section, level, fmt, ...) \
	_LOG_RECORD(section, level, fmt, ##__VA_ARGS__)
//	_LOG_SECTION_EXTENDED(section, level, fmt, ##__VA_ARGS__)

// per level compile-time filters
#if _LOG_IS_ENABLED_LEVEL_STATIC(L_DEBUG)
	#define _LOG_FILTER_L_DEBUG(section, fmt, ...) \
		_LOG_FILTERED(section, L_DEBUG, fmt, ##__VA_ARGS__)
#else
	#define _LOG_FILTER_L_DEBUG(section, fmt, ...)
#endif
#if _LOG_IS_ENABLED_LEVEL_STATIC(L_INFO)
	#define _LOG_FILTER_L_INFO(section, fmt, ...) \
		_LOG_FILTERED(section, L_INFO, fmt, ##__VA_ARGS__)
#else
	#define _LOG_FILTER_L_INFO(section, fmt,...)
	#warning log messages of level INFO are not compiled into the binary
#endif
#if _LOG_IS_ENABLED_LEVEL_STATIC(L_WARNING)
	#define _LOG_FILTER_L_WARNING(section, fmt, ...) \
		_LOG_FILTERED(section, L_WARNING, fmt, ##__VA_ARGS__)
#else
	#define _LOG_FILTER_L_WARNING(section, fmt, ...)
	#warning log messages of level WARNING are not compiled into the binary
#endif
#if _LOG_IS_ENABLED_LEVEL_STATIC(L_ERROR)
	#define _LOG_FILTER_L_ERROR(section, fmt, ...) \
		_LOG_FILTERED(section, L_ERROR, fmt, ##__VA_ARGS__)
#else
	#define _LOG_FILTER_L_ERROR(section, fmt, ##__VA_ARGS__)
	#warning log messages of level ERROR are not compiled into the binary
#endif
#if _LOG_IS_ENABLED_LEVEL_STATIC(L_FATAL)
	#define _LOG_FILTER_L_FATAL(section, fmt, ...) \
		_LOG_FILTERED(section, L_FATAL, fmt, ##__VA_ARGS__)
#else
	#define _LOG_FILTER_L_FATAL(section, fmt, ...)
	#warning log messages of level FATAL are not compiled into the binary
#endif

/// Connects to the sink macro
#define _LOG_SECTION(section, level, fmt, ...) \
	_LOG_FILTER_##level(section, fmt, ##__VA_ARGS__)

/// Uses the section defined in LOG_SECTION
#define _LOG_SECTION_DEFINED(level, fmt, ...) \
	_LOG_SECTION(LOG_SECTION, level, fmt, ##__VA_ARGS__)

/// Entry point for frontend-internal processing
#define _LOG(level, fmt, ...) \
	_LOG_SECTION_DEFINED(level, fmt, ##__VA_ARGS__)


#define _LOG_IS_ENABLED_STATIC(level) \
	(  _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	&& _LOG_IS_ENABLED_SECTION_DEFINED_STATIC())

#define _LOG_IS_ENABLED(level) \
	(  _LOG_IS_ENABLED_STATIC(level) \
	&& _LOG_IS_ENABLED_LEVEL_RUNTIME(level) \
	&& _LOG_IS_ENABLED_SECTION_DEFINED_RUNTIME())


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// API

/**
 * Only stuff from in this doc-group should ever be used by clients of this API.
 *
 * @addtogroup logging_api
 * @{
 */

/**
 * Returns whether logging for the current section and the supplied level is
 * enabled at compile-time.
 * This might return true, even if the back-end will ignore the message anyway.
 * You should consider using the LOG_IS_ENABLED macro instead (recommended).
 * This may be used with the preprocessor directly (in an #if statement):
 * <code>
 * #if LOG_IS_ENABLED_STATIC(L_DEBUG)
 *   const int statistics = calculateStats(); // performance-heavy
 *   LOG(L_DEBUG, "the stats: %i", statistics);
 * #endif
 * </code>
 * @see LOG_IS_ENABLED
 */
#define LOG_IS_ENABLED_STATIC(level) \
	_LOG_IS_ENABLED_STATIC(level)

/**
 * Returns whether logging for the current section and the supplied level is
 * enabled at run-time.
 * Usage example:
 * <code>
 * if (LOG_IS_ENABLED(L_DEBUG)) {
 *   const int statistics = calculateStats(); // performance-heavy
 *   LOG(L_DEBUG, "the stats: %i", statistics);
 * }
 * </code>
 * @see LOG_IS_ENABLED_STATIC
 */
#define LOG_IS_ENABLED(level) \
	_LOG_IS_ENABLED(level)

/**
 * Registers a log message with level INFO.
 * For message formatting help, see the printf documentation.
 * Usage example:
 * <code>
 * LOG("my age is %i", 21);
 * LOG("nothing to see here, move on.");
 * </code>
 * @see printf
 * @see LOG_L()
 * @see LOG_IS_ENABLED()
 */
#define LOG(fmt, ...) \
	_LOG(L_INFO, fmt, ##__VA_ARGS__)

/**
 * Registers a log message with a specifiable level.
 * Usage example:
 * <code>
 * LOG_L(L_DEBUG, "my age is %i", 21);
 * </code>
 * @see LOG()
 */
#define LOG_L(level, fmt, ...) \
	_LOG(level, fmt, ##__VA_ARGS__)

/**
 * Registers a log message with a specifiable section.
 * Usage example:
 * <code>
 * LOG_S("one-time-section", "my age is %i", 21);
 * </code>
 * @see LOG()
 * @see LOG_SECTION
 */
#define LOG_S(section, fmt, ...) \
	_LOG_SECTION(section, L_INFO, fmt, ##__VA_ARGS__)

/**
 * Registers a log message with a specifiable section and level.
 * Usage example:
 * <code>
 * LOG_SL("one-time-section", L_DEBUG, "my age is %i", 21);
 * </code>
 * @see LOG_L()
 * @see LOG_S()
 * @see LOG_SECTION
 */
#define LOG_SL(section, level, fmt, ...) \
	_LOG_SECTION(section, level, fmt, ##__VA_ARGS__)

/** @} */ // group logging_api

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _I_LOG_H
