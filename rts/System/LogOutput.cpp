/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/LogOutput.h"

#include "lib/gml/gmlmut.h"
#include "System/Util.h"
#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/FileSink.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"
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

/*
LogFlush defaults to true, because there should be pretty low performance gains as most
fwrite implementations cache on their own. Also disabling this causes stack-traces to
be cut off. BEFORE letting it default to false again, verify that it really increases
performance as the drawbacks really suck.
*/
CONFIG(bool, LogFlush).defaultValue(true)
		.description("Instantly write to the logfile, use only for debugging as it will cause a slowdown");

/******************************************************************************/
/******************************************************************************/

CLogOutput logOutput;

static std::ofstream* filelog = NULL;
static bool initialized = false;

CLogOutput::CLogOutput()
	: fileName("")
	, filePath("")
{
	// multiple infologs can't exist together!
	assert(this == &logOutput);
	assert(!filelog);

	SetFileName("infolog.txt");

}


CLogOutput::~CLogOutput()
{
	GML_STDMUTEX_LOCK_NOPROF(log); // End

	SafeDelete(filelog);
}

const std::string& CLogOutput::GetFileName() const
{
	return fileName;
}
const std::string& CLogOutput::GetFilePath() const
{
	assert(initialized);
	return filePath;
}
void CLogOutput::SetFileName(std::string fname)
{
	GML_STDMUTEX_LOCK_NOPROF(log); // SetFileName

	assert(!initialized);
	fileName = fname;
}

std::string CLogOutput::CreateFilePath(const std::string& fileName)
{
	return FileSystem::EnsurePathSepAtEnd(FileSystem::GetCwd()) + fileName;
}


void CLogOutput::RotateLogFile() const
{
	if (FileSystem::FileExists(filePath)) {
		// logArchiveDir: /absolute/writeable/data/dir/log/
		std::string logArchiveDir = filePath.substr(0, filePath.find_last_of("/\\") + 1);
		logArchiveDir = logArchiveDir + "log" + FileSystem::GetNativePathSeparator();

		const std::string archivedLogFile = logArchiveDir + FileSystem::GetFileModificationDate(filePath) + "_" + fileName;

		// create the log archive dir if it does not exist yet
		if (!FileSystem::DirExists(logArchiveDir)) {
			FileSystem::CreateDirectory(logArchiveDir);
		}

		// move the old log to the archive dir
		const int moveError = rename(filePath.c_str(), archivedLogFile.c_str());
		if (moveError != 0) {
			// no log here yet
			std::cerr << "Failed rotating the log file" << std::endl;
		}
	}
}


bool CLogOutput::IsInitialized()
{
	return initialized;
}


void CLogOutput::Initialize()
{
	assert(configHandler!=NULL);

	if (initialized) return;

	filePath = CreateFilePath(fileName);

	if (configHandler->GetBool("RotateLogFiles")) {
		RotateLogFile();
	}

	log_file_addLogFile(filePath.c_str(), NULL, LOG_LEVEL_ALL, configHandler->GetBool("LogFlush"));

	initialized = true;
	InitializeSections();

	LOG("LogOutput initialized.");
}



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

	// <enabledSections> always starts with a ','
	for (size_t n = 1; n < enabledSections.size(); ) {
		const size_t k = enabledSections.find(",", n);

		if (k != std::string::npos) {
			const std::string& sub = enabledSections.substr(n, k - n);
			const std::string& sec = sub.substr(0, std::min(sub.size(), sub.find(":")));
			const std::string& lvl = sub.substr(std::min(sub.size(), sub.find(":") + 1), std::string::npos);

			if (!lvl.empty()) {
				sectionLevelMap[sec] = StringToInt(lvl);
			} else {
				#if defined(DEBUG)
				sectionLevelMap[sec] = LOG_LEVEL_DEBUG;
				#else
				sectionLevelMap[sec] = LOG_LEVEL_INFO;
				#endif

			}

			n = k + 1;
		} else {
			n = k;
		}
	}

	return sectionLevelMap;
}

void CLogOutput::InitializeSections()
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

	for (auto si = registeredSections.begin(); si != registeredSections.end(); ++si) {
		if (si != registeredSections.begin()) {
			availableLogSectionsStr << ", ";
		}

		availableLogSectionsStr << *si;

		{
			const std::string sectionName = StringToLower(*si);
			const auto sectionIter = enabledSections.find(sectionName);

			// skip if section is registered but not enabled
			if (sectionIter == enabledSections.end())
				continue;

			const int sectionLevel = sectionIter->second;

			// TODO: convert sectionLevel to nearest lower LOG_LEVEL_*
			#if defined(DEBUG)
			log_filter_section_setMinLevel(*si, LOG_LEVEL_DEBUG);
			enabledLogSectionsStr << *si << "(LOG_LEVEL_DEBUG)";
			#else
			log_filter_section_setMinLevel(*si, LOG_LEVEL_INFO);
			enabledLogSectionsStr << *si << "(LOG_LEVEL_INFO)";
			#endif
		}

		if (si != registeredSections.begin() && si != (--registeredSections.end())) {
			enabledLogSectionsStr << ", ";
		}
	}

	LOG("%s", (availableLogSectionsStr.str()).c_str());
	LOG("%s", (enabledLogSectionsStr.str()).c_str());

	LOG("Enable or disable log sections using the LogSections configuration key");
	LOG("  or the SPRING_LOG_SECTIONS environment variable (both comma separated).");
	LOG("  Use \"none\" to disable the default log sections.");
}


void CLogOutput::LogSystemInfo()
{
	LOG("Spring %s", SpringVersion::GetFull().c_str());
	LOG("Build date/time: %s", SpringVersion::GetBuildTime().c_str());
	LOG("Build environment: %s", SpringVersion::GetBuildEnvironment().c_str());
	LOG("Compiler: %s", SpringVersion::GetCompiler().c_str());
	LOG("OS: %s", Platform::GetOS().c_str());
	if (Platform::Is64Bit())
		LOG("OS: 64bit native mode");
	else if (Platform::Is32BitEmulation())
		LOG("OS: emulated 32bit mode");
	else
		LOG("OS: 32bit native mode");
}

