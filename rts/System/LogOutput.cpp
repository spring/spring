#include "StdAfx.h"
#include "Rendering/GL/myGL.h"

#include "LogOutput.h"

#include <assert.h>
#include <fstream>
#include <string.h>
#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "Util.h"
#include "float3.h"
#include "Sim/Misc/GlobalSynced.h"
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
		PreInitLogEntry(CLogSubsystem* subsystem, string text)
			: subsystem(subsystem), text(text) {}

		CLogSubsystem* subsystem;
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
static bool stdoutDebug = false;
static boost::recursive_mutex tempstrMutex;
static string tempstr;

static const int BUFFER_SIZE = 2048;


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
	GML_STDMUTEX_LOCK(log);

	SafeDelete(filelog);
}


void CLogOutput::SetMirrorToStdout(bool value)
{
	GML_STDMUTEX_LOCK(log);

	stdoutDebug = value;
}


void CLogOutput::SetFilename(const char* fname)
{
	GML_STDMUTEX_LOCK(log);

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
//	GML_STDMUTEX_LOCK(log);

	if (initialized) return;

	filelog = new std::ofstream(filename);
	if (filelog->bad())
		SafeDelete(filelog);

	initialized = true;
	(*this) << "LogOutput initialized.\n";

	InitializeSubsystems();

	for (vector<PreInitLogEntry>::iterator it = preInitLog().begin(); it != preInitLog().end(); ++it)
		Output(*it->subsystem, it->text.c_str());
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
	(*this) << "Available log subsystems: ";
	for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
		if (sys->name && *sys->name) {
			(*this) << sys->name;
			if (sys->next)
				(*this) << ", ";
		}
	}
	(*this) << "\n";

	// enabled subsystems is superset of the ones specified in environment
	// and the ones specified in the configuration file.
	// configHandler cannot be accessed here in unitsync since it may not exist.
#ifndef UNITSYNC
	string subsystems = "," + StringToLower(configHandler.GetString("LogSubsystems", "")) + ",";
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


	(*this) << "Enabled log subsystems: ";
	for (CLogSubsystem* sys = CLogSubsystem::GetList(); sys; sys = sys->next) {
		if (sys->name && *sys->name) {
			const string name = StringToLower(sys->name);
			const string::size_type index = subsystems.find("," + name + ",");

			// log subsystems which are enabled by default can not be disabled
			// ("enabled by default" wouldn't make sense otherwise...)
			if (!sys->enabled && index != string::npos)
				sys->enabled = true;

			if (sys->enabled) {
				(*this) << sys->name;
				if (sys->next)
					(*this) << ", ";
			}
		}
	}
	(*this) << "\n";

	(*this) << "Enable or disable log subsystems using the LogSubsystems configuration key\n";
	(*this) << "  or the SPRING_LOG_SUBSYSTEMS environment variable (both comma separated).\n";
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
void CLogOutput::Output(CLogSubsystem& subsystem, const char* str)
{
	GML_STDMUTEX_LOCK(log);

	if (!initialized) {
		preInitLog().push_back(PreInitLogEntry(&subsystem, str));
		return;
	}

	if (!subsystem.enabled) return;

	// Output to subscribers
	for(vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi)
		(*lsi)->NotifyLogMsg(subsystem, str);

	int index = strlen(str) - 1;
	bool newline = ((index < 0) || (str[index] != '\n'));

#ifdef _MSC_VER
	OutputDebugString(str);
	if (newline)
		OutputDebugString("\n");
#endif	// _MSC_VER

	if (filelog) {
#if !defined UNITSYNC && !defined DEDICATED
		if (gs) {
			(*filelog) << IntToString(gs->frameNum, "[%7d] ");
		}
#endif
		if (subsystem.name && *subsystem.name)
			(*filelog) << subsystem.name << ": ";
		(*filelog) << str;
		if (newline)
			(*filelog) << "\n";
		filelog->flush();
	}

	if (stdoutDebug) {
		if (subsystem.name && *subsystem.name) {
			fputs(subsystem.name, stdout);
			fputs(": ", stdout);
		}
		fputs(str, stdout);
		if (newline)
			putchar('\n');
		fflush(stdout);
	}
}


void CLogOutput::SetLastMsgPos(const float3& pos)
{
	GML_STDMUTEX_LOCK(log);

	for(vector<ILogSubscriber*>::iterator lsi = subscribers.begin(); lsi != subscribers.end(); ++lsi)
		(*lsi)->SetLastMsgPos(pos);
}


void CLogOutput::AddSubscriber(ILogSubscriber* ls)
{
	GML_STDMUTEX_LOCK(log);

	subscribers.push_back(ls);
}


void CLogOutput::RemoveAllSubscribers()
{
	GML_STDMUTEX_LOCK(log);

	subscribers.clear();
}


void CLogOutput::RemoveSubscriber(ILogSubscriber *ls)
{
	GML_STDMUTEX_LOCK(log);

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
	Output(subsystem, text);
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
	Output(LOG_DEFAULT, text.c_str());
}


void CLogOutput::Print(CLogSubsystem& subsystem, const std::string& text)
{
	Output(subsystem, text.c_str());
}

CLogSubsystem& CLogOutput::GetDefaultLogSubsystem()
{
	return LOG_DEFAULT;
}


CLogOutput& CLogOutput::operator<< (const int i)
{
	char t[50];
	sprintf(t, "%d ", i);
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);
	tempstr += t;
	return *this;
}


CLogOutput& CLogOutput::operator<< (const float f)
{
	char t[50];
	sprintf(t, "%f ", f);
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);
	tempstr += t;
	return *this;
}

CLogOutput& CLogOutput::operator<< (const float3& f)
{
	return *this << f.x << " " << f.y << " " << f.z;
}

CLogOutput& CLogOutput::operator<< (const char* c)
{
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);

	for(int a = 0; c[a]; ++a) {
		if (c[a] == '\n') {
			Output(LOG_DEFAULT, tempstr.c_str());
			tempstr.clear();
			break;
		} else {
			tempstr += c[a];
		}
	}
	return *this;
}

CLogOutput& CLogOutput::operator<< (const std::string& s)
{
	return this->operator<< (s.c_str());
}
