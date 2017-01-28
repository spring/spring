/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <climits>
#include <cstring>

#include "System/TimeProfiler.h"
#include "System/GlobalRNG.h"
#include "System/Log/ILog.h"
#include "System/Threading/SpringThreading.h"

#ifdef THREADPOOL
	#include "System/Threading/ThreadPool.h"
#endif

static spring::mutex profileMutex;
static spring::unordered_map<int, std::string> hashToName;
static spring::unordered_map<int, int> refCounters;

static CGlobalUnsyncedRNG profileColorRNG;



static unsigned HashString(const char* s, size_t n)
{
	unsigned hash = 0;

	for (size_t i = 0; (i < n || n == std::string::npos); ++i) {
		if (s[i] == 0)
			break;

		hash += s[i];
		hash ^= (hash << 7) | (hash >> (sizeof(hash) * CHAR_BIT - 7));
	}

	return hash;
}

#if 0
// unused
static unsigned HashString(const std::string& s) {
	return (HashString(s.c_str(), s.size()));
}



BasicTimer::BasicTimer(const std::string& timerName)
	: nameHash(HashString(timerName))
	, startTime(spring_gettime())

	, name(timerName)
{
	const auto iter = hashToName.find(nameHash);

	if (iter == hashToName.end()) {
		hashToName.insert(std::pair<int, std::string>(nameHash, timerName)).first;
	} else {
#ifdef DEBUG
		if (iter->second != timerName) {
			LOG_L(L_ERROR, "Timer hash collision: %s <=> %s", timerName.c_str(), iter->second.c_str());
			assert(false);
		}
#endif
	}
}
#endif

BasicTimer::BasicTimer(const char* timerName)
	: nameHash(HashString(timerName, std::string::npos))
	, startTime(spring_gettime())

	, name(timerName)
{
	const auto iter = hashToName.find(nameHash);

	if (iter == hashToName.end()) {
		hashToName.insert(std::pair<int, std::string>(nameHash, timerName)).first;
	} else {
#ifdef DEBUG
		if (iter->second != timerName) {
			LOG_L(L_ERROR, "Timer hash collision: %s <=> %s", timerName, iter->second.c_str());
			assert(false);
		}
#endif
	}
}

spring_time BasicTimer::GetDuration() const
{
	return spring_difftime(spring_gettime(), startTime);
}



#if 0
// unused
ScopedTimer::ScopedTimer(const std::string& name, bool _autoShowGraph, bool _specialTimer)
	: BasicTimer(name)

	, autoShowGraph(_autoShowGraph)
	, specialTimer(_specialTimer)
{
	auto iter = refCounters.find(nameHash);

	if (iter == refCounters.end())
		iter = refCounters.insert(std::pair<int, int>(nameHash, 0)).first;

	++(iter->second);
}
#endif

ScopedTimer::ScopedTimer(const char* timerName, bool _autoShowGraph, bool _specialTimer)
	: BasicTimer(timerName)

	// Game::SendClientProcUsage depends on "Sim" and "Draw" percentages, BenchMark on "Lua"
	// note that address-comparison is intended here, timer names are (and must be) literals
	, autoShowGraph(_autoShowGraph)
	, specialTimer(_specialTimer)
{
	auto iter = refCounters.find(nameHash);

	if (iter == refCounters.end())
		iter = refCounters.insert(std::pair<int, int>(nameHash, 0)).first;

	++(iter->second);
}

ScopedTimer::~ScopedTimer()
{
	// no avoiding a second lookup since iterators can be invalidated with unordered_map
	auto iter = refCounters.find(nameHash);

	assert(iter != refCounters.end());
	assert(iter->second > 0);

	if (--(iter->second) == 0) {
		profiler.AddTime(GetName(), startTime, GetDuration(), autoShowGraph, specialTimer, false);
	}
}



ScopedOnceTimer::ScopedOnceTimer(const char* timerName)
	: startTime(spring_gettime())
	, name(timerName)
{
}

