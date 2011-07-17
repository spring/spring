/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "SyncTracer.h"
#include "Game/GlobalUnsynced.h"
#include "System/Log/ILog.h"

#include <cstdio>
#include <fstream>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSyncTracer tracefile;

static const int MAX_ACTIVE = 10;

bool CSyncTracer::init()
{
#ifdef TRACE_SYNC
	if (logfile == NULL) {
		char newLogFileName[64];
		if (gu) {
			sprintf(newLogFileName, "trace%i.log", gu->myTeam);
		} else {
			sprintf(newLogFileName, "trace_early.log");
		}
		logfile = new std::ofstream(newLogFileName);
		LOG("Sync trace log: %s", newLogFileName);
	}
#endif
	return logfile != 0;
}

CSyncTracer::CSyncTracer()
	: file(NULL)
	, logfile(NULL)
	, firstActive(0)
	, nowActive(0)
{
}

CSyncTracer::~CSyncTracer()
{
#ifdef TRACE_SYNC
	delete file;
	delete logfile;
#endif
}

void CSyncTracer::Commit()
{
#ifdef TRACE_SYNC
	if (file == NULL) {
		char newFileName[64];
		if (gu)
			sprintf(newFileName, "trace%i.log", gu->myTeam);
		else
			sprintf(newFileName, "trace_early.log");
		file = new std::ofstream(newFileName);
	}
#endif

	if (file == NULL)
		return;

	(*file) << traces[firstActive].c_str();
	while (nowActive != firstActive) {
		firstActive++;
		if (firstActive == MAX_ACTIVE)
			firstActive = 0;
		(*file) << traces[firstActive].c_str();
	}
	traces[nowActive] = "";
}

void CSyncTracer::NewInterval()
{
	nowActive++;
	if (nowActive == MAX_ACTIVE)
		nowActive = 0;
	traces[nowActive] = "";
}

void CSyncTracer::DeleteInterval()
{
	if (firstActive != nowActive)
		firstActive++;
	if (firstActive == MAX_ACTIVE)
		firstActive = 0;
}

CSyncTracer& CSyncTracer::operator<<(const std::string& value)
{
	traces[nowActive] += value;
	if (init()) (*logfile) << value;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const char* value)
{
	traces[nowActive] += value;
	if (init()) (*logfile) << value;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const int value)
{
	char trace[20];
	sprintf(trace, "%d", value);
	traces[nowActive] += trace;
	if (init()) (*logfile) << value;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const unsigned value)
{
	char trace[20];
	sprintf(trace, "%d", value);
	traces[nowActive] += trace;
	if (init()) (*logfile) << value;
	return *this;
}

CSyncTracer& CSyncTracer::operator<<(const float value)
{
	char trace[50];
	sprintf(trace, "%f", value);
	traces[nowActive] += trace;
	if (init()) (*logfile) << value;
	return *this;
}
