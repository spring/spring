#ifndef TIMEPROFILER_H
#define TIMEPROFILER_H
// TimeProfiler.h: interface for the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <boost/noncopyable.hpp>

#include "float3.h"

// disable this if you want minimal profiling (sim time is still measured because of game slowdown)
#define SCOPED_TIMER(name) ScopedTimer myScopedTimerFromMakro(name);


class BasicTimer : public boost::noncopyable
{
public:
	BasicTimer(const char* const name);

protected:
	const std::string name;
	const unsigned starttime;
};

/**
@brief Time profiling helper class
@author Karl-Robert Ernst

Construct an instance of this class where you want to begin time measuring, and destruct it at the end (or let it be autodestructed).
*/
class ScopedTimer : public BasicTimer
{
public:
	ScopedTimer(const char* const name);
	/**
	@brief destruct and add time to profiler
	*/
	~ScopedTimer();
};

class ScopedOnceTimer : public BasicTimer
{
public:
	ScopedOnceTimer(const char* const name);
	/**
	@brief destruct and print needed time to infolog
	*/
	~ScopedOnceTimer();
};

class CTimeProfiler
{
public:
	struct TimeRecord{
		unsigned total;
		unsigned current;
		unsigned frames[128];
		float percent;
		float3 color;
		bool showGraph;
	};

	CTimeProfiler();
	~CTimeProfiler();

	float GetPercent(const char *name);
	void AddTime(const std::string& name, unsigned time);
	void Update();
	
	std::map<std::string,TimeRecord> profile;
	
private:
	unsigned lastBigUpdate;
	unsigned currentPosition; // increases each update, from 0 to 127
};

extern CTimeProfiler profiler;

#endif /* TIMEPROFILER_H */
