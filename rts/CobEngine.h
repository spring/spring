#pragma once

/*
 * The cob engine is responsible for "scheduling" and running threads that are running in 
 * infinite loops.
 * It also manages reading and caching of the actual .cob files
 */

#include "CobThread.h"
#include "InfoConsole.h"

#include <list>
#include <queue>
#include <map>

class CCobThread;
class CCobInstance;
class CCobFile;

using namespace std;

class CCobThreadPtr_less : public binary_function<CCobThread *, CCobThread *, bool> {
	CCobThread *a, *b;
public:
	bool operator() (const CCobThread *const &a, const CCobThread *const &b) const {return a->GetWakeTime() > b->GetWakeTime();}
};


class CCobEngine
{
protected:
	list<CCobThread *> running;
	list<CCobThread *> wantToRun;				//Threads are added here if they are in Running. And moved to real running after running is empty
	priority_queue<CCobThread *, vector<CCobThread *>, CCobThreadPtr_less> sleeping;
	list<CCobInstance *> animating;				//hash would be optimal. but not crucial.
	map<string, CCobFile *> cobFiles;
public:
	CCobEngine(void);
	~CCobEngine(void);
	void AddThread(CCobThread *thread);
	void AddInstance(CCobInstance *instance);
	void RemoveInstance(CCobInstance *instance);
	void Tick(int deltaTime);
	CCobFile &GetCobFile(string name);
};

extern CCobEngine GCobEngine;
extern int GCurrentTime;