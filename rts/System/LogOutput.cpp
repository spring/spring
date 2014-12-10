/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/LogOutput.h"

#include "System/Util.h"
#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/FileSink.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"
#include "System/Log/LogUtil.h"
#include "System/Platform/Misc.h"

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include <cassert>
#include <cstring>

#include <boost/thread/recursive_mutex.hpp>


/******************************************************************************/
/******************************************************************************/

CONFIG(bool, RotateLogFiles).defaultValue(false)
		.description("rotate logfiles, old logfiles will be moved into the subfolder \"log\".");

CONFIG(std::string, LogSections).defaultValue("")
		.description("Comma seperated list of enabled logsections, see infolog.txt / console output for possible values");

CONFIG(int, LogFlushLevel).defaultValue(LOG_LEVEL_ERROR)
		.description("Flush the logfile when level of message is above LogFlushLevel. i.e. ERROR is flushed as default, WARNING isn't.");

/******************************************************************************/
/******************************************************************************/

static std::map<std::string, int> GetEnabledSections() {
	std::map<std::string, int> sectionLevelMap;

	std::string enabledSections = ",";
	std::string envSections = ",";

#if defined(UNITSYNC)
	#if defined(DEBUG)
	// unitsync logging in debug mode always on
	// configHandler cannot be accessed here in unitsync, as it may not exist.
	enabledSections += "unitsync,ArchiveScanner,";
	#endif
#else
	#if defined(DEDICATED)
	enabledSections += "DedicatedServer,";
	#endif
	#if !defined(DEBUG)
	// Always show at least INFO level of these sections
	enabledSections += "Sound,";
	#endif
	enabledSections += StringToLower(configHandler->GetString("LogSections"));
#endif

	if (getenv("SPRING_LOG_SECTIONS") != NULL) {
		// allow disabling all sections from the env var by setting it to "none"
		envSections += getenv("SPRING_LOG_SECTIONS");
		envSections = StringToLower(envSections);

		if (envSections == "none") {
			enabledSections = "";
		} else {
			enabledSections += envSections;
		}
	}

	enabledSections = StringToLower(enabledSections);
	enabledSections = StringStrip(enabledSections, " \t\n\r");

	// make the last "section:level" substring findable
	if (!enabledSections.empty() && enabledSections.back() != ',')
		enabledSections += ",";

	// n=1 because <enabledSections> always starts with a ',' (if non-empty)
	for (size_t n = 1; n < enabledSections.size(); ) {
		const size_t k = enabledSections.find(",", n);

		if (k != std::string::npos) {
			const std::string& sub = enabledSections.substr(n, k - n);

			if (!sub.empty()) {
				const size_t sepChr = sub.find(":");

				const std::string& logSec = (sepChr != std::string::npos)? sub.substr(         0,            sepChr): sub;
				const std::string& logLvl = (sepChr != std::string::npos)? sub.substr(sepChr + 1, std::string::npos):  "";

				if (!logLvl.empty()) {
					sectionLevelMap[logSec] = StringToInt(logLvl);
				} else {
					#if defined(DEBUG)
					sectionLevelMap[logSec] = LOG_LEVEL_DEBUG;
					#else
					sectionLevelMap[logSec] = DEFAULT_LOG_LEVEL;
					#endif

				}
			}

			n = k + 1;
		} else {
			n = k;
		}
	}

	return sectionLevelMap;
}

/**
 * @brief initialize the log sections
 *
 * This writes a list of all available and all enabled sections to the log.
 *
 * Log sections can be enabled using the configuration key "LogSections",
 * or the environment variable "SPRING_LOG_SECTIONS".
 *
 * Both specify a comma-separated list of sections that should be enabled.
 * The lists from both sources are combined, there is no overriding.
 *
 * A section that is enabled by default, can not be disabled.
 */
