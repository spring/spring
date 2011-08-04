/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"

#include "System/LogOutput.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "lib/gml/gmlmut.h"
#include "System/Util.h"
#include "System/float3.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameVersion.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystemHandler.h"
#include "System/Log/DefaultFilter.h"
#include "System/Log/Level.h"
#include "System/mmgr.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

/******************************************************************************/
/******************************************************************************/

CONFIG(std::string, RotateLogFiles).defaultValue("auto");
CONFIG(std::string, LogSubsystems).defaultValue("");

CLogSubsystem* CLogSubsystem::linkedList;
static CLogSubsystem LOG_DEFAULT("", true);


CLogSubsystem::CLogSubsystem(const char* name, bool enabled)
: name(name), next(linkedList), enabled(enabled)
{
	linkedList = this;
}

/******************************************************************************/
/******************************************************************************/

CLogOutput logOutput;

namespace
{
	struct PreInitLogEntry
	{
		PreInitLogEntry(const CLogSubsystem* subsystem, string text)
			: subsystem(subsystem), text(text) {}

		const CLogSubsystem* subsystem;
		string text;
	};
}

// wrapped in a function to prevent order of initialization problems
// when logOutput is used before main() is entered.
static vector<PreInitLogEntry>& preInitLog()
{
	static vector<PreInitLogEntry> preInitLog;
	return preInitLog;
}

static std::ofstream* filelog = NULL;
static bool initialized = false;

static const int BUFFER_SIZE = 2048;


LogObject::LogObject(const CLogSubsystem& _subsys) : subsys(_subsys)
{
}

LogObject::LogObject() : subsys(LOG_DEFAULT)
{
}

LogObject::~LogObject()
{
	// LogOutput adds a newline if necessary
	logOutput.Prints(subsys, str.str());
}

CLogOutput::CLogOutput()
	: fileName("")
	, filePath("")
	, subscribersEnabled(true)
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
	return FileSystemHandler::GetCwd() + (char)FileSystemHandler::GetNativePathSeparator() + fileName;
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
		if (FileSystemHandler::FileExists(filePath)) {
			// logArchiveDir: /absolute/writeable/data/dir/log/
			std::string logArchiveDir = filePath.substr(0, filePath.find_last_of("/\\") + 1);
			logArchiveDir = logArchiveDir + "log" + (char)FileSystemHandler::GetNativePathSeparator();

			const std::string archivedLogFile = logArchiveDir + FileSystemHandler::GetFileModificationDate(filePath) + "_" + fileName;

			// create the log archive dir if it does nto exist yet
			if (!FileSystemHandler::DirExists(logArchiveDir)) {
				FileSystemHandler::mkdir(logArchiveDir);
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
	Print("LogOutput initialized.\n");
	Print("Spring %s", SpringVersion::GetFull().c_str());
	Print("Build date/time: %s", SpringVersion::GetBuildTime().c_str());
	Print("Build environment: %s", SpringVersion::GetBuildEnvironment().c_str());
	Print("Compiler: %s", SpringVersion::GetCompiler().c_str());

	InitializeSubsystems();

	for (vector<PreInitLogEntry>::iterator it = preInitLog().begin(); it != preInitLog().end(); ++it)
	{
		if (!it->subsystem->enabled) return;

		// Output to subscribers
		if (subscribersEnabled) {
			for (vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi) {
				(*lsi)->NotifyLogMsg(*(it->subsystem), it->text);
			}
		}
		ToFile(*it->subsystem, it->text);
	}
	preInitLog().clear();
}

void CLogOutput::InitializeSubsystems()
{
	// the new systems (ILog.h) log-sub-systems are called sections:
	const std::set<const char*> sections = log_filter_section_getRegisteredSet();

	{
		LogObject lo;
		lo << "Available log subsystems: ";
		int numSec = 0;
		for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
			if (sys->name && *sys->name) {
				if (numSec > 0) {
					lo << ", ";
				}
				lo << sys->name;
				numSec++;
			}
		}
		std::set<const char*>::const_iterator si;
		for (si = sections.begin(); si != sections.end(); ++si) {
			if (numSec > 0) {
				lo << ", ";
			}
			lo << *si;
			numSec++;
		}
	}

	// enabled subsystems is superset of the ones specified in the environment
	// and the ones specified in the configuration file.
	// configHandler cannot be accessed here in unitsync since it may not exist.
#ifndef UNITSYNC
	std::string subsystems = "," + StringToLower(configHandler->GetString("LogSubsystems")) + ",";
#else
	#ifdef DEBUG
	// unitsync logging in debug mode always on
	std::string subsystems = ",unitsync,ArchiveScanner";
	#else
	std::string subsystems = ",";
	#endif
#endif

	const char* const env = getenv("SPRING_LOG_SUBSYSTEMS");
	bool env_override = false;
	if (env) {
		// this allows to disable all subsystems from the env var
		std::string env_subsystems(StringToLower(env));
		if (env_subsystems == std::string("none")) {
			subsystems = "";
			env_override = true;
		} else {
			subsystems += env_subsystems + ",";
		}
	}
	const std::string subsystemsLC = StringToLower(subsystems);

	{
		LogObject lo;
		lo << "Enabled log subsystems: ";
		int numSec = 0;

		// classical log-subsystems
		for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
			if (sys->name && *sys->name) {
				const std::string name = StringToLower(sys->name);
				const bool found = (subsystemsLC.find("," + name + ",") != string::npos);

				if (env_override) {
					sys->enabled = found;
				} else if (!sys->enabled && found) {
					// log subsystems which are enabled by default can not be disabled
					// ("enabled by default" wouldn't make sense otherwise...)
					sys->enabled = true;
				}

				if (sys->enabled) {
					if (numSec > 0) {
						lo << ", ";
					}
					lo << sys->name;
					numSec++;
				}
			}
		}

		// new log sections
		std::set<const char*>::const_iterator si;
		for (si = sections.begin(); si != sections.end(); ++si) {
			const std::string name = StringToLower(*si);
			const bool found = (subsystemsLC.find("," + name + ",") != string::npos);

			if (found) {
				if (numSec > 0) {
					lo << ", ";
				}
#if       defined(DEBUG)
				log_filter_section_setMinLevel(*si, LOG_LEVEL_DEBUG);
				lo << *si << "(LOG_LEVEL_DEBUG)";
#else  // defined(DEBUG)
				log_filter_section_setMinLevel(*si, LOG_LEVEL_INFO);
				lo << *si << "(LOG_LEVEL_INFO)";
#endif // defined(DEBUG)
				numSec++;
			}
		}
	}

	Print("Enable or disable log subsystems using the LogSubsystems configuration key\n");
	Print("  or the SPRING_LOG_SUBSYSTEMS environment variable (both comma separated).\n");
	Print("  Use \"none\" to disable the default log subsystems.\n");
}


