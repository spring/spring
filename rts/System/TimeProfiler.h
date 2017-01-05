/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_PROFILER_H
#define TIME_PROFILER_H

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
#define SCOPED_TIMER(name) ScopedTimer myScopedTimerFromMakro(name);
#define SCOPED_MT_TIMER(name) ScopedMtTimer myScopedTimerFromMakro(name);


class BasicTimer : public spring::noncopyable
{
public:
	BasicTimer(const spring_time time): hash(0), starttime(time) {}
	BasicTimer(const std::string& timerName);
	BasicTimer(const char* timerName);

	const std::string& GetName() const { return (nameIterator->second); }
	spring_time GetDuration() const;

protected:
	const unsigned hash;
	const spring_time starttime;

	spring::unordered_map<int, std::string>::iterator nameIterator;
	spring::unordered_map<int, int>::iterator refsIterator;
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
	ScopedTimer(const std::string& timerName, bool autoShow = false);
	ScopedTimer(const char* timerName, bool autoShow = false);
	~ScopedTimer();

private:
	const bool autoShowGraph;
};


class ScopedMtTimer : public BasicTimer
{
public:
	ScopedMtTimer(const std::string& timerName, bool autoShow = false);
	ScopedMtTimer(const char* timerName, bool autoShow = false);
	~ScopedMtTimer();

private:
	const bool autoShowGraph;

	std::string name;
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
	const std::string& GetName() const { return name; }
	spring_time GetDuration() const;

protected:
	const spring_time starttime;

	std::string name;
};



class CTimeProfiler
{
public:
	CTimeProfiler();
	~CTimeProfiler();

	static CTimeProfiler& GetInstance();

	float GetPercent(const char *name);
	void Update();

	void PrintProfilingInfo() const;

	void AddTime(const std::string& name, const spring_time time, const bool showGraph = false);

public:
	struct TimeRecord {
		TimeRecord()
		: total(0)
		, current(0)
		, maxLag(0)
		, percent(0)
		, color(0,0,0)
		, peak(0)
		, newPeak(false)
		, newLagPeak(false)
		, showGraph(false)
		{
			memset(frames, 0, sizeof(frames));
		}
		spring_time total;
		spring_time current;
		float maxLag;
		static const unsigned frames_size = 128;
		spring_time frames[frames_size];
		float percent;
		float3 color;
		float peak;
		bool newPeak;
		bool newLagPeak;
		bool showGraph;
	};

	spring::unordered_map<std::string, TimeRecord> profile;

	std::vector< std::deque< std::pair<spring_time, spring_time> > > profileCore;

private:
	spring_time lastBigUpdate;
	/// increases each update, from 0 to (frames_size-1)
	unsigned currentPosition;
};

#define profiler (CTimeProfiler::GetInstance())

#endif // TIME_PROFILER_H