ScopedOnceTimer::ScopedOnceTimer(const std::string& timerName)
	: startTime(spring_gettime())
	, name(timerName)
{
}

ScopedOnceTimer::~ScopedOnceTimer()
{
	LOG("[%s][%s] %ims", __func__, name.c_str(), int(GetDuration().toMilliSecsi()));
}

spring_time ScopedOnceTimer::GetDuration() const
{
	return spring_difftime(spring_gettime(), startTime);
}



#if 0
// unused
ScopedMtTimer::ScopedMtTimer(const std::string& timerName, bool _autoShowGraph)
	// can not call BasicTimer's other ctor, accesses global map
	// collisions for MT timers do not need to be checked anyway
	: BasicTimer(spring_gettime())
	, autoShowGraph(_autoShowGraph)
{
	name = timerName;
}
#endif

ScopedMtTimer::ScopedMtTimer(const char* timerName, bool _autoShowGraph)
	: BasicTimer(spring_gettime())
	, autoShowGraph(_autoShowGraph)
{
	name = timerName;
}

ScopedMtTimer::~ScopedMtTimer()
{
	profiler.AddTime(GetName(), startTime, GetDuration(), autoShowGraph, false, true);
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimeProfiler::CTimeProfiler()
{
	ResetState();
}

CTimeProfiler::~CTimeProfiler()
{
	#if 0
	// should not be needed, destructor runs after main returns and all threads are gone
	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}
	#endif
}

CTimeProfiler& CTimeProfiler::GetInstance()
{
	static CTimeProfiler tp;
	return tp;
}


void CTimeProfiler::ResetState() {
	// grab lock; ThreadPool workers might already be running SCOPED_MT_TIMER
	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}

	profile.clear();
	sortedProfile.clear();
	#ifdef THREADPOOL
	profileCore.clear();
	profileCore.resize(ThreadPool::GetMaxThreads());
	#endif

	profileColorRNG.Seed(spring_tomsecs(lastBigUpdate = spring_gettime()));

	currentPosition = 0;
	resortProfiles = 0;

	enabled = false;
}

void CTimeProfiler::Update()
{
	if (!enabled) {
		UpdateRaw();
		ResortProfilesRaw();
		RefreshProfilesRaw();
		return;
	}

	// FIXME: non-locking threadsafe
	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}

	UpdateRaw();
	ResortProfilesRaw();
	RefreshProfilesRaw();
}

void CTimeProfiler::UpdateRaw()
{
	currentPosition += 1;
	currentPosition &= (TimeRecord::numFrames - 1);

	for (auto& pi: profile) {
		pi.second.frames[currentPosition] = spring_notime;
	}

	const spring_time curTime = spring_gettime();
	const float timeDiff = spring_diffmsecs(curTime, lastBigUpdate);

	if (timeDiff > 500.0f) {
		// update percentages and peaks twice every second
		for (auto& pi: profile) {
			auto& p = pi.second;

			p.percent = spring_tomsecs(p.current) / timeDiff;
			p.current = spring_notime;

			p.newLagPeak = false;
			p.newPeak = false;

			if (p.percent > p.peak) {
				p.peak = p.percent;
				p.newPeak = true;
			}
		}
		lastBigUpdate = curTime;
	}

	if (curTime.toSecsi() % 6 == 0) {
		for (auto& pi: profile) {
			(pi.second).maxLag *= 0.5f;
		}
	}
}

void CTimeProfiler::ResortProfilesRaw()
{
	if (resortProfiles > 0) {
		resortProfiles = 0;

		sortedProfile.clear();
		sortedProfile.reserve(profile.size());

		typedef std::pair<std::string, TimeRecord> TimeRecordPair;
		typedef std::function<bool(const TimeRecordPair&, const TimeRecordPair&)> ProfileSortFunc;

		const ProfileSortFunc sortFunc = [](const TimeRecordPair& a, const TimeRecordPair& b) { return (a.first < b.first); };

		// either caller already has lock, or we are disabled and thread-safe
		for (auto it = profile.begin(); it != profile.end(); ++it) {
			sortedProfile.emplace_back(it->first, it->second);
		}

		std::sort(sortedProfile.begin(), sortedProfile.end(), sortFunc);
	}
}


