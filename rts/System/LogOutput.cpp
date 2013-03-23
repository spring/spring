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

CONFIG(std::string, RotateLogFiles).defaultValue("auto")
		.description("rotate logfiles, valid values are \"always\" (default in debug builds) and \"never\" (default in release builds).");
CONFIG(std::string, LogSections).defaultValue("")
		.description("Comma seperated list of enabled logsections, see infolog.txt / console output for possible values");

/*
LogFlush defaults to true, because there should be pretty low performance gains as most
fwrite implementations cache on their own. Also cut-off logs often caused stack-traces
to be cut off. BEFORE letting it default to false again, verify that it really increases
performance as the drawbacks really suck
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

	bool doRotateLogFiles = false;
	std::string rotatePolicy = "auto";
	if (configHandler != NULL) {
		rotatePolicy = configHandler->GetString("RotateLogFiles");
	}
	if (rotatePolicy == "always") {
		doRotateLogFiles = true;
	} else if (rotatePolicy == "never") {
		doRotateLogFiles = false;
	} else { // auto
#ifdef DEBUG
		doRotateLogFiles = true;
#else
		doRotateLogFiles = false;
#endif
	}
	SetLogFileRotating(doRotateLogFiles);
}


CLogOutput::~CLogOutput()
{
	End();
}


void CLogOutput::End()
{
	GML_STDMUTEX_LOCK_NOPROF(log); // End

	SafeDelete(filelog);
	//log_file_removeLogFile(filePath.c_str());
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
	return FileSystem::GetCwd() + (char)FileSystem::GetNativePathSeparator() + fileName;
}


void CLogOutput::SetLogFileRotating(bool enabled)
{
	assert(!initialized);
	rotateLogFiles = enabled;
}
bool CLogOutput::IsLogFileRotating() const
{
	return rotateLogFiles;
}

void CLogOutput::RotateLogFile() const
{
	if (IsLogFileRotating()) {
		if (FileSystem::FileExists(filePath)) {
			// logArchiveDir: /absolute/writeable/data/dir/log/
			std::string logArchiveDir = filePath.substr(0, filePath.find_last_of("/\\") + 1);
			logArchiveDir = logArchiveDir + "log" + (char)FileSystem::GetNativePathSeparator();

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
}

void CLogOutput::Initialize()
{
	if (initialized) return;

	filePath = CreateFilePath(fileName);
	RotateLogFile();

	/*filelog = new std::ofstream(filePath.c_str());
	if (filelog->bad())
		SafeDelete(filelog);*/
	const bool flush = configHandler->GetBool("LogFlush");
	log_file_addLogFile(filePath.c_str(), NULL, LOG_LEVEL_ALL, flush);

	initialized = true;
	InitializeSections();

	/*std::vector<std::string>::iterator pili;
	for (pili = preInitLog().begin(); pili != preInitLog().end(); ++pili) {
		ToFile(*pili);
	}
	preInitLog().clear();*/

	LOG("LogOutput initialized.");
	LOG("Spring %s", SpringVersion::GetFull().c_str());
	LOG("Build date/time: %s", SpringVersion::GetBuildTime().c_str());
	LOG("Build environment: %s", SpringVersion::GetBuildEnvironment().c_str());
	LOG("Compiler: %s", SpringVersion::GetCompiler().c_str());
}

void CLogOutput::InitializeSections()
{
	// the new systems (ILog.h) log-sub-systems are called sections:
	const std::set<const char*> sections = log_filter_section_getRegisteredSet();

	{
		std::stringstream logSectionsStr;
		logSectionsStr << "Available log sections: ";
		int numSec = 0;
		std::set<const char*>::const_iterator si;
		for (si = sections.begin(); si != sections.end(); ++si) {
			if (numSec > 0) {
				logSectionsStr << ", ";
			}
			logSectionsStr << *si;
			numSec++;
		}
		LOG("%s", logSectionsStr.str().c_str());
	}

	// enabled sections is a superset of the ones specified in the environment
	// and the ones specified in the configuration file.
	// configHandler cannot be accessed here in unitsync, as it may not exist.
	std::string enabledSections = ",";
#if defined(UNITSYNC)
	#if defined(DEBUG)
	// unitsync logging in debug mode always on
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
	enabledSections += StringToLower(configHandler->GetString("LogSections")) + ",";
#endif

	const char* const envSec = getenv("SPRING_LOG_SECTIONS");
	std::string env;
	if (envSec != NULL) {
		env += ",";
		env += envSec;
	}

	if (!env.empty()) {
		// this allows to disable all sections from the env var
		std::string envSections(StringToLower(env));
		if (envSections == std::string("none")) {
			enabledSections = "";
		} else {
			enabledSections += envSections + ",";
		}
	}
	const std::string enabledSectionsLC = StringToLower(enabledSections);

	{
		std::stringstream enabledLogSectionsStr;
		enabledLogSectionsStr << "Enabled log sections: ";
		int numSec = 0;

		// new log sections
		std::set<const char*>::const_iterator si;
		for (si = sections.begin(); si != sections.end(); ++si) {
			const std::string name = StringToLower(*si);
			const bool found = (enabledSectionsLC.find("," + name + ",") != std::string::npos);

			if (found) {
				if (numSec > 0) {
					enabledLogSectionsStr << ", ";
				}
#if       defined(DEBUG)
				log_filter_section_setMinLevel(*si, LOG_LEVEL_DEBUG);
				enabledLogSectionsStr << *si << "(LOG_LEVEL_DEBUG)";
#else  // defined(DEBUG)
				log_filter_section_setMinLevel(*si, LOG_LEVEL_INFO);
				enabledLogSectionsStr << *si << "(LOG_LEVEL_INFO)";
#endif // defined(DEBUG)
				numSec++;
			}
		}
		LOG("%s", enabledLogSectionsStr.str().c_str());
	}

	LOG("Enable or disable log sections using the LogSections configuration key");
	LOG("  or the SPRING_LOG_SECTIONS environment variable (both comma separated).");
	LOG("  Use \"none\" to disable the default log sections.");
}

