/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/TimeProfiler.h"

#include <cstring>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

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
	} else {
		assert(nameIterator->second == myname);
	}
}


BasicTimer::BasicTimer(const char* myname)
: hash(hash_(myname))
, starttime(spring_gettime())

{
	nameIterator = hashToName.find(hash);
	if (nameIterator == hashToName.end()) {
		nameIterator = hashToName.insert(std::pair<int,std::string>(hash, myname)).first;
	} else {
		assert(nameIterator->second == myname);
	}
}


const std::string& BasicTimer::GetName() const
{
	return nameIterator->second;
}


spring_time BasicTimer::GetDuration() const
{
	return spring_difftime(spring_gettime(), starttime);
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
		profiler.AddTime(GetName(), GetDuration(), autoShowGraph);
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG("%s: %i ms", GetName().c_str(), int(GetDuration().toMilliSecsi()));
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
	profiler.AddTime(GetName(), GetDuration(), autoShowGraph);
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
	for (auto& pi: profile) {
		pi.second.frames[currentPosition] = spring_notime;
	}

	const spring_time curTime = spring_gettime();
	const float timeDiff = spring_diffmsecs(curTime, lastBigUpdate);
	if (timeDiff > 500.0f) // twice every second
	{
		for (auto& pi: profile) {
			auto& p = pi.second;
			p.percent = spring_tomsecs(p.current) / timeDiff;
			p.current = spring_notime;
			p.newLagPeak = false;
			p.newPeak = false;
			if(p.percent > p.peak) {
				p.peak = p.percent;
				p.newPeak = true;
			}
		}
		lastBigUpdate = curTime;
	}

	if (curTime.toSecsi() % 6 == 0) {
		for (auto& pi: profile) {
			auto& p = pi.second;
			p.maxLag *= 0.5f;
		}
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
	auto pi = profile.find(name);
	if (pi != profile.end()) {
		// profile already exists
		//FIXME use atomic ints
		auto& p = pi->second;
		p.total   += time;
		p.current += time;
		p.frames[currentPosition] += time;
		if (p.maxLag < time.toMilliSecsf()) {
			p.maxLag     = time.toMilliSecsf();
			p.newLagPeak = true;
		}
	} else {
		boost::unique_lock<boost::mutex> ulk(m, boost::defer_lock);
		while (!ulk.try_lock()) {}

		// create a new profile
		auto& p = profile[name];
		p.total   = time;
		p.current = time;
		p.maxLag  = time.toMilliSecsf();
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
	LOG("%35s|%18s|%s", "Part", "Total Time", "Time of the last 0.5s");

	for (auto pi = profile.begin(); pi != profile.end(); ++pi) {
		const std::string& name = pi->first;
		const TimeRecord& tr = pi->second;

		LOG("%35s %16.2fms %5.2f%%", name.c_str(), tr.total.toMilliSecsf(), tr.percent * 100);
	}
}
