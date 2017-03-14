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
				int minLevel = LOG_LEVEL_ALL, int flushLevel = LOG_LEVEL_ERROR)
			: outStream(outStream)
			, sections(sections)
			, minLevel(minLevel)
			, flushLevel(flushLevel)
		{}

		FILE* GetOutStream() const {
			return outStream;
		}

		bool IsLogging(int level, const char* section) const {
			return ((level >= minLevel) && (sections.empty() || (sections.find("," + std::string(section) + ",") != std::string::npos)));
		}
		bool FlushOnWrite(int level) const {
			return (level >= flushLevel);
		}

	private:
		FILE* outStream;
		std::string sections;
		int minLevel;
		int flushLevel;
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
		LogRecord(int level, const std::string& section, const std::string& record)
			: level(level)
			, section(section)
			, record(record)
		{}

		int GetLevel() const { return level; }
		const std::string& GetSection() const { return section; }
		const std::string& GetRecord() const { return record; }

	private:
		int level;
		std::string section;
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
	void log_file_writeToFiles(int level, const char* section, const char* record)
	{
		const logFiles_t& logFiles = log_file_getLogFiles();

		for (auto lfi = logFiles.begin(); lfi != logFiles.end(); ++lfi) {
			if (lfi->second.IsLogging(level, section) && (lfi->second.GetOutStream() != NULL)) {
				log_file_writeToFile(lfi->second.GetOutStream(), record, lfi->second.FlushOnWrite(level));
			}
		}
	}

	/**
	 * Flushes the buffers of the individual log files.
	 */
	void log_file_flushFiles() {
		const logFiles_t& logFiles = log_file_getLogFiles();

		for (auto lfi = logFiles.begin(); lfi != logFiles.end(); ++lfi) {
			if (lfi->second.GetOutStream() != NULL) {
				fflush(lfi->second.GetOutStream());
			}
		}
	}

	/**
	 * Writes the content of the buffer to all the currently registered log
	 * files.
	 */
	void log_file_writeBufferToFiles() {
		logRecords_t& logRecords = log_file_getRecordBuffer();

		while (!logRecords.empty()) {
			const auto lri = logRecords.begin();
			log_file_writeToFiles(lri->GetLevel(), lri->GetSection().c_str(), lri->GetRecord().c_str());
			logRecords.erase(lri);
		}
	}

	inline void log_file_writeToBuffer(int level, const std::string& section, const std::string& record)
	{
		log_file_getRecordBuffer().push_back(LogRecord(level, section, record));
	}
}


#ifdef __cplusplus
extern "C" {
#endif

void log_file_addLogFile(
	const char* filePath,
	const char* sections,
	int minLevel,
	int flushLevel
) {
	assert(filePath != NULL);

	logFiles_t& logFiles = log_file_getLogFiles();

	const std::string sectionsStr = (sections == NULL) ? "" : sections;
	const std::string filePathStr = filePath;
	const auto lfi = logFiles.find(filePathStr);

	// we are already logging to this file
	if (lfi != logFiles.end())
		return;

	FILE* tmpStream = fopen(filePath, "w");

	if (tmpStream == NULL) {
		LOG_L(L_ERROR, "Failed to open log file for writing: %s", filePath);
		return;
	}

	setvbuf(tmpStream, NULL, _IOFBF, (BUFSIZ < 8192) ? BUFSIZ : 8192); // limit buffer to 8kB

	logFiles[filePathStr] = LogFileDetails(tmpStream, sectionsStr, minLevel, flushLevel);
}

void log_file_removeLogFile(const char* filePath) {
	assert(filePath != NULL);

	logFiles_t& logFiles = log_file_getLogFiles();

	const std::string filePathStr = filePath;
	const auto lfi = logFiles.find(filePathStr);

	// we are not logging to this file
	if (lfi == logFiles.end())
		return;

	// turn off logging to this file
	FILE* tmpStream = lfi->second.GetOutStream();
	logFiles.erase(lfi);
	fclose(tmpStream);
}

void log_file_removeAllLogFiles() {
	while (!log_file_getLogFiles().empty()) {
		const auto lfi = log_file_getLogFiles().begin();
		log_file_removeLogFile(lfi->first.c_str());
	}
}


/**
 * @name logging_sink_file
 * ILog.h sink implementation.
 */
///@{

/// Records a log entry
static void log_sink_record_file(int level, const char* section, const char* record)
{
	if (logFilesValidTracker && log_file_isActivelyLogging()) {
		// write buffer to log file
		log_file_writeBufferToFiles();

		// write current record to log file
		log_file_writeToFiles(level, section, record);
	} else {
		// buffer until a log file is ready for output
		log_file_writeToBuffer(level, section, record);
	}
}

/// Cleans up all log streams, by flushing them.
static void log_sink_cleanup_file() {
	if (!log_file_isActivelyLogging())
		return;

	// flush the log buffers to files
	log_file_flushFiles();
}

///@}


namespace {
	/// Auto-registers the sink defined in this file before main() is called
	struct FileSinkRegistrator {
		FileSinkRegistrator() {
			log_backend_registerSink(&log_sink_record_file);
			log_backend_registerCleanup(&log_sink_cleanup_file);
		}
		~FileSinkRegistrator() {
			log_backend_unregisterSink(&log_sink_record_file);
			log_backend_unregisterCleanup(&log_sink_cleanup_file);
		}
	} fileSinkRegistrator;
}

#ifdef __cplusplus
} // extern "C"
#endif

