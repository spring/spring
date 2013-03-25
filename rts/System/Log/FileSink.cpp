/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSink.h"

#include "Backend.h"
#include "FramePrefixer.h"
#include "Level.h" // for LOG_LEVEL_*
#include "System/maindefines.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <list>
#include <map>


namespace {

	struct LogFileDetails {
		LogFileDetails(FILE* outStream = NULL, const std::string& sections = "",
				int minLevel = LOG_LEVEL_ALL, bool flush = true)
			: outStream(outStream)
			, sections(sections)
			, minLevel(minLevel)
			, flush(flush)
		{}

		FILE* GetOutStream() const {
			return outStream;
		}

		bool IsLogging(const char* section, int level) const {
			return ((level >= minLevel) && (sections.empty()
					|| (sections.find("," + std::string(section) + ",")
					!= std::string::npos)));
		}
		bool FlushOnWrite() const {
			return flush;
		}

	private:
		FILE* outStream;
		std::string sections;
		int minLevel;
		bool flush;
	};
	typedef std::map<std::string, LogFileDetails> logFiles_t;

	/**
	 * This is only used to check whether some code tries to access the
	 * log-files contianer after it got deleted.
	 */
	bool logFilesValidTracker = true;

	/**
	 * This class allows us to stop logging cleanly, when the application exits,
	 * and while the container is still valid (not deleted yet).
	 */
	struct LogFilesContainer {
		~LogFilesContainer() {
			log_file_removeAllLogFiles();
			logFilesValidTracker = false;
		}
		logFiles_t& GetLogFiles() {
			return logFiles;
		}

	private:
		logFiles_t logFiles;
	};

	inline logFiles_t& log_file_getLogFiles() {
		static LogFilesContainer logFilesContainer;

		assert(logFilesValidTracker);
		return logFilesContainer.GetLogFiles();
	}

	/**
	 * This class allows us to stop logging cleanly, when the application exits,
	 * and while the container is still valid (not deleted yet).
	 */
	struct LogRecord {
		LogRecord(const std::string& section, int level,
				const std::string& record)
			: section(section)
			, level(level)
			, record(record)
		{}

		const std::string& GetSection() const { return section; }
		int GetLevel() const { return level; }
		const std::string& GetRecord() const { return record; }

	private:
		std::string section;
		int level;
		std::string record;
	};
	typedef std::list<LogRecord> logRecords_t;

	inline logRecords_t& log_file_getRecordBuffer() {
		static logRecords_t buffer;
		return buffer;
	}

	inline bool log_file_isActivelyLogging() {
		return (!log_file_getLogFiles().empty());
	}

	void log_file_writeToFile(FILE* outStream, const char* record, bool flush) {

		char framePrefix[128] = {'\0'};
		log_framePrefixer_createPrefix(framePrefix, sizeof(framePrefix));

		FPRINTF(outStream, "%s%s\n", framePrefix, record);

		if (flush)
			fflush(outStream);
	}

	/**
	 * Writes to the individual log files, if they do want to log the section.
	 */
	void log_file_writeToFiles(const char* section, int level,
			const char* record)
	{
		const logFiles_t& logFiles = log_file_getLogFiles();
		logFiles_t::const_iterator lfi;
		for (lfi = logFiles.begin(); lfi != logFiles.end(); ++lfi) {
			if (lfi->second.IsLogging(section, level)
					&& (lfi->second.GetOutStream() != NULL))
			{
				log_file_writeToFile(lfi->second.GetOutStream(), record, lfi->second.FlushOnWrite());
			}
		}
	}

	/**
	 * Flushes the buffer of a single log file.
	 */
	void log_file_flushFile(FILE* outStream) {
		fflush(outStream);
	}

	/**
	 * Flushes the buffers of the individual log files.
	 */
	void log_file_flushFiles() {
		const logFiles_t& logFiles = log_file_getLogFiles();
		logFiles_t::const_iterator lfi;
		for (lfi = logFiles.begin(); lfi != logFiles.end(); ++lfi) {
			if (lfi->second.GetOutStream() != NULL) {
				log_file_flushFile(lfi->second.GetOutStream());
			}
		}
	}

	/**
	 * Writes the content of the buffer to all the currently registered log
	 * files.
	 */
	void log_file_writeBufferToFiles() {

		while (!log_file_getRecordBuffer().empty()) {
			logRecords_t& logRecords = log_file_getRecordBuffer();
			const logRecords_t::iterator lri = logRecords.begin();
			log_file_writeToFiles(lri->GetSection().c_str(), lri->GetLevel(),
					lri->GetRecord().c_str());
			logRecords.erase(lri);
		}
	}

	inline void log_file_writeToBuffer(const std::string& section, int level,
			const std::string& record)
	{
		log_file_getRecordBuffer().push_back(LogRecord(section, level, record));
	}
}


#ifdef __cplusplus
extern "C" {
#endif

void log_file_addLogFile(const char* filePath, const char* sections, int minLevel, bool flush) {

	assert(filePath != NULL);

	logFiles_t& logFiles = log_file_getLogFiles();
	const std::string filePathStr = filePath;
	const logFiles_t::const_iterator lfi = logFiles.find(filePathStr);
	if (lfi != logFiles.end()) {
		// we are already logging to this file
		return;
	}

	FILE* tmpStream = fopen(filePath, "w");
	if (tmpStream == NULL) {
		LOG_L(L_ERROR, "Failed to open log file for writing: %s", filePath);
		return;
	}

	setvbuf(tmpStream, NULL, _IOFBF, (BUFSIZ < 8192) ? BUFSIZ : 8192); // limit buffer to 8kB

	const std::string sectionsStr = (sections == NULL) ? "" : sections;
	logFiles[filePathStr] = LogFileDetails(tmpStream, sectionsStr, minLevel, flush);
}

void log_file_removeLogFile(const char* filePath) {

	assert(filePath != NULL);

	logFiles_t& logFiles = log_file_getLogFiles();
	const std::string filePathStr = filePath;
	const logFiles_t::iterator lfi = logFiles.find(filePathStr);
	if (lfi == logFiles.end()) {
		// we are not logging to this file
		return;
	}

	// turn off logging to this file
	FILE* tmpStream = lfi->second.GetOutStream();
	logFiles.erase(lfi);
	fclose(tmpStream);
	tmpStream = NULL;
}

void log_file_removeAllLogFiles() {

	while (!log_file_getLogFiles().empty()) {
		const logFiles_t::const_iterator lfi = log_file_getLogFiles().begin();
		log_file_removeLogFile(lfi->first.c_str());
	}
}

/**
 * @name logging_sink_file
 * ILog.h sink implementation.
 */
///@{

/// Records a log entry
static void log_sink_record_file(const char* section, int level,
		const char* record)
{
	if (log_file_isActivelyLogging()) {
		// write buffer to log file
		log_file_writeBufferToFiles();

		// write current record to log file
		log_file_writeToFiles(section, level, record);
	} else {
		// buffer until a log file is ready for output
		log_file_writeToBuffer(section, level, record);
	}
}

/// Cleans up all log streams, by flushing them.
static void log_sink_cleanup_file() {
	if (log_file_isActivelyLogging()) {
		// flush the log buffers to files
		log_file_flushFiles();
	}
}

///@}


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct FileSinkRegistrator {
		FileSinkRegistrator() {
			log_backend_registerSink(&log_sink_record_file);
			log_backend_registerCleanup(&log_sink_cleanup_file);
		}
	} fileSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

