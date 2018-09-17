/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_OUTPUT_H
#define LOG_OUTPUT_H

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
	const std::string& GetFileName() const { return fileName; }
	/**
	 * @brief returns the absolute path to the log file
	 *
	 * Relative paths are relative to the writable data-dir.
	 * This method may only be called after the logger got initialized.
	 * @see Initialize()
	 */
	const std::string& GetFilePath() const { return filePath; }

	/**
	 * @brief initialize logOutput
	 *
	 * Only after calling this method, logOutput starts writing to disk.
	 * The log file is written in the current directory so this may only be called
	 * after the engine chdir'ed to the correct directory.
	 */
	void Initialize();
	bool IsInitialized() const { return (!filePath.empty()); }


	/**
	 * Log()s system informations (CPU, 32/64bit, gcc/boost version, ...)
	 */
	static void LogSectionInfo();
	static void LogSystemInfo();
	static void LogConfigInfo();
	static void LogExceptionInfo(const char* src, const char* msg);

private:
	/**
	 * @brief creates an absolute file path from a file name
	 *
	 * Will use the CWD, which should be the writable data-dir format
	 * absoluteification.
	 */
	static std::string CreateFilePath(const std::string& fileName);

	/**
	 * @brief moves the log file of the last run
	 *
	 * Moves the log file of the last run, to preserve it,
	 *
	 */
	void RotateLogFile() const;


	std::string fileName;
	std::string filePath;

};


extern CLogOutput logOutput;

#endif // LOG_OUTPUT_H

