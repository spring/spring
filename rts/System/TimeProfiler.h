/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_PROFILER_H
#define TIME_PROFILER_H

#include "System/Misc/SpringTime.h"
#include "System/float3.h"

#include <boost/noncopyable.hpp>
#include <cstring>
#include <string>
#include <map>


// disable this if you want minimal profiling
// (sim time is still measured because of game slowdown)
#define SCOPED_TIMER(name) ScopedTimer myScopedTimerFromMakro(name);


class BasicTimer : public boost::noncopyable
{
public:
	BasicTimer(const std::string& myname)
		: name(myname)
		, starttime(spring_gettime())
	{
	}

protected:
	const std::string name;
	const spring_time starttime;
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
	ScopedTimer(const std::string& name, bool autoShow = false);
	~ScopedTimer();

private:
	const bool autoShowGraph;
	const unsigned hash;
};



/**
 * @brief print passed time to infolog
 */
class ScopedOnceTimer : public BasicTimer
{
public:
	ScopedOnceTimer(const std::string& name): BasicTimer(name) {}
	~ScopedOnceTimer();
};



class CTimeProfiler
{
public:
	struct TimeRecord {
		TimeRecord() : total(0), current(0), percent(0), color(0,0,0), showGraph(false), peak(0), newpeak(false) {
			memset(frames, 0, sizeof(frames));
		}
		spring_time total;
		spring_time current;
		static const unsigned frames_size = 128;
		spring_time frames[frames_size];
		float percent;
		float3 color;
		bool showGraph;
		float peak;
		bool newpeak;
	};

	CTimeProfiler();
	~CTimeProfiler();

	float GetPercent(const char *name);
	void AddTime(const std::string& name, spring_time time, bool showGraph = false);
	void Update();

	void PrintProfilingInfo() const;

	std::map<std::string,TimeRecord> profile;

private:
	spring_time lastBigUpdate;
	/// increases each update, from 0 to (frames_size-1)
	unsigned currentPosition;
};

extern CTimeProfiler profiler;

#endif // TIME_PROFILER_H
