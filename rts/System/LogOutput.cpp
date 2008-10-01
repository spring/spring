
#include "StdAfx.h"
#include <assert.h>
#include "LogOutput.h"
#if !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
#include "GlobalStuff.h"
#endif	/* !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */
#include <cstdarg>
#include <fstream>
#include <string.h>
#include <boost/thread/recursive_mutex.hpp>


#ifdef _MSC_VER
#include <windows.h>
#endif

//#if defined BUILDING_AI
//static std::ofstream* ai_filelog = 0;
/*
#if defined BUILDING_AI_INTERFACE
#define LOG_FILE_NAME "CAIInterface_infolog.txt"
static std::ofstream* aiinterface_filelog = 0;
static bool aiinterface_initialized = false;
#define FILE_LOG aiinterface_filelog
#define INITIALIZED aiinterface_initialized
#else
*/
//#define LOG_FILE_NAME "infolog.txt"
#if defined BUILDING_AI_INTERFACE
#define LOG_FILE_PREFIX "infolog_aiinterface"
#define LOG_FILE_PREFIX "infolog"
#else
#endif
#define LOG_FILE_SUFFIX ".txt"
//static std::ofstream* filelog = 0;
//static bool initialized = false;
static int infologIndex = 0;
#define FILE_LOG filelog
//#define INITIALIZED initialized
//#endif
static bool stdoutDebug = false;
//CLogOutput logOutput;
CLogOutput& logOutput = CLogOutput::GetInstance();
static boost::recursive_mutex tempstrMutex;
static std::string tempstr;

static const int bufferSize = 2048;

CLogOutput CLogOutput::myLogOutput = CLogOutput();
//std::ofstream* CLogOutput::filelog = NULL;

CLogOutput& CLogOutput::GetInstance() {
	return myLogOutput;
}

CLogOutput::CLogOutput()
{
	assert(this == &logOutput);
	FILE_LOG = NULL;
	//assert(!(FILE_LOG)); // multiple infologs can't exist together!
}

CLogOutput::~CLogOutput()
{
	End();
}

void CLogOutput::End()
{
	delete FILE_LOG;
	FILE_LOG = 0;
}

void CLogOutput::SetMirrorToStdout(bool value)
{
	stdoutDebug = value;
}

void CLogOutput::Output(int zone, const char *str)
{
/*
	if (!INITIALIZED) {
		FILE_LOG = new std::ofstream(LOG_FILE_NAME);
		INITIALIZED = true;
	}
*/
	#if !defined BUILDING_AI_INTERFACE
	if (FILE_LOG == NULL) {
		#if defined BUILDING_AI_INTERFACE
		const int MAX_STR_LENGTH = 511;
		char logFileName[MAX_STR_LENGTH + 1];
		SNPRINTF(logFileName, MAX_STR_LENGTH, "infolog_aiinterface_%d.txt", infologIndex++);
		#else
		const char* logFileName = "infolog.txt";
		#endif
		FILE_LOG = new std::ofstream(logFileName);
	}

	// Output to subscribers
#if !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	for(std::vector<ILogSubscriber*>::iterator lsi=subscribers.begin(); lsi!=subscribers.end();++lsi)
		(*lsi)->NotifyLogMsg(zone, str);
#endif	/* !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */

	int nl = strlen(str) - 1;

#ifdef _MSC_VER
	OutputDebugString(str);
	if (nl < 0 || str[nl] != '\n')
		OutputDebugString("\n");
#endif

	if (FILE_LOG) {
#if !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
		if (gs) {
			(*FILE_LOG) << IntToString(gs->frameNum, "[%7d] ");
		}
#endif	/* !defined DEDICATED && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE */
		(*FILE_LOG) << str;
		if (nl < 0 || str[nl] != '\n')
			(*FILE_LOG) << "\n";
		FILE_LOG->flush();
	}

	if (stdoutDebug) {
		fputs(str, stdout);
		if (nl < 0 || str[nl] != '\n') {
			putchar('\n');
		}
		fflush(stdout);
	}
	#endif	/* !defined BUILDING_AI_INTERFACE */
}

void CLogOutput::SetLastMsgPos(const float3& pos)
{
	for(std::vector<ILogSubscriber*>::iterator lsi=subscribers.begin();lsi!=subscribers.end();++lsi)
		(*lsi)->SetLastMsgPos(pos);
}

void CLogOutput::AddSubscriber(ILogSubscriber* ls)
{
	subscribers.push_back(ls);
}

void CLogOutput::RemoveAllSubscribers()
{
	subscribers.clear();
}

void CLogOutput::RemoveSubscriber(ILogSubscriber *ls)
{
	subscribers.erase(std::find(subscribers.begin(),subscribers.end(),ls));
}

// ----------------------------------------------------------------------
// Printing functions
// ----------------------------------------------------------------------

void CLogOutput::Print(int zone, const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	Printv(zone, fmt, argp);
	va_end(argp);
}

void CLogOutput::Printv(int zone, const char *fmt, va_list argp)
{
	char text[bufferSize];

	VSNPRINTF(text, sizeof(text), fmt, argp);
	Output(zone, text);
}

void CLogOutput::Print(const char *fmt, ...)
{
	va_list argp;

	va_start(argp, fmt);
	Printv(0, fmt, argp);
	va_end(argp);
}

void CLogOutput::Print(const std::string& text)
{
	Output(0, text.c_str());
}


void CLogOutput::Print(int zone, const std::string& text)
{
	Output(zone, text.c_str());
}


CLogOutput& CLogOutput::operator<< (const int i)
{
	char t[50];
	sprintf(t,"%d ",i);
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);
	tempstr += t;
	return *this;
}


CLogOutput& CLogOutput::operator<< (const float f)
{
	char t[50];
	sprintf(t,"%f ",f);
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

	for(int a=0;c[a];a++) {
		if (c[a] == '\n') {
			Output(0, tempstr.c_str());
			tempstr.clear();
			break;
		} else
			tempstr += c[a];
	}
	return *this;
}

CLogOutput& CLogOutput::operator<< (const std::string &s)
{
	return this->operator <<(s.c_str());
}
