/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"

#include "System/LogOutput.h"

#include "lib/gml/gmlmut.h"
#include "System/Util.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/ILog.h"
#include "System/Log/Level.h"
#include "System/mmgr.h"

#include <string>
#include <set>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include <cassert>
#include <cstring>

#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif

/******************************************************************************/
/******************************************************************************/

CONFIG(std::string, RotateLogFiles).defaultValue("auto");
CONFIG(std::string, LogSections).defaultValue("");
CONFIG(std::string, LogSubsystems).defaultValue(""); // XXX deprecated on 22. August 2011, before the 0.83 release

/******************************************************************************/
/******************************************************************************/

CLogOutput logOutput;

// wrapped in a function to prevent order of initialization problems
// when logOutput is used before main() is entered.
static std::vector<std::string>& preInitLog()
{
	static std::vector<std::string> preInitLog;
	return preInitLog;
}

static std::ofstream* filelog = NULL;
static bool initialized = false;

static const int BUFFER_SIZE = 2048;

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
}

void CLogOutput::Flush()
{
	GML_STDMUTEX_LOCK_NOPROF(log); // Flush

	if (filelog != NULL) {
		filelog->flush();
	}
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

	filelog = new std::ofstream(filePath.c_str());
	if (filelog->bad())
		SafeDelete(filelog);

	initialized = true;
	InitializeSections();

	std::vector<std::string>::iterator pili;
	for (pili = preInitLog().begin(); pili != preInitLog().end(); ++pili) {
		ToFile(*pili);
	}
	preInitLog().clear();

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
#ifndef UNITSYNC
	#ifndef DEBUG
	// Always show at least INFO level of these sections
	enabledSections += "Sound,";
	#endif
	enabledSections += StringToLower(configHandler->GetString("LogSections")) + ",";
	enabledSections += StringToLower(configHandler->GetString("LogSubsystems")) + ","; // XXX deprecated on 22. August 2011, before the 0.83 release
#else
	#ifdef DEBUG
	// unitsync logging in debug mode always on
	enabledSections += "unitsync,ArchiveScanner,";
	#endif
#endif

	const char* const envSec = getenv("SPRING_LOG_SECTIONS");
	const char* const envSubsys = getenv("SPRING_LOG_SUBSYSTEMS"); // XXX deprecated on 22. August 2011, before the 0.83 release
	std::string env;
	if (envSec != NULL) {
		env += ",";
		env += envSec;
	}
	if (envSubsys != NULL) {
		env += ",";
		env += envSubsys;
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


void CLogOutput::Output(const std::string& str)
{
	GML_STDMUTEX_LOCK(log); // Output

	std::string msg;

#if !defined UNITSYNC && !defined DEDICATED
	if (gs) {
		msg += IntToString(gs->frameNum, "[f=%07d] ");
	}
#endif

	msg += str;

	if (!initialized) {
		ToStderr(msg);
		preInitLog().push_back(msg);
		return;
	}

#ifdef _MSC_VER
	int index = strlen(str.c_str()) - 1;
	bool newline = ((index < 0) || (str[index] != '\n'));
	OutputDebugString(msg.c_str());
	if (newline) {
		OutputDebugString("\n");
	}
#endif // _MSC_VER

	ToFile(msg);
	ToStderr(msg);
}



// ----------------------------------------------------------------------
// Output functions
// ----------------------------------------------------------------------

void CLogOutput::ToStderr(const std::string& message)
{
	if (message.empty()) {
		return;
	}

	const bool newline = (message.at(message.size() -1) != '\n');

	std::cerr << message;
	if (newline)
		std::cerr << std::endl;
#ifdef DEBUG
	// flushing may be bad for in particular dedicated server performance
	// crash handler should cleanly close the log file usually anyway
	else
		std::cerr.flush();
#endif
}

void CLogOutput::ToFile(const std::string& message)
{
	if (message.empty() || (filelog == NULL)) {
		return;
	}

	const bool newline = (message.at(message.size() -1) != '\n');

	(*filelog) << message;
	if (newline)
		(*filelog) << std::endl;
#ifdef DEBUG
	// flushing may be bad for in particular dedicated server performance
	// crash handler should cleanly close the log file usually anyway
	else
		filelog->flush();
#endif
}

