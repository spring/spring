/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_OUTPUT_H
#define LOG_OUTPUT_H

#include "lib/gml/gmlcnf.h"
#if defined(USE_GML) && GML_ENABLE_SIM
#include <boost/thread/mutex.hpp>
#endif

#include <string>
#include <vector>


/**
 * @brief logging class
 * Game UI elements that display log can subscribe to it to receive the log
 * messages.
 */
class CLogOutput
{
public:
	CLogOutput();
	~CLogOutput();

	/**
	 * @brief set the log file
	 *
	 * Relative paths are relative to the writable data-dir.
	 * This method may only be called as long as the logger is not yet
	 * initialized.
	 * @see Initialize()
	 */
	void SetFileName(std::string fileName);
	/**
	 * @brief returns the log file name (without path)
	 *
	 * Relative paths are relative to the writable data-dir.
	 */
	const std::string& GetFileName() const;
	/**
	 * @brief returns the absolute path to the log file
	 *
	 * Relative paths are relative to the writable data-dir.
	 * This method may only be called after the logger got initialized.
	 * @see Initialize()
	 */
	const std::string& GetFilePath() const;

	/**
	 * @brief initialize logOutput
	 *
	 * Only after calling this method, logOutput starts writing to disk.
	 * The log file is written in the current directory so this may only be called
	 * after the engine chdir'ed to the correct directory.
	 */
	void Initialize();

	/**
	 * Log()s system informations (CPU, 32/64bit, gcc/boost version, ...)
	 */
	static void LogSystemInfo();

private:
	void End();

	/**
	 * @brief initialize the log sections
	 *
	 * This writes a list of all available and all enabled sections to the log.
	 *
	 * Log sections can be enabled using the configuration key "LogSections",
	 * or the environment variable "SPRING_LOG_SECTIONS".
	 *
	 * Both specify a comma separated list of sections that should be enabled.
	 * The lists from both sources are combined, there is no overriding.
	 *
	 * A section that is enabled by default, can not be disabled.
	 */
	void InitializeSections();

	/**
	 * @brief creates an absolute file path from a file name
	 *
	 * Will use the CWD, which should be the writable data-dir format
	 * absoluteification.
	 */
	static std::string CreateFilePath(const std::string& fileName);

	/**
	 * @brief enable/disable log file rotation
	 *
	 * The default is determined by the config setting RotateLogFiles and
	 * whether this is a DEBUG build or not.
	 * RotateLogFiles defaults to "auto", and could be set to "never"
	 * or "always". On "auto", it will rotate logs only for debug builds.
	 * You may only call this as long as the logger did not yet get initialized.
	 */
	void SetLogFileRotating(bool enabled);
	bool IsLogFileRotating() const;
	/**
	 * @brief ff enabled, moves the log file of the last run
	 *
	 * Moves the log file of the last run, to preserve it,
	 * if log file rotation is enabled.
	 *
	 * By default, this is enabled only for DEBUG builds;
	 * ... (describe config file value here)
	 */
	void RotateLogFile() const;


	std::string fileName;
	std::string filePath;
	bool rotateLogFiles;

#if defined(USE_GML) && GML_ENABLE_SIM
	boost::mutex logmutex;
#endif
};


extern CLogOutput logOutput;

#endif // LOG_OUTPUT_H