void CTimeProfiler::RefreshProfiles()
{
	// ProfileDrawer calls this, and is only enabled when we are
	assert(enabled);

	// lock so nothing modifies *unsorted* profiles during the refresh
	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}

	RefreshProfilesRaw();
}

void CTimeProfiler::RefreshProfilesRaw()
{
	// either called from ProfileDrawer or from Update; the latter
	// makes the "/debuginfo profiling" command work when disabled
	for (auto it = sortedProfile.begin(); it != sortedProfile.end(); ++it) {
		TimeRecord& rec = it->second;

		const bool showGraph = rec.showGraph;

		rec = profile[it->first];
		rec.showGraph = showGraph;
	}
}


float CTimeProfiler::GetPercent(const char* name)
{
	// if disabled, only special timers can pass AddTime
	// all of those are non-threaded, so no need to lock
	if (!enabled)
		return (GetPercentRaw(name));

	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}

	return (GetPercentRaw(name));
}


void CTimeProfiler::AddTime(
	const std::string& name,
	const spring_time startTime,
	const spring_time deltaTime,
	const bool showGraph,
	const bool specialTimer,
	const bool threadTimer
) {
	const spring_time t0 = spring_now();

	if (!enabled) {
		if (!specialTimer)
			return;

		assert(!threadTimer);
		AddTimeRaw(name, startTime, deltaTime, showGraph, threadTimer);
		AddTimeRaw("Misc::Profiler::AddTime", t0, spring_now() - t0, false, false);
		return;
	}

	// acquire lock at the start; one inserting thread could
	// cause a profile rehash and invalidate <pi> for another
	std::unique_lock<spring::mutex> ulk(profileMutex, std::defer_lock);
	while (!ulk.try_lock()) {}

	AddTimeRaw(name, startTime, deltaTime, showGraph, threadTimer);
	AddTimeRaw("Misc::Profiler::AddTime", t0, spring_now() - t0, false, false);
}

void CTimeProfiler::AddTimeRaw(
	const std::string& name,
	const spring_time startTime,
	const spring_time deltaTime,
	const bool showGraph,
	const bool threadTimer
) {
#ifdef THREADPOOL
	if (threadTimer)
		profileCore[ThreadPool::GetThreadNum()].emplace_back(startTime, spring_gettime());
#endif

	auto pi = profile.find(name);
	auto& p = (pi != profile.end())? pi->second: profile[name];

	// these are 0 if just created, works for both paths
	p.total   += deltaTime;
	p.current += deltaTime;

	p.newLagPeak = (p.maxLag > 0.0f && deltaTime.toMilliSecsf() > p.maxLag);
	p.maxLag     = std::max(p.maxLag, deltaTime.toMilliSecsf());

	if (pi != profile.end()) {
		// profile already exists
		p.frames[currentPosition] += deltaTime;
	} else {
		// new profile, new color
		p.color.x = profileColorRNG.NextFloat();
		p.color.y = profileColorRNG.NextFloat();
		p.color.z = profileColorRNG.NextFloat();
		p.showGraph = showGraph;

		resortProfiles += 1;
	}
}

void CTimeProfiler::PrintProfilingInfo() const
{
	if (sortedProfile.empty())
		return;

	LOG("%35s|%18s|%s", "Part", "Total Time", "Time of the last 0.5s");

	for (auto pi = sortedProfile.begin(); pi != sortedProfile.end(); ++pi) {
		const std::string& name = pi->first;
		const TimeRecord& tr = pi->second;

		LOG("%35s %16.2fms %5.2f%%", name.c_str(), tr.total.toMilliSecsf(), tr.percent * 100);
	}
}

