/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TIME_PROFILER_H
#define TIME_PROFILER_H

#include "System/Misc/SpringTime.h"
#include "System/float3.h"

#include <boost/noncopyable.hpp>
#include <cstring>
#include <string>
#include <map>
#include <deque>
#include <vector>

// disable this if you want minimal profiling
// (sim time is still measured because of game slowdown)
#define SCOPED_TIMER(name) ScopedTimer myScopedTimerFromMakro(name);
#define SCOPED_MT_TIMER(name) ScopedMtTimer myScopedTimerFromMakro(name);


class BasicTimer : public boost::noncopyable
{
public:
	BasicTimer(const std::string& myname);
	BasicTimer(const char* myname);

	const std::string& GetName() const;

protected:
	const unsigned hash;
	const spring_time starttime;
	std::map<int, std::string>::iterator nameIterator;
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
	ScopedTimer(const char* name, bool autoShow = false);
	~ScopedTimer();

private:
	const bool autoShowGraph;
	std::map<int, int>::iterator it;
};


class ScopedMtTimer : public BasicTimer
{
public:
	ScopedMtTimer(const std::string& name, bool autoShow = false);
	ScopedMtTimer(const char* name, bool autoShow = false);
	~ScopedMtTimer();

private:
	const bool autoShowGraph;
	std::map<int, int>::iterator it;
};



/**
 * @brief print passed time to infolog
 */
class ScopedOnceTimer : public BasicTimer
{
public:
	ScopedOnceTimer(const std::string& name): BasicTimer(name) {}
	ScopedOnceTimer(const char* name): BasicTimer(name) {}
	~ScopedOnceTimer();
};



class CTimeProfiler
{
public:
	CTimeProfiler();
	~CTimeProfiler();

	float GetPercent(const char *name);
	void Update();

	void PrintProfilingInfo() const;

	void AddTime(const std::string& name, const spring_time time, const bool showGraph = false);

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

	std::map<std::string,TimeRecord> profile;

	std::vector<std::deque<std::pair<spring_time,spring_time>>> profileCore;

private:
	spring_time lastBigUpdate;
	/// increases each update, from 0 to (frames_size-1)
	unsigned currentPosition;
};

extern CTimeProfiler profiler;

#endif // TIME_PROFILER_H
