/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_FILE_SINK_H
#define LOG_FILE_SINK_H

/**
 * This is a simple sink for the ILog.h logging API.
 * It routes all (or a subset of) the log records to zero or more log files.
 */

#include <stdio.h> // FILE
#include <stdlib.h> // for NULL

#include "Level.h" // for LOG_LEVEL_ALL

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name logging_sink_file_control
 * ILog.h file-sink control interface.
 */
///@{

/**
 * Start logging to a file.
 * @param filePath the path to the log file
 * @param sections A list of comma separated sections to log to the file or
 *   NULL to log everything. To include the default section, you have to include
 * @param minLevel minimum log-level for this logfile
 * @param flushLevel every log message above this level is flushed to disk
 *   ",,".
 */
void log_file_addLogFile(const char* filePath, const char* sections = NULL,
		int minLevel = LOG_LEVEL_ALL, int flushLevel = LOG_LEVEL_ERROR);

FILE* log_file_getLogFileStream(const char* filePath);

void log_file_removeLogFile(const char* filePath);

void log_file_removeAllLogFiles();

///@}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_FILE_SINK_H

