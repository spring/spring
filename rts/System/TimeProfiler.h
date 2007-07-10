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
#else
	#define START_TIME_PROFILE(name) ;
	#define END_TIME_PROFILE(name) ;
#endif

#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#ifdef PROFILE_TIME

#include <string>
#include <map>
#include <vector>
#include "SDL_types.h"

#include "Game/UI/InputReceiver.h"

using namespace std;

class CTimeProfiler : public CInputReceiver
{
	struct TimeRecord{
		Uint64 total;
		Uint64 current;
		Uint64 frames[128];
		float percent;
		float3 color;
		bool showGraph;
	};
public:
	void StartTimer(const char* name);
	void EndTimer(const char* name);

	void AddTime(string name,Sint64 time);
	void Update();
	void Draw();
	CTimeProfiler();
	virtual ~CTimeProfiler();

	map<string,TimeRecord> profile;
	float lastBigUpdate;

	virtual bool MousePress(int x, int y, int button);
	virtual bool IsAbove(int x, int y);

	Sint64 startTimes[1000];
	const char* startNames[1000];
	int startTimeNum;
};

extern CTimeProfiler profiler;
#endif

#endif /* TIMEPROFILER_H */
