/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FileSink.h"
#include "Backend.h"
#include "FramePrefixer.h"
#include "Level.h" // for LOG_LEVEL_*
#include "System/MainDefines.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"

#include <cassert>
#include <cstdio>
#include <string>

#include <algorithm>
#include <vector>


namespace log_file {
	struct LogFileDetails {
		LogFileDetails(
			FILE* outStream = nullptr,
			const std::string& sections = "",
			int minLevel = LOG_LEVEL_ALL,
			int flushLevel = LOG_LEVEL_ERROR
		)
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

	/**
	 * This is only used to check whether some code tries to access the
	 * log-files container after it got deleted.
	 */
	bool validTracker = true;


	/**
	 * This class allows us to stop logging cleanly, when the application exits,
	 * and while the container is still valid (not deleted yet).
	 */
	struct LogFilesContainer {
	public:
		typedef std::pair<std::string, LogFileDetails> LogFilePair;
		typedef std::vector<LogFilePair> LogFilesMap;

		~LogFilesContainer() {
			log_file_removeAllLogFiles();
			validTracker = false;
		}
		LogFilesMap& GetLogFiles() {
			return logFiles;
		}

	private:
		std::vector< std::pair<std::string, LogFileDetails> > logFiles;
	};

	using LogFilePair = LogFilesContainer::LogFilePair;
	using LogFilesMap = LogFilesContainer::LogFilesMap;


	inline LogFilesMap& getLogFiles() {
		static LogFilesContainer logFilesContainer;

		assert(validTracker);
		return (logFilesContainer.GetLogFiles());
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
	typedef std::vector<LogRecord> logRecords_t;


	inline logRecords_t& getRecordBuffer() {
		static logRecords_t buffer;
		return buffer;
	}

	inline bool isActivelyLogging() {
		return (!getLogFiles().empty());
	}

	void writeToFile(FILE* outStream, const char* record, bool flush) {
		char framePrefix[128] = {'\0'};
		log_framePrefixer_createPrefix(framePrefix, sizeof(framePrefix));

		FPRINTF(outStream, "%s%s\n", framePrefix, record);

		if (flush)
			fflush(outStream);
	}

	/**
	 * Writes to the individual log files, if they do want to log the section.
	 */
	void writeToFiles(int level, const char* section, const char* record)
	{
		const auto& logFiles = getLogFiles();

		for (const auto& p: logFiles) {
			if (!p.second.IsLogging(level, section))
				continue;
			if (p.second.GetOutStream() == nullptr)
				continue;

			writeToFile(p.second.GetOutStream(), record, p.second.FlushOnWrite(level));
		}
	}

	/**
	 * Flushes the buffers of the individual log files.
	 */
	void flushFiles() {
		const auto& logFiles = getLogFiles();

		for (const auto& p: logFiles) {
			if (p.second.GetOutStream() == nullptr)
				continue;

			fflush(p.second.GetOutStream());
		}
	}
	/**

	 * Writes the content of the buffer to all the currently registered log
	 * files.
	 */
	void writeBufferToFiles() {
		logRecords_t& logRecords = getRecordBuffer();

		for (LogRecord& logRec: logRecords) {
			writeToFiles(logRec.GetLevel(), logRec.GetSection().c_str(), logRec.GetRecord().c_str());
		}

		logRecords.clear();
	}

	inline void writeToBuffer(int level, const std::string& section, const std::string& record)
	{
		logRecords_t& logRecords = getRecordBuffer();

		if (logRecords.empty())
			logRecords.reserve(1024);

		logRecords.emplace_back(level, section, record);
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
	assert(filePath != nullptr);

	auto& logFiles = log_file::getLogFiles();

	const std::string sectionsStr = (sections == nullptr) ? "" : sections;
	const std::string filePathStr = filePath;

	const auto pred = [](const log_file::LogFilePair& a, const log_file::LogFilePair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(logFiles.begin(), logFiles.end(), log_file::LogFilePair{filePathStr, nullptr}, pred);

	// we are already logging to this file
	if (iter != logFiles.end() && iter->first == filePathStr)
		return;

	FILE* tmpStream = fopen(filePath, "w");

	if (tmpStream == nullptr) {
		LOG_L(L_ERROR, "[%s] failed to open log file \"%s\" for writing", __func__, filePath);
		return;
	}

	setvbuf(tmpStream, nullptr, _IOFBF, std::min(BUFSIZ, 8192)); // limit buffer to 8kB

	logFiles.emplace_back(filePathStr, log_file::LogFileDetails(tmpStream, sectionsStr, minLevel, flushLevel));

	// swap into position; only a handful of files are ever added
	for (size_t i = logFiles.size() - 1; i > 0; i--) {
		if (logFiles[i - 1].first < logFiles[i].first)
			break;

		std::swap(logFiles[i - 1], logFiles[i]);
	}
}

void log_file_removeLogFile(const char* filePath) {
	assert(filePath != nullptr);

	auto& logFiles = log_file::getLogFiles();

	const auto pred = [](const log_file::LogFilePair& a, const log_file::LogFilePair& b) { return (a.first < b.first); };
	const auto iter = std::lower_bound(logFiles.begin(), logFiles.end(), log_file::LogFilePair{filePath, nullptr}, pred);

	// we are not logging to this file
	if (iter == logFiles.end() || strcmp(iter->first.c_str(), filePath) != 0)
		return;

	// turn off logging to this file
	fclose(iter->second.GetOutStream());

	// erase entry
	for (size_t i = iter - logFiles.begin(), n = logFiles.size() - 1; i < n; i++) {
		logFiles[i].first  = std::move(logFiles[i + 1].first );
		logFiles[i].second = std::move(logFiles[i + 1].second);
	}

	logFiles.pop_back();
}

void log_file_removeAllLogFiles() {
	auto& logFiles = log_file::getLogFiles();

	for (auto& logFilePair: logFiles) {
		fclose(logFilePair.second.GetOutStream());
	}

	logFiles.clear();
}


FILE* log_file_getLogFileStream(const char* filePath) {
	const auto& logFiles = log_file::getLogFiles();

	for (const auto& p: logFiles) {
		if (strcmp((p.first).c_str(), filePath) != 0)
			continue;

		return ((p.second).GetOutStream());
	}

	return nullptr;
}



/**
 * @name logging_sink_file
 * ILog.h sink implementation.
 */
///@{

/// Records a log entry
static void log_sink_record_file(int level, const char* section, const char* record)
{
	if (log_file::validTracker && log_file::isActivelyLogging()) {
		// write buffer to log file
		log_file::writeBufferToFiles();

		// write current record to log file
		log_file::writeToFiles(level, section, record);
	} else {
		// buffer until a log file is ready for output
		log_file::writeToBuffer(level, section, record);
	}
}

/// Cleans up all log streams, by flushing them.
static void log_sink_cleanup_file() {
	if (!log_file::isActivelyLogging())
		return;

	// flush the log buffers to files
	log_file::flushFiles();
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

