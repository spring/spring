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
#else
	#include "System/booldefines.h"
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Connection to the non-pre-processor part of the frontend

/**
 * Allows the global filter to manage a minimal log level too,
 * optionally per section.
 * This will only ever be called for levels higher then the minimal level set
 * during compile-time (_LOG_LEVEL_MIN), and if the section is not already
 * disabled during compile-time.
 */
extern bool log_frontend_isEnabled(const char* section, int level);

/**
 * Allows the global filter to maintain a set of all setions used in the binary.
 * This will be called once per each LOG*() line in the source.
 */
extern void log_frontend_registerSection(const char* section);


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
 * are really met, so it will have to call log_frontend_isEnabled() internally.
 */
extern void log_frontend_record(const char* section, int level, const char* fmt,
		...) FORMAT_STRING(3);

#undef FORMAT_STRING


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Level & Section

#include "Level.h"

#define _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	(LOG_LEVE##level >= _LOG_LEVEL_MIN)


#include "Section.h"

// enable all log sections at compile-time
#define _LOG_IS_ENABLED_SECTION_STATIC(section) \
	true
#define _LOG_IS_ENABLED_SECTION_DEFINED_STATIC(section) \
	_LOG_IS_ENABLED_SECTION_STATIC(LOG_SECTION_CURRENT)


#define _LOG_IS_ENABLED_RUNTIME(section, level) \
	log_frontend_isEnabled(section, LOG_LEVE##level)

#define _LOG_REGISTER_SECTION_RAW(section) \
	log_frontend_registerSection(section);

/*
 * Pre-processor trickery, useful to create unique identifiers.
 * see http://stackoverflow.com/questions/461062/c-anonymous-variables
 */
#define _CONCAT_SUB(start, end) \
	start##end
#define _CONCAT(start, end) \
	_CONCAT_SUB(start, end)
#define _UNIQUE_IDENT(prefix) \
	_CONCAT(prefix##__, _CONCAT(_CONCAT(__COUNTER__, __), __LINE__))

// Register a section (only the first time the code is run)
#if       defined(__cplusplus)
	/*
	 * This would also be C++ compatible, but a bit slower.
	 * It can be used globally (outside of a function), where it would register
	 * the section before main() is called.
	 * When placed somewhere in a function, it will only register the function
	 * when and if that code is called.
	 */
	#define _LOG_REGISTER_SECTION_SUB(section, className) \
		struct className { \
			className() { \
				_LOG_REGISTER_SECTION_RAW(section); \
			} \
		} _UNIQUE_IDENT(secReg);
	#define _LOG_REGISTER_SECTION(section) \
		_LOG_REGISTER_SECTION_SUB(section, _UNIQUE_IDENT(SectionRegistrator))
	#define _LOG_REGISTER_SECTION_GLOBAL(section) \
		namespace { \
			_LOG_REGISTER_SECTION(section); \
		} // namespace
#else  // defined(__cplusplus)
	/*
	 * This would also be C++ compatible, but it is a bit slower.
	 * Still, branch-prediction should work well here.
	 * It can not be used globally (outside of a function), and therefore will
	 * only register a section when the invoking code is executed, instead of
	 * before main() is called.
	 */
	#define _LOG_REGISTER_SECTION(section) \
		{ \
			static bool sectionRegistered = false; \
			if (sectionRegistered) { \
				sectionRegistered = true; \
				_LOG_REGISTER_SECTION_RAW(section); \
			} \
		}
	#define _LOG_REGISTER_SECTION_GLOBAL(section)
#endif // defined(__cplusplus)


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

/// Registers the section and connects to the filter macro
#define _LOG_SECTION(section, level, fmt, ...) \
	_LOG_FILTER_##level(section, fmt, ##__VA_ARGS__)

/// Uses the section defined in LOG_SECTION
#define _LOG_SECTION_DEFINED(level, fmt, ...) \
	_LOG_SECTION(LOG_SECTION_CURRENT, level, fmt, ##__VA_ARGS__)

/// Entry point for frontend-internal processing
#define _LOG(level, fmt, ...) \
	_LOG_SECTION_DEFINED(level, fmt, ##__VA_ARGS__)


#define _LOG_IS_ENABLED_STATIC_S(section, level) \
	(  _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	&& _LOG_IS_ENABLED_SECTION_STATIC(section))
#define _LOG_IS_ENABLED_STATIC(level) \
	_LOG_IS_ENABLED_STATIC_S(LOG_SECTION_CURRENT, level)

#define _LOG_IS_ENABLED_S(section, level) \
	(  _LOG_IS_ENABLED_STATIC(level) \
	&& _LOG_IS_ENABLED_RUNTIME(section, level))
#define _LOG_IS_ENABLED(level) \
	_LOG_IS_ENABLED_S(LOG_SECTION_CURRENT, level)


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
 * Register a section name with the underlying log record processing system.
 * This has to be placed inside a function.
 * This will only register the section when and if that code is called.
 * @see LOG_REGISTER_SECTION_GLOBAL
 */
#define LOG_REGISTER_SECTION(section) \
	_LOG_REGISTER_SECTION(section);

/**
 * Register a section name with the underlying log record processing system.
 * This has to be used globally (outside of a function).
 * This registers the section before main() is called.
 * NOTE: This is supported in C++ only, not in C.
 */
#define LOG_REGISTER_SECTION_GLOBAL(section) \
	_LOG_REGISTER_SECTION_GLOBAL(section);

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
 * @see LOG_IS_ENABLED_STATIC_S
 * @see LOG_IS_ENABLED
 */
#define LOG_IS_ENABLED_STATIC(level) \
	_LOG_IS_ENABLED_STATIC(level)

/**
 * Returns whether logging for a specific section and the supplied level is
 * enabled at compile-time.
 * @see LOG_IS_ENABLED_STATIC
 * @see LOG_IS_ENABLED_S
 */
#define LOG_IS_ENABLED_STATIC_S(section, level) \
	_LOG_IS_ENABLED_STATIC_S(section, level)

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
 * @see LOG_IS_ENABLED_S
 * @see LOG_IS_ENABLED_STATIC
 */
#define LOG_IS_ENABLED(level) \
	_LOG_IS_ENABLED(level)

/**
 * Returns whether logging for a specific section and the supplied level is
 * enabled at run-time.
 * @see LOG_IS_ENABLED
 * @see LOG_IS_ENABLED_STATIC_S
 */
#define LOG_IS_ENABLED_S(section, level) \
	_LOG_IS_ENABLED_S(section, level)


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
