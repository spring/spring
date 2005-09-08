#ifndef TIMEPROFILER_H
#define TIMEPROFILER_H
// TimeProfiler.h: interface for the CTimeProfiler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMEPROFILER_H__3C2D635C_014C_4725_8254_32D218832C95__INCLUDED_)
#define AFX_TIMEPROFILER_H__3C2D635C_014C_4725_8254_32D218832C95__INCLUDED_

#define PROFILE_TIME

#ifdef PROFILE_TIME
	#define START_TIME_PROFILE profiler.StartTimer();
	#define END_TIME_PROFILE(name) profiler.EndTimer(name);
#else
	#define START_TIME_PROFILE
	#define END_TIME_PROFILE
#endif

#pragma warning(disable:4786)

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#ifdef PROFILE_TIME

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <string>
#include <map>
#include <vector>
#include "SDL_types.h"

#include "InputReceiver.h"

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
	void StartTimer();
	void EndTimer(char* name);

	void AddTime(string name,Sint64 time);
	void Update();
	void Draw();
	CTimeProfiler();
	virtual ~CTimeProfiler();

	map<string,TimeRecord> profile;
	double lastBigUpdate;

	virtual bool MousePress(int x, int y, int button);
	virtual bool IsAbove(int x, int y);

	Sint64 startTimes[1000];
	int startTimeNum;
};

extern CTimeProfiler profiler;
#endif

#endif // !defined(AFX_TIMEPROFILER_H__3C2D635C_014C_4725_8254_32D218832C95__INCLUDED_)


#endif /* TIMEPROFILER_H */