void CLogOutput::Output(const CLogSubsystem& subsystem, const std::string& str)
{
	GML_STDMUTEX_LOCK(log); // Output

	std::string msg;

#if !defined UNITSYNC && !defined DEDICATED
	if (gs) {
		msg += IntToString(gs->frameNum, "[f=%07d] ");
	}
#endif
	if (subsystem.name && *subsystem.name) {
		msg += ("[" + std::string(subsystem.name) + "] ");
	}

	msg += str;

	if (!initialized) {
		ToStderr(subsystem, msg);
		preInitLog().push_back(PreInitLogEntry(&subsystem, msg));
		return;
	}

	if (!subsystem.enabled) return;

	// Output to subscribers
	//FIXME make threadsafe!!!
	if (subscribersEnabled) {
		for (vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi) {
			(*lsi)->NotifyLogMsg(subsystem, str);
		}
	}

#ifdef _MSC_VER
	int index = strlen(str.c_str()) - 1;
	bool newline = ((index < 0) || (str[index] != '\n'));
	OutputDebugString(msg.c_str());
	if (newline) {
		OutputDebugString("\n");
	}
#endif // _MSC_VER

	ToFile(subsystem, msg);
	ToStderr(subsystem, msg);
}


void CLogOutput::SetLastMsgPos(const float3& pos)
{
	GML_STDMUTEX_LOCK(log); // SetLastMsgPos

	if (subscribersEnabled) {
		for (vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi) {
			(*lsi)->SetLastMsgPos(pos);
		}
	}
}



void CLogOutput::AddSubscriber(ILogSubscriber* ls)
{
	GML_STDMUTEX_LOCK(log); // AddSubscriber

	subscribers.push_back(ls);
}

void CLogOutput::RemoveSubscriber(ILogSubscriber *ls)
{
	GML_STDMUTEX_LOCK(log); // RemoveSubscriber

	subscribers.erase(std::find(subscribers.begin(), subscribers.end(), ls));
}



void CLogOutput::SetSubscribersEnabled(bool enabled) {
	subscribersEnabled = enabled;
}

bool CLogOutput::IsSubscribersEnabled() const {
	return subscribersEnabled;
}



// ----------------------------------------------------------------------
// Printing functions
// ----------------------------------------------------------------------

void CLogOutput::Print(const CLogSubsystem& subsystem, const char* fmt, ...)
{
	// if logOutput isn't initialized then subsystem.enabled still has it's default value
	if (initialized && !subsystem.enabled) return;

	va_list argp;

	va_start(argp, fmt);
	Printv(subsystem, fmt, argp);
	va_end(argp);
}


void CLogOutput::Printv(const CLogSubsystem& subsystem, const char* fmt, va_list argp)
{
	// if logOutput isn't initialized then subsystem.enabled still has it's default value
	if (initialized && !subsystem.enabled) return;

	char text[BUFFER_SIZE];

	VSNPRINTF(text, sizeof(text), fmt, argp);
	Output(subsystem, std::string(text));
}


void CLogOutput::Print(const char* fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	Printv(LOG_DEFAULT, fmt, argp);
	va_end(argp);
}


void CLogOutput::Print(const std::string& text)
{
	Output(LOG_DEFAULT, text);
}


void CLogOutput::Prints(const CLogSubsystem& subsystem, const std::string& text)
{
	Output(subsystem, text);
}


CLogSubsystem& CLogOutput::GetDefaultLogSubsystem()
{
	return LOG_DEFAULT;
}



void CLogOutput::ToStderr(const CLogSubsystem& subsystem, const std::string& message)
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

void CLogOutput::ToFile(const CLogSubsystem& subsystem, const std::string& message)
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