static void InitializeLogSections()
{
	// the new systems (ILog.h) log-sub-systems are called sections
	const std::set<const char*>& registeredSections = log_filter_section_getRegisteredSet();

	// enabled sections is a superset of the ones specified in the
	// environment and the ones specified in the configuration file.
	const std::map<std::string, int>& enabledSections = GetEnabledSections();

	std::stringstream availableLogSectionsStr;
	std::stringstream enabledLogSectionsStr;

	availableLogSectionsStr << "Available log sections: ";
	enabledLogSectionsStr << "Enabled log sections: ";

	unsigned int numRegisteredSections = 0;
	unsigned int numEnabledSections = 0;

	for (auto si = registeredSections.begin(); si != registeredSections.end(); ++si) {
		numRegisteredSections++;

		availableLogSectionsStr << ((numRegisteredSections > 1)? ", ": "");
		availableLogSectionsStr << *si;

		// enabled sections (keys) are in lower-case
		const auto sectionIter = enabledSections.find(StringToLower(*si));

		// skip if section is registered but not enabled
		if (sectionIter == enabledSections.end())
			continue;

		// user-specified wanted level for this section
		const int sectionLevel = sectionIter->second;

		if (sectionLevel >= LOG_LEVEL_NONE)
			continue;

		// find the nearest lower known log-level (in descending order)
		const int logLevel = log_util_getNearestLevel(sectionLevel);

		// levels can't go lower than this
		if (logLevel < 0)
			continue;

		log_filter_section_setMinLevel(*si, logLevel);

		enabledLogSectionsStr << ((numEnabledSections > 0)? ", ": "");
		enabledLogSectionsStr << *si << "(" << log_util_levelToString(logLevel) << ")";

		numEnabledSections++;
	}

	LOG("%s", (availableLogSectionsStr.str()).c_str());
	LOG("%s", (enabledLogSectionsStr.str()).c_str());

	LOG("Enable or disable log sections using the LogSections configuration key");
	LOG("  or the SPRING_LOG_SECTIONS environment variable (both comma separated).");
	LOG("  Use \"none\" to disable the default log sections.");
}




CLogOutput logOutput;

CLogOutput::CLogOutput()
	: fileName("")
	, filePath("")
{
	// multiple infologs can't exist together!
	assert(this == &logOutput);

	SetFileName("infolog.txt");
}

CLogOutput::~CLogOutput()
{
}

void CLogOutput::SetFileName(std::string fname)
{
	assert(!IsInitialized());
	fileName = fname;
}

std::string CLogOutput::CreateFilePath(const std::string& fileName)
{
	return (FileSystem::EnsurePathSepAtEnd(FileSystem::GetCwd()) + fileName);
}


void CLogOutput::RotateLogFile() const
{
	if (!FileSystem::FileExists(filePath))
		return;

	// logArchiveDir: /absolute/writeable/data/dir/log/
	const std::string logArchiveDir = filePath.substr(0, filePath.find_last_of("/\\") + 1) + "log" + FileSystem::GetNativePathSeparator();
	const std::string archivedLogFile = logArchiveDir + FileSystem::GetFileModificationDate(filePath) + "_" + fileName;

	// create the log archive dir if it does not exist yet
	if (!FileSystem::DirExists(logArchiveDir))
		FileSystem::CreateDirectory(logArchiveDir);

	// move the old log to the archive dir
	if (rename(filePath.c_str(), archivedLogFile.c_str()) != 0) {
		// no log here yet
		std::cerr << "Failed rotating the log file" << std::endl;
	}
}



void CLogOutput::Initialize()
{
	assert(configHandler != NULL);

	if (IsInitialized())
		return;

	filePath = CreateFilePath(fileName);

	if (configHandler->GetBool("RotateLogFiles"))
		RotateLogFile();

	log_file_addLogFile(filePath.c_str(), NULL, LOG_LEVEL_ALL, configHandler->GetInt("LogFlushLevel"));
	InitializeLogSections();

	LOG("LogOutput initialized.");
}



void CLogOutput::LogSystemInfo()
{
	LOG("Spring %s", SpringVersion::GetFull().c_str());
	LOG("Build Date & Time: %s", SpringVersion::GetBuildTime().c_str());
	LOG("Build Environment: %s", SpringVersion::GetBuildEnvironment().c_str());
	LOG("Compiler Version:  %s", SpringVersion::GetCompiler().c_str());
	LOG("Operating System:  %s", Platform::GetOS().c_str());

	if (Platform::Is64Bit()) {
		LOG("Word Size:         64-bit (native mode)");
	} else if (Platform::Is32BitEmulation()) {
		LOG("Word Size:         32-bit (emulated)");
	} else {
		LOG("Word Size:         32-bit (native mode)");
	}
}

