/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"

#include <cstring>
#include <boost/unordered_map.hpp>

#include "lib/gml/gmlmut.h"
#include "System/Log/ILog.h"
#include "System/UnsyncedRNG.h"


static std::map<int, int> refs;
CTimeProfiler profiler;

static unsigned hash_(const std::string& s)
{
	unsigned hash = s.size();
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
		hash += *it;
	}
	return hash;
}


ScopedTimer::ScopedTimer(const std::string& name, bool autoShow)
	: BasicTimer(name)
	, autoShowGraph(autoShow)
	, hash(hash_(name))
{
	++refs[hash];
}


ScopedTimer::~ScopedTimer()
{
	int& ref = refs[hash];
	if (--ref == 0)
		profiler.AddTime(name, spring_difftime(spring_gettime(), starttime), autoShowGraph);
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG("%s: %i ms", name.c_str(), spring_diffmsecs(spring_gettime(), starttime));
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimeProfiler::CTimeProfiler()
{
	currentPosition = 0;
	lastBigUpdate = spring_gettime();
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
		pi->second.frames[currentPosition] = spring_notime;
	}

	const spring_time curTime = spring_gettime();
	const float timeDiff = spring_diffmsecs(curTime, lastBigUpdate);
	if (timeDiff > 500.0f) // twice every second
	{
		for (std::map<std::string,TimeRecord>::iterator pi = profile.begin(); pi != profile.end(); ++pi)
		{
			pi->second.percent = spring_tomsecs(pi->second.current) / timeDiff;
			pi->second.current = spring_notime;

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

void CTimeProfiler::AddTime(const std::string& name, spring_time time, bool showGraph)
{
	GML_STDMUTEX_LOCK_NOPROF(time); // AddTime

	std::map<std::string, TimeRecord>::iterator pi;
	if ( (pi = profile.find(name)) != profile.end() ) {
		// profile already exists
		pi->second.total   += time;
		pi->second.current += time;
		pi->second.frames[currentPosition] += time;
	} else {
		// create a new profile
		profile[name].total   = time;
		profile[name].current = time;
		profile[name].percent = 0;
		memset(profile[name].frames, 0, TimeRecord::frames_size*sizeof(unsigned));
		static UnsyncedRNG rand;
		rand.Seed(spring_tomsecs(spring_gettime()));
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
				spring_tomsecs(pi->second.total) * 1000,
				pi->second.percent * 100);
	}
}
