/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

/*
 * see ILog.h for documentation
 */

/**
 * @name logging_api
 */
///@{

#define LOG_LEVEL_ALL       0

#define LOG_LEVEL_DEBUG     20
#define LOG_LEVEL_INFO      30
#define LOG_LEVEL_NOTICE    35
#define LOG_LEVEL_WARNING   40
#define LOG_LEVEL_ERROR     50
#define LOG_LEVEL_FATAL     60

#define LOG_LEVEL_NONE     255

#define DEFAULT_LOG_LEVEL_SHORT         L_NOTICE
#define DEFAULT_LOG_LEVEL       LOG_LEVEL_NOTICE

///@}


/**
 * Minimal log level (compile-time).
 * This is the compile-time version of this setting;
 * there might be a more restrictive one at runtime.
 * Initialize to the default (may be overriden with -DLOG_LEVEL=X).
 */
#ifndef _LOG_LEVEL_MIN
	#ifdef DEBUG
		#define _LOG_LEVEL_MIN LOG_LEVEL_DEBUG
	#else
		#define _LOG_LEVEL_MIN LOG_LEVEL_INFO
	#endif
#endif

#endif // LOG_LEVEL_H
