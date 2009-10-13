// TimeProfiler.cpp: implementation of the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////
#include "TimeProfiler.h"

#include <SDL_timer.h>
#include <cstring>

#include "mmgr.h"
#include "lib/gml/gml.h"
#include "LogOutput.h"
#include "UnsyncedRNG.h"


BasicTimer::BasicTimer(const char* const myname) : name(myname), starttime(SDL_GetTicks())
{
}

ScopedTimer::ScopedTimer(const char* const myname) : BasicTimer(myname)
{
}

ScopedTimer::~ScopedTimer()
{
	const unsigned stoptime = SDL_GetTicks();
	profiler.AddTime(name, stoptime - starttime);
}

ScopedOnceTimer::ScopedOnceTimer(const char* const myname) : BasicTimer(myname)
{
}

ScopedOnceTimer::ScopedOnceTimer(const std::string& myname): BasicTimer(myname.c_str())
{
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	const unsigned stoptime = SDL_GetTicks();
	LogObject() << name << ": " << stoptime - starttime << " ms";
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTimeProfiler profiler;

CTimeProfiler::CTimeProfiler()
{
	currentPosition = 0;
	lastBigUpdate = SDL_GetTicks();
}

CTimeProfiler::~CTimeProfiler()
{
}

void CTimeProfiler::Update()
{
	GML_STDMUTEX_LOCK_NOPROF(time); // Update

	++currentPosition;
	currentPosition &= 127;
	std::map<std::string,TimeRecord>::iterator pi;
	for (pi = profile.begin(); pi != profile.end(); ++pi)
	{
		pi->second.frames[currentPosition]=0;
	}

	const unsigned curTime = SDL_GetTicks();
	const unsigned timeDiff = curTime - lastBigUpdate;
	if (timeDiff > 500) // twice every second
	{
		for (std::map<std::string,TimeRecord>::iterator pi = profile.begin(); pi != profile.end(); ++pi)
		{
			pi->second.percent = ((float)pi->second.current) / ((float)timeDiff);
			pi->second.current=0;

		}
		lastBigUpdate = curTime;
	}
}

float CTimeProfiler::GetPercent(const char *name)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // GetTimePercent

	return profile[name].percent;
}

void CTimeProfiler::AddTime(const std::string& name, unsigned time)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // AddTime

	std::map<std::string, TimeRecord>::iterator pi;
	if ( (pi = profile.find(name)) != profile.end() )
	{
		pi->second.total+=time;
		pi->second.current+=time;
		pi->second.frames[currentPosition]+=time;
	}
	else
	{
		profile[name].total=time;
		profile[name].current=time;
		profile[name].percent=0;
		memset(profile[name].frames, 0, 128*sizeof(unsigned));
		static UnsyncedRNG rand;
		rand.Seed(SDL_GetTicks());
		profile[name].color.x = rand.RandFloat();
		profile[name].color.y = rand.RandFloat();
		profile[name].color.z = rand.RandFloat();
		profile[name].showGraph=true;
	}
}
