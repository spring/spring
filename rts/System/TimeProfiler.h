#ifndef TIMEPROFILER_H
#define TIMEPROFILER_H
// TimeProfiler.h: interface for the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <boost/noncopyable.hpp>

#include "float3.h"

// disable this if you want minimal profiling
// (sim time is still measured because of game slowdown)
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
 * @brief Time profiling helper class
 * @author Karl-Robert Ernst
 *
 * Construct an instance of this class where you want to begin time measuring,
 * and destruct it at the end (or let it be autodestructed).
 */
class ScopedTimer : public BasicTimer
{
public:
	ScopedTimer(const char* const name);
	/**
	 * @brief destroy and add time to profiler
	 */
	~ScopedTimer();
};

class ScopedOnceTimer : public BasicTimer
{
public:
	ScopedOnceTimer(const char* const name);
	ScopedOnceTimer(const std::string& name);
	/**
	 * @brief destroy and print passed time to infolog
	 */
	~ScopedOnceTimer();
};

class CTimeProfiler
{
public:
	struct TimeRecord {
		unsigned total;
		unsigned current;
		static const unsigned frames_size = 128;
		unsigned frames[frames_size];
		float percent;
		float3 color;
		bool showGraph;
	};

	CTimeProfiler();
	~CTimeProfiler();

	float GetPercent(const char *name);
	void AddTime(const std::string& name, unsigned time);
	void Update();

	void PrintProfilingInfo() const;

	std::map<std::string,TimeRecord> profile;

private:
	unsigned lastBigUpdate;
	/// increases each update, from 0 to (frames_size-1)
	unsigned currentPosition;
};

extern CTimeProfiler profiler;

#endif // TIMEPROFILER_H
