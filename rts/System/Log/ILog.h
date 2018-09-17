/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_LOG_H
#define I_LOG_H

/*
 * Logging API
 * -----------
 * Light-weight & flexible logging API.
 *
 * Aims:
 * - Support a fixed set of severities levels:
 *   * L_DEBUG   : fine-grained information that is most useful to debug
 *   * L_INFO    : same as L_NOTICE just that it is surpressed on RELEASE builds when a non-default logSection is set
 *   * L_NOTICE  : default log level (always outputed)
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
 * There is a graphical diagram, explaining the basic structure of this API
 * under doc/logApi.dia (Dia diagram editor file).
 */

#ifdef __cplusplus
extern "C" {
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
extern bool log_frontend_isEnabled(int level, const char* section);

/**
 * Allows the global filter to maintain a set of all setions used in the binary.
 * This will be called once per each LOG*() line in the source.
 */
extern void log_frontend_register_section(const char* section);
extern void log_frontend_register_runtime_section(int level, const char* section);


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
extern void log_frontend_record(int level, const char* section, const char* fmt, ...) FORMAT_STRING(3);

#undef FORMAT_STRING

/**
 * @see LOG_CLEANUP
 */
extern void log_frontend_cleanup();


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Level & Section

#include "Level.h"
#include "Section.h"

#define _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	(LOG_LEVE##level >= _LOG_LEVEL_MIN)



// enable all log sections at compile-time
#define _LOG_IS_ENABLED_SECTION_STATIC(section) \
	true
#define _LOG_IS_ENABLED_SECTION_DEFINED_STATIC(section) \
	_LOG_IS_ENABLED_SECTION_STATIC(LOG_SECTION_CURRENT)


#define _LOG_IS_ENABLED_RUNTIME(section, level) \
	log_frontend_isEnabled(LOG_LEVE##level, section)

#define _LOG_REGISTER_SECTION_RAW(section) \
	log_frontend_register_section(section);

/*
 * Pre-processor trickery, useful to create unique identifiers.
 * see http://stackoverflow.com/questions/461062/c-anonymous-variables
 */
#define _STR_CONCAT_SUB(start, end)   start##end
#define _STR_CONCAT(start, end)   _STR_CONCAT_SUB(start, end)
#define _UNIQUE_IDENT(prefix)   _STR_CONCAT(prefix##__, _STR_CONCAT(_STR_CONCAT(__COUNTER__, __), __LINE__))

// Register a section (only the first time the code is run)
#if defined(__cplusplus)
	/*
	 * This would also be C++ compatible, but a bit slower.
	 * It can be used globally (outside of a function), where it would register
	 * the section before main() is called.
	 * When placed somewhere in a function, it will only register the function
	 * when and if that code is called.
	 */
	#define _LOG_REGISTER_SECTION_SUB(section, className)         \
		struct className {                                        \
			className() {                                         \
				_LOG_REGISTER_SECTION_RAW(section);               \
			}                                                     \
		} _UNIQUE_IDENT(secReg);

	#define _LOG_REGISTER_SECTION(section) \
		_LOG_REGISTER_SECTION_SUB(section, _UNIQUE_IDENT(SectionRegistrator))

	#define _LOG_REGISTER_SECTION_GLOBAL(section) \
		namespace {                               \
			_LOG_REGISTER_SECTION(section)        \
		} // namespace

#else  // defined(__cplusplus)

	/*
	 * This would also be C++ compatible, but it is a bit slower.
	 * Still, branch-prediction should work well here.
	 * It can not be used globally (outside of a function), and therefore will
	 * only register a section when the invoking code is executed, instead of
	 * before main() is called.
	 */
	#define _LOG_REGISTER_SECTION(section)           \
		{                                            \
			static bool sectionRegistered = false;   \
			if (!sectionRegistered) {                \
				sectionRegistered = true;            \
				_LOG_REGISTER_SECTION_RAW(section);  \
			}                                        \
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
 * In theory, we could also use this to completely disable logging at
 * compile-time already.
 */

/// Redirect to runtime processing
#define _LOG_RECORD(section, level, fmt, ...)   log_frontend_record(LOG_LEVE##level, section, fmt, ##__VA_ARGS__)

/// per level compile-time filter
#define _LOG_FILTER(section, level, fmt, ...) if (_LOG_IS_ENABLED_LEVEL_STATIC(level)) _LOG_RECORD(section, level, fmt, ##__VA_ARGS__)

/// Registers the section and connects to the filter macro
#define _LOG_SECTION(section, level, fmt, ...)   _LOG_FILTER(section, level, fmt, ##__VA_ARGS__)

/// Uses the section defined in LOG_SECTION
#define _LOG_SECTION_DEFINED(level, fmt, ...)   _LOG_SECTION(LOG_SECTION_CURRENT, level, fmt, ##__VA_ARGS__)

/// Entry point for frontend-internal processing
#define _LOG(level, fmt, ...)   _LOG_SECTION_DEFINED(level, fmt, ##__VA_ARGS__)


#define _LOG_IS_ENABLED_STATIC_S(section, level) \
	(  _LOG_IS_ENABLED_LEVEL_STATIC(level) \
	&& _LOG_IS_ENABLED_SECTION_STATIC(section))

#define _LOG_IS_ENABLED_S(section, level) \
	(  _LOG_IS_ENABLED_STATIC_S(section, level) \
	&& _LOG_IS_ENABLED_RUNTIME(section, level))

#define _LOG_IS_ENABLED_STATIC(level)   _LOG_IS_ENABLED_STATIC_S(LOG_SECTION_CURRENT, level)
#define _LOG_IS_ENABLED(level)   _LOG_IS_ENABLED_S(LOG_SECTION_CURRENT, level)


/// Redirect to runtime processing
#define _LOG_CLEANUP() \
	log_frontend_cleanup()



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// API

/**
 * @name logging_api
 * Only stuff from in this doc-group should ever be used by clients of this API.
 */
///@{

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
	_LOG_REGISTER_SECTION_GLOBAL(section)

/**
 * Returns whether logging for the current section and the supplied level is
 * enabled at compile-time.
 * This might return true, even if the back-end will ignore the message anyway.
 * You should consider using the LOG_IS_ENABLED macro instead (recommended).
 * This may be used with the preprocessor directly
 * (in an @code #if @endcode statement):
 * @code
 * #if LOG_IS_ENABLED_STATIC(L_DEBUG)
 *   const int statistics = calculateStats(); // performance-heavy
 *   LOG(L_DEBUG, "the stats: %i", statistics);
 * #endif
 * @endcode
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
	_LOG(DEFAULT_LOG_LEVEL_SHORT, fmt, ##__VA_ARGS__)

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

/**
 * Registers a log message with a specifiable integer level.
 * Usage example:
 * <code>
 * LOG_I(LOG_LEVEL_DEBUG, "my age is %i", 21);
 * LOG_I(20, "my age is %i", 21);
 * </code>
 * @see LOG()
 * @see LOG_L()
 */
#define LOG_I(level, fmt, ...) \
{ \
	if (level >= _LOG_LEVEL_MIN) { \
		log_frontend_record(level, LOG_SECTION_CURRENT, fmt, ##__VA_ARGS__); \
	} \
}

/**
 * Registers a log message with a specifiable section and integer level.
 * Usage example:
 * <code>
 * LOG_SI("one-time-section", 20, "my age is %i", 21);
 * </code>
 * @see LOG_I()
 * @see LOG_L()
 * @see LOG_S()
 * @see LOG_SECTION
 */
#define LOG_SI(section, level, fmt, ...) \
{ \
	if (level >= _LOG_LEVEL_MIN) { \
		log_frontend_record(level, section, fmt, ##__VA_ARGS__); \
	} \
}

/**
 * Informs all registered sinks to cleanup their state,
 * to be ready for a shutdown.
 * NOTE This is not a general way to cleanup the log sinks, but to be used in
 * exceptional occasions only, for example while handling a graceful crash.
 */
#define LOG_CLEANUP() \
	_LOG_CLEANUP()

///@}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} // extern "C"
#endif

#endif // I_LOG_H
