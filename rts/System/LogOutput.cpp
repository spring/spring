/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "LogOutput.h"

#include <assert.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "lib/gml/gml.h"
#include "Util.h"
#include "float3.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameVersion.h"
#include "ConfigHandler.h"
#include "FileSystem/FileSystemHandler.h"
#include "mmgr.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

/******************************************************************************/
/******************************************************************************/

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
		rotatePolicy = configHandler->GetString("RotateLogFiles", "auto");
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
				std::cout << "Failed rotating the log file" << std::endl;
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
	logOutput.Print("Build date/time: %s", SpringVersion::BuildTime);

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
	{
		LogObject lo;
		lo << "Available log subsystems: ";
		for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
			if (sys->name && *sys->name) {
				lo << sys->name;
				if (sys->next)
					lo << ", ";
			}
		}
	}
	// enabled subsystems is superset of the ones specified in environment
	// and the ones specified in the configuration file.
	// configHandler cannot be accessed here in unitsync since it may not exist.
#ifndef UNITSYNC
	string subsystems = "," + StringToLower(configHandler->GetString("LogSubsystems", "")) + ",";
#else
#  ifdef DEBUG
	// unitsync logging in debug mode always on
	string subsystems = ",unitsync,";
#  else
	string subsystems = ",";
#  endif
#endif

	const char* const env = getenv("SPRING_LOG_SUBSYSTEMS");
	bool env_override = false;
	if (env)
	{
		// this allows to disable all subsystems from the env var
		std::string env_subsystems(StringToLower(env));
		if ( env_subsystems == std::string("none" ))
		{
			subsystems = "";
			env_override = true;
		}
		else
			subsystems += env_subsystems + ",";
	}


	{
		LogObject lo;
		lo << "Enabled log subsystems: ";
		for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
			if (sys->name && *sys->name) {
				const string name = StringToLower(sys->name);
				const string::size_type index = subsystems.find("," + name + ",");

				if (env_override)
					sys->enabled = index != string::npos;
				// log subsystems which are enabled by default can not be disabled
				// ("enabled by default" wouldn't make sense otherwise...)
				else if (!sys->enabled && index != string::npos)
					sys->enabled = true;

				if (sys->enabled) {
					lo << sys->name;
					if (sys->next)
						lo << ", ";
				}
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
		ToStdout(subsystem, msg);
		preInitLog().push_back(PreInitLogEntry(&subsystem, msg));
		return;
	}

	if (!subsystem.enabled) return;

	// Output to subscribers
	if (subscribersEnabled) {
		for (vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi) {
			(*lsi)->NotifyLogMsg(subsystem, msg);
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

	ToStdout(subsystem, msg);
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

void CLogOutput::Print(CLogSubsystem& subsystem, const char* fmt, ...)
{
	// if logOutput isn't initialized then subsystem.enabled still has it's default value
	if (initialized && !subsystem.enabled) return;

	va_list argp;

	va_start(argp, fmt);
	Printv(subsystem, fmt, argp);
	va_end(argp);
}


void CLogOutput::Printv(CLogSubsystem& subsystem, const char* fmt, va_list argp)
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



void CLogOutput::ToStdout(const CLogSubsystem& subsystem, const std::string& message)
{
	if (message.empty()) {
		return;
	}

	const bool newline = (message.at(message.size() -1) != '\n');

	std::cout << message;
	if (newline)
		std::cout << std::endl;
#ifdef DEBUG
	// flushing may be bad for in particular dedicated server performance
	// crash handler should cleanly close the log file usually anyway
	else
		std::cout.flush();
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
