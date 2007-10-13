#ifndef TIMEPROFILER_H
#define TIMEPROFILER_H
// TimeProfiler.h: interface for the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#ifdef __powerpc__
/* HACK It won't compile under PPC Linux unless profiler is
        made a pointer, and even then it'll smash the stack
        before dying. Hopefully, someone's milage will vary. :(
 */
#undef PROFILE_TIME
#else
#define PROFILE_TIME
#endif

#ifdef PROFILE_TIME
	#define START_TIME_PROFILE(name) profiler.StartTimer(name);
	#define END_TIME_PROFILE(name) profiler.EndTimer(name);
	#define SCOPED_TIMER(name) ScopedTimer myScopedTimerFromMakro(name);
#else
	#define START_TIME_PROFILE(name) ;
	#define END_TIME_PROFILE(name) ;
	#define SCOPED_TIMER(name) ;
#endif

#ifdef PROFILE_TIME

#include <string>
#include <map>
#include <vector>
#include "SDL_types.h"
#include <boost/noncopyable.hpp>

#include "Game/UI/InputReceiver.h"

/**
@brief Time profiling is now easy
@author Karl-Robert Ernst
Construct an instance of this class where you want to begin time measuring, and destruct it at the end (or let it be autodestructed).
*/
class ScopedTimer : public boost::noncopyable
{
public:
	/**
	@brief Initialise
	*/
	ScopedTimer(const char* const name);
	
	/**
	@brief destruct and add time to profiler
	*/
	~ScopedTimer();
	
private:
	const std::string name;
	const unsigned starttime;
};

class CTimeProfiler : public CInputReceiver
{
	struct TimeRecord{
		unsigned total;
		unsigned current;
		Uint64 frames[128];
		float percent;
		float3 color;
		bool showGraph;
	};
public:
	void StartTimer(const char* name);
	void EndTimer(const char* name=0);

	void AddTime(const std::string& name, unsigned time);
	void Update();
	void Draw();
	CTimeProfiler();
	virtual ~CTimeProfiler();

	virtual bool MousePress(int x, int y, int button);
	virtual bool IsAbove(int x, int y);	
	
	std::map<std::string,TimeRecord> profile;
	
private:
	float lastBigUpdate;
	
	unsigned startTimes[1000];
	const char* startNames[1000];
	unsigned startTimeNum;
};

extern CTimeProfiler profiler;
#endif

#endif /* TIMEPROFILER_H */
