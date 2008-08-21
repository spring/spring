
#include "StdAfx.h"
#include <assert.h>
#include <cstdarg>
#include <fstream>
#include <string.h>
#include <boost/thread/recursive_mutex.hpp>

#ifdef _MSC_VER
#include <windows.h>
#endif

#include "mmgr.h"

#include "LogOutput.h"

static std::ofstream* filelog = 0;
static bool initialized = false;
static bool stdoutDebug = false;
CLogOutput logOutput;
static boost::recursive_mutex tempstrMutex;
static std::string tempstr;

static const int bufferSize = 2048;

CLogOutput::CLogOutput()
{
	assert(this == &logOutput);
	assert(!filelog); // multiple infologs can't exist together!
}

CLogOutput::~CLogOutput()
{
	End();
}

void CLogOutput::End()
{
	delete filelog;
	filelog = 0;
}

void CLogOutput::SetMirrorToStdout(bool value)
{
	stdoutDebug = value;
}

void CLogOutput::Output(int zone, const char *str)
{
	if (!initialized) {
		filelog = new std::ofstream("infolog.txt");
		initialized = true;
	}

	// Output to subscribers
	for(std::vector<ILogSubscriber*>::iterator lsi=subscribers.begin();lsi!=subscribers.end();++lsi)
		(*lsi)->NotifyLogMsg(zone, str);

	int nl = strlen(str) - 1;

#ifdef _MSC_VER
	OutputDebugString(str);
	if (nl < 0 || str[nl] != '\n')
		OutputDebugString("\n");
#endif

	if (filelog) {
#ifndef DEDICATED
		if (gs) {
			(*filelog) << IntToString(gs->frameNum, "[%7d] ");
		}
#endif
		(*filelog) << str;
		if (nl < 0 || str[nl] != '\n')
			(*filelog) << "\n";
		filelog->flush();
	}

	if (stdoutDebug) {
		fputs(str, stdout);
		if (nl < 0 || str[nl] != '\n') {
			putchar('\n');
		}
		fflush(stdout);
	}
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
