/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"

#include <cstring>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

#include "lib/gml/gmlmut.h"
#include "System/Log/ILog.h"
#include "System/UnsyncedRNG.h"
#ifdef THREADPOOL
	#include "System/ThreadPool.h"
#endif

static boost::mutex m;
static std::map<int, std::string> hashToName;
static std::map<int, int> refs;



static unsigned hash_(const std::string& s)
{
	unsigned hash = s.size();
	for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
		hash += *it;
	}
	return hash;
}

static unsigned hash_(const char* s)
{
	unsigned hash = 0;
	for (size_t i = 0; ; ++i) {
		if (s[i]) {
			hash += s[i];
		} else {
			unsigned len = i;
			hash += len;
			break;
		}
	}
	return hash;
}


BasicTimer::BasicTimer(const std::string& myname)
: hash(hash_(myname))
, starttime(spring_gettime())

{
	nameIterator = hashToName.find(hash);
	if (nameIterator == hashToName.end()) {
		nameIterator = hashToName.insert(std::pair<int,std::string>(hash, myname)).first;
	}
}


BasicTimer::BasicTimer(const char* myname)
: hash(hash_(myname))
, starttime(spring_gettime())

{
	nameIterator = hashToName.find(hash);
	if (nameIterator == hashToName.end()) {
		nameIterator = hashToName.insert(std::pair<int,std::string>(hash, myname)).first;
	}
}


const std::string& BasicTimer::GetName() const
{
	return nameIterator->second;
}


ScopedTimer::ScopedTimer(const std::string& name, bool autoShow)
	: BasicTimer(name)
	, autoShowGraph(autoShow)

{
	it = refs.find(hash);
	if (it == refs.end()) {
		it = refs.insert(std::pair<int,int>(hash, 0)).first;
	}
	++(it->second);
}


ScopedTimer::ScopedTimer(const char* name, bool autoShow)
	: BasicTimer(name)
	, autoShowGraph(autoShow)

{
	it = refs.find(hash);
	if (it == refs.end()) {
		it = refs.insert(std::pair<int,int>(hash, 0)).first;
	}
	++(it->second);
}


ScopedTimer::~ScopedTimer()
{
	int& ref = it->second;
	if (--ref == 0)
		profiler.AddTime(GetName(), spring_difftime(spring_gettime(), starttime), autoShowGraph);
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG("%s: %lli ms", GetName().c_str(), spring_diffmsecs(spring_gettime(), starttime));
}



ScopedMtTimer::ScopedMtTimer(const std::string& name, bool autoShow)
	: BasicTimer(name)
	, autoShowGraph(autoShow)
{
}


ScopedMtTimer::ScopedMtTimer(const char* name, bool autoShow)
	: BasicTimer(name)
	, autoShowGraph(autoShow)
{
}


ScopedMtTimer::~ScopedMtTimer()
{
	profiler.AddTime(GetName(), spring_difftime(spring_gettime(), starttime), autoShowGraph);
#ifdef THREADPOOL
	auto& list = profiler.profileCore[ThreadPool::GetThreadNum()];
	list.emplace_back(starttime, spring_gettime());
#endif
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimeProfiler::CTimeProfiler():
	lastBigUpdate(spring_gettime()),
	currentPosition(0)
{
#ifdef THREADPOOL
	profileCore.resize(ThreadPool::GetMaxThreads());
#endif
}

CTimeProfiler::~CTimeProfiler()
{
	boost::unique_lock<boost::mutex> ulk(m, boost::defer_lock);
	while (!ulk.try_lock()) {}
}

CTimeProfiler& CTimeProfiler::GetInstance()
{
	static CTimeProfiler tp;
	return tp;
}

void CTimeProfiler::Update()
{
	//FIXME non-locking threadsafe
	boost::unique_lock<boost::mutex> ulk(m, boost::defer_lock);
	while (!ulk.try_lock()) {}

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
	boost::unique_lock<boost::mutex> ulk(m, boost::defer_lock);
	while (!ulk.try_lock()) {}

	return profile[name].percent;
}

void CTimeProfiler::AddTime(const std::string& name, const spring_time time, const bool showGraph)
{
	std::map<std::string, TimeRecord>::iterator pi;
	if ( (pi = profile.find(name)) != profile.end() ) {
		// profile already exists
		//FIXME use atomic ints
		pi->second.total   += time;
		pi->second.current += time;
		pi->second.frames[currentPosition] += time;
	} else {
		boost::unique_lock<boost::mutex> ulk(m, boost::defer_lock);
		while (!ulk.try_lock()) {}

		// create a new profile
		auto& p = profile[name];
		p.total   = time;
		p.current = time;
		p.percent = 0;
		memset(p.frames, 0, TimeRecord::frames_size * sizeof(unsigned));
		static UnsyncedRNG rand;
		rand.Seed(spring_tomsecs(spring_gettime()));
		p.color.x = rand.RandFloat();
		p.color.y = rand.RandFloat();
		p.color.z = rand.RandFloat();
		p.showGraph = showGraph;
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
				pi->second.total.toSecsf(),
				pi->second.percent * 100);
	}
}
