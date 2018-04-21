/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_PROFILER_H
#define TIME_PROFILER_H

#include <atomic>
#include <cstring> // memset
#include <string>
#include <deque>
#include <vector>

#include "System/Misc/SpringTime.h"
#include "System/Misc/NonCopyable.h"
#include "System/float3.h"
#include "System/UnorderedMap.hpp"

// disable this if you want minimal profiling
// (sim time is still measured because of game slowdown)
#define SCOPED_TIMER(name) ScopedTimer __scopedTimer(name);
#define SCOPED_SPECIAL_TIMER(name) ScopedTimer __scopedTimer(name, false, true)
#define SCOPED_MT_TIMER(name) ScopedMtTimer __scopedTimer(name);


class BasicTimer : public spring::noncopyable
{
public:
	BasicTimer(const spring_time time): nameHash(0), startTime(time) {}
	BasicTimer(const std::string& timerName);
	BasicTimer(const char* timerName);

	const std::string& GetName() const { return name; }
	spring_time GetDuration() const;

protected:
	const unsigned nameHash;
	const spring_time startTime;

	std::string name;
};


/**
 * @brief Time profiling helper class
 *
 * Construct an instance of this class where you want to begin time measuring,
 * and destruct it at the end (or let it be autodestructed).
 */
class ScopedTimer : public BasicTimer
{
public:
	ScopedTimer(const std::string& timerName, bool _autoShowGraph = false, bool _specialTimer = false);
	ScopedTimer(const char* timerName, bool _autoShowGraph = false, bool _specialTimer = false);
	~ScopedTimer();

private:
	const bool autoShowGraph;
	const bool specialTimer;
};


class ScopedMtTimer : public BasicTimer
{
public:
	ScopedMtTimer(const std::string& timerName, bool _autoShowGraph = false);
	ScopedMtTimer(const char* timerName, bool _autoShowGraph = false);
	~ScopedMtTimer();

private:
	const bool autoShowGraph;
};



/**
 * @brief print passed time to infolog
 */
class ScopedOnceTimer
{
public:
	ScopedOnceTimer(const std::string& name);
	ScopedOnceTimer(const char* name);
	~ScopedOnceTimer();

	spring_time GetDuration() const;

protected:
	const spring_time startTime;

	std::string name;
};



class CTimeProfiler
{
public:
	CTimeProfiler();
	~CTimeProfiler();

	static CTimeProfiler& GetInstance();

	struct TimeRecord {
		TimeRecord()
		: total(0.0f)
		, current(0.0f)

		, newPeak(false)
		, newLagPeak(false)
		, showGraph(false)
		{
			memset(frames, 0, sizeof(frames));
		}

		static constexpr unsigned numFrames = 128;

		spring_time total;
		spring_time current;
		spring_time frames[numFrames];

		// .x := maximum dt, .y := time-percentage, .z := peak-percentage
		float3 stats;
		float3 color;

		bool newPeak;
		bool newLagPeak;
		bool showGraph;
	};

public:
	std::vector< std::pair<std::string, TimeRecord> >& GetSortedProfiles() { return sortedProfiles; }
	std::vector< std::deque< std::pair<spring_time, spring_time> > >& GetThreadProfiles() { return threadProfiles; }

	size_t GetNumSortedProfiles() const { return (sortedProfiles.size()); }
	size_t GetNumThreadProfiles() const { return (threadProfiles.size()); }

	float GetTimePercentage(const char* name) const { return (GetTimeRecord(name).stats.y); }
	float GetTimePercentageRaw(const char* name) const { return (GetTimeRecordRaw(name).stats.y); }

	const TimeRecord& GetTimeRecord(const char* name) const;
	const TimeRecord& GetTimeRecordRaw(const char* name) const {
		// do not default-create keys, breaks resorting
		const auto it = profiles.find(name);
		const static TimeRecord rec;

		if (it == profiles.end())
			return rec;

		return (it->second);
	}

	void ToggleLock(bool lock);
	void ResetState();
	void ResetPeaks() {
		ToggleLock(true);

		for (auto& p: profiles)
			p.second.stats.z = 0.0f;

		ToggleLock(false);
	}

	void Update();
	void UpdateRaw();

	void ResortProfilesRaw();
	void RefreshProfiles();
	void RefreshProfilesRaw();

	void SetEnabled(bool b) { enabled = b; }
	void PrintProfilingInfo() const;

	void AddTime(
		const std::string& name,
		const spring_time startTime,
		const spring_time deltaTime,
		const bool showGraph = false,
		const bool specialTimer = false,
		const bool threadTimer = false
	);
	void AddTimeRaw(
		const std::string& name,
		const spring_time startTime,
		const spring_time deltaTime,
		const bool showGraph,
		const bool threadTimer
	);

private:
	spring::unordered_map<std::string, TimeRecord> profiles;

	std::vector< std::pair<std::string, TimeRecord> > sortedProfiles;
	std::vector< std::deque< std::pair<spring_time, spring_time> > > threadProfiles;

	spring_time lastBigUpdate;

	/// increases each update, from 0 to (numFrames-1)
	unsigned currentPosition;
	unsigned resortProfiles;

	// if false, AddTime is a no-op for (almost) all timers
	std::atomic<bool> enabled;
};

#define profiler (CTimeProfiler::GetInstance())

#endif // TIME_PROFILER_H
