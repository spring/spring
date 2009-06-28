#include "StdAfx.h"

#include "LogOutput.h"

#include <assert.h>
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
#include "mmgr.h"

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

static vector<ILogSubscriber*> subscribers;
static const char* filename = "infolog.txt";
static std::ofstream* filelog = 0;
static bool initialized = false;
static boost::recursive_mutex tempstrMutex;
static string tempstr;

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
{
	// multiple infologs can't exist together!
	assert(this == &logOutput);
	assert(!filelog);
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


void CLogOutput::SetFilename(const char* fname)
{
	GML_STDMUTEX_LOCK(log); // SetFilename

	assert(!initialized);
	filename = fname;
}


/**
 * @brief initialize logOutput
 *
 * Only after calling this method, logOutput starts writing to disk.
 * The log file is written in the current directory so this may only be called
 * after the engine chdir'ed to the correct directory.
 */
void CLogOutput::Initialize()
{
	if (initialized) return;

	filelog = new std::ofstream(filename);
	if (filelog->bad())
		SafeDelete(filelog);

	initialized = true;
	Print("LogOutput initialized.\n");
	Print("Spring %s", SpringVersion::GetFull().c_str());

	InitializeSubsystems();

	for (vector<PreInitLogEntry>::iterator it = preInitLog().begin(); it != preInitLog().end(); ++it)
	{
		if (!it->subsystem->enabled) return;

		// Output to subscribers
		for(vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi)
			(*lsi)->NotifyLogMsg(*(it->subsystem), it->text.c_str());
		if (filelog)
			ToFile(*it->subsystem, it->text);
	}
	preInitLog().clear();
}


/**
 * @brief initialize the log subsystems
 *
 * This writes list of all available and all enabled subsystems to the log.
 *
 * Log subsystems can be enabled using the configuration key "LogSubsystems",
 * or the environment variable "SPRING_LOG_SUBSYSTEMS".
 *
 * Both specify a comma separated list of subsystems that should be enabled.
 * The lists from both sources are combined, there is no overriding.
 *
 * A subsystem that is by default enabled, can not be disabled.
 */
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
	if (env)
		subsystems += StringToLower(env) + ",";


	{
		LogObject lo;
		lo << "Enabled log subsystems: ";
		for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
			if (sys->name && *sys->name) {
				const string name = StringToLower(sys->name);
				const string::size_type index = subsystems.find("," + name + ",");
	
				// log subsystems which are enabled by default can not be disabled
				// ("enabled by default" wouldn't make sense otherwise...)
				if (!sys->enabled && index != string::npos)
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
}


/**
 * @brief core log output method, used by all others
 *
 * Note that, when logOutput isn't initialized yet, the logging is done to the
 * global std::vector preInitLog(), and is only written to disk in the call to
 * Initialize().
 *
 * This method notifies all registered ILogSubscribers, calls OutputDebugString
 * (for MSVC builds) and prints the message to stdout and the file log.
 */
void CLogOutput::Output(const CLogSubsystem& subsystem, const std::string& str)
{
	GML_STDMUTEX_LOCK(log); // Output

	if (!initialized) {
		ToStdout(subsystem, str);
		preInitLog().push_back(PreInitLogEntry(&subsystem, str));
		return;
	}

	if (!subsystem.enabled) return;

	// Output to subscribers
	for(vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi)
		(*lsi)->NotifyLogMsg(subsystem, str.c_str());

#ifdef _MSC_VER
	int index = strlen(str.c_str()) - 1;
	bool newline = ((index < 0) || (str[index] != '\n'));
	OutputDebugString(str.c_str());
	if (newline)
		OutputDebugString("\n");
#endif // _MSC_VER

	
	if (filelog) {
		ToFile(subsystem, str);
	}

	ToStdout(subsystem, str);
}


void CLogOutput::SetLastMsgPos(const float3& pos)
{
	GML_STDMUTEX_LOCK(log); // SetLastMsgPos

	for(vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi)
		(*lsi)->SetLastMsgPos(pos);
}


void CLogOutput::AddSubscriber(ILogSubscriber* ls)
{
	GML_STDMUTEX_LOCK(log); // AddSubscriber

	subscribers.push_back(ls);
}


void CLogOutput::RemoveAllSubscribers()
{
	GML_STDMUTEX_LOCK(log); // RemoveAllSubscribers

	subscribers.clear();
}


void CLogOutput::RemoveSubscriber(ILogSubscriber *ls)
{
	GML_STDMUTEX_LOCK(log); // RemoveSubscriber

	subscribers.erase(std::find(subscribers.begin(), subscribers.end(), ls));
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

void CLogOutput::ToStdout(const CLogSubsystem& subsystem, const std::string message)
{
	if (message.empty())
		return;
	const bool newline = (message.at(message.size() -1) != '\n');
	
	if (subsystem.name && *subsystem.name) {
		std::cout << subsystem.name << ": ";
	}
	std::cout << message;
	if (newline)
		std::cout << std::endl;
	else
		std::cout.flush();
}

void CLogOutput::ToFile(const CLogSubsystem& subsystem, const std::string message)
{
	if (message.empty())
		return;
	const bool newline = (message.at(message.size() -1) != '\n');
	
#if !defined UNITSYNC && !defined DEDICATED
	if (gs) {
		(*filelog) << IntToString(gs->frameNum, "[%7d] ");
	}
#endif
	if (subsystem.name && *subsystem.name)
		(*filelog) << subsystem.name << ": ";
	(*filelog) << message;
	if (newline)
		(*filelog) << std::endl;
	else
		filelog->flush();
}
