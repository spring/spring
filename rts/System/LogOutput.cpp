
#include "StdAfx.h"
#include "LogOutput.h"
#include <fstream>
#include <cstdarg>
#include <boost/thread/recursive_mutex.hpp>


#ifdef _MSC_VER
#include <windows.h>
#endif

static std::ofstream* filelog = 0;
static bool initialized = false;
CLogOutput logOutput;
static boost::recursive_mutex tempstrMutex;
static std::string tempstr;

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

void CLogOutput::Output(int priority, const char *str)
{
	if (!initialized) {
		filelog = new std::ofstream("infolog.txt");
		initialized = true;
	}

	// Output to subscribers
	for(std::vector<ILogSubscriber*>::iterator lsi=subscribers.begin();lsi!=subscribers.end();++lsi)
		(*lsi)->NotifyLogMsg(priority, str);

	int nl = strlen(str) - 1;

#ifdef _MSC_VER
	OutputDebugString(str);
	if (nl < 0 || str[nl] != '\n')
		OutputDebugString("\n");
#endif

	if (filelog) {
		(*filelog) << str;
		if (nl < 0 || str[nl] != '\n')
			(*filelog) << "\n";
		filelog->flush();
	}
}

void CLogOutput::SetLastMsgPos(float3 pos)
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

void CLogOutput::Print(int priority, const char *fmt, ...)
{
	char text[500];
	va_list	ap;

	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);

	Output (priority,text);
}

void CLogOutput::Print(const char *fmt, ...)
{
	char text[1500];
	va_list	ap;	

	va_start(ap, fmt);
	VSNPRINTF(text, sizeof(text), fmt, ap);
	va_end(ap);

	Output(0, text);
}

void CLogOutput::Print(const std::string& text)
{
	Output(0, text.c_str());
}


void CLogOutput::Print(int priority, const std::string& text)
{
	Output(priority, text.c_str());
}


CLogOutput& CLogOutput::operator<< (int i)
{
	char t[50];
	sprintf(t,"%d ",i);
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);
	tempstr += t;
	return *this;
}


CLogOutput& CLogOutput::operator<< (float f)
{
	char t[50];
	sprintf(t,"%f ",f);
	boost::recursive_mutex::scoped_lock scoped_lock(tempstrMutex);
	tempstr += t;
	return *this;
}

CLogOutput& CLogOutput::operator<< (const char* c)
{
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
