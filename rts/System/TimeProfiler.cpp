/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"

#include <SDL_timer.h>
#include <cstring>

#include "lib/gml/gmlmut.h"
#include "System/Log/ILog.h"
#include "System/UnsyncedRNG.h"

static std::map<const std::string, int> refs;

BasicTimer::BasicTimer(const char* const myname) : name(myname), starttime(SDL_GetTicks())
{
	++refs[name];
}



ScopedTimer::~ScopedTimer()
{
	int& ref = refs[name];
	if (--ref == 0)
		profiler.AddTime(name, SDL_GetTicks() - starttime, autoShowGraph);
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG("%s: %i ms", name.c_str(), (SDL_GetTicks() - starttime));
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
	currentPosition &= TimeRecord::frames_size-1;
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

			if(pi->second.percent > pi->second.peak) {
				pi->second.peak = pi->second.percent;
				pi->second.newpeak = true;
			}
			else
				pi->second.newpeak = false;
		}
		lastBigUpdate = curTime;
	}
}

float CTimeProfiler::GetPercent(const char* name)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // GetTimePercent

	return profile[name].percent;
}

void CTimeProfiler::AddTime(const std::string& name, unsigned time, bool showGraph)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // AddTime

	std::map<std::string, TimeRecord>::iterator pi;
	if ( (pi = profile.find(name)) != profile.end() ) {
		// profile already exists
		pi->second.total+=time;
		pi->second.current+=time;
		pi->second.frames[currentPosition]+=time;
	} else {
		// create a new profile
		profile[name].total=time;
		profile[name].current=time;
		profile[name].percent=0;
		memset(profile[name].frames, 0, TimeRecord::frames_size*sizeof(unsigned));
		static UnsyncedRNG rand;
		rand.Seed(SDL_GetTicks());
		profile[name].color.x = rand.RandFloat();
		profile[name].color.y = rand.RandFloat();
		profile[name].color.z = rand.RandFloat();
		profile[name].showGraph = showGraph;
	}
}

void CTimeProfiler::PrintProfilingInfo() const
{
	LOG("%35s|%18s|%s",
			"Part",
			"Total Time",
			"Time of the last 0.5s");
	std::map<std::string, CTimeProfiler::TimeRecord>::const_iterator pi;
	for (pi = profile.begin(); pi != profile.end(); ++pi) {
		LOG("%35s %16.2fs %5.2f%%",
				pi->first.c_str(),
				((float)pi->second.total) / 1000.f,
				pi->second.percent * 100);
	}
}
