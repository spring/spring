#ifndef __COB_ENGINE_H__
#define __COB_ENGINE_H__

/*
 * The cob engine is responsible for "scheduling" and running threads that are running in 
 * infinite loops.
 * It also manages reading and caching of the actual .cob files
 */

#include "CobThread.h"
#include "LogOutput.h"

#include <list>
#include <queue>
#include <map>

class CCobThread;
class CCobInstance;
class CCobFile;

class CCobThreadPtr_less : public std::binary_function<CCobThread *, CCobThread *, bool> {
	CCobThread *a, *b;
public:
	bool operator() (const CCobThread *const &a, const CCobThread *const &b) const {return a->GetWakeTime() > b->GetWakeTime();}
};


class CCobEngine
{
protected:
	std::list<CCobThread *> running;
	std::list<CCobThread *> wantToRun;				//Threads are added here if they are in Running. And moved to real running after running is empty
	std::priority_queue<CCobThread *, vector<CCobThread *>, CCobThreadPtr_less> sleeping;
	std::list<CCobInstance *> animating;				//hash would be optimal. but not crucial.
	std::map<std::string, CCobFile *> cobFiles;
	CCobThread *curThread;
public:
	CCobEngine(void);
	~CCobEngine(void);
	void AddThread(CCobThread *thread);
	void AddInstance(CCobInstance *instance);
	void RemoveInstance(CCobInstance *instance);
	void Tick(int deltaTime);
	void SetCurThread(CCobThread *cur);
	void ShowScriptWarning(const std::string& msg);
	void ShowScriptError(const std::string& msg);
	CCobFile& GetCobFile(const std::string& name);
	CCobFile& ReloadCobFile(const std::string& name);
	const CCobFile* GetScriptAddr(const std::string& name) const;
};

extern CCobEngine GCobEngine;
extern int GCurrentTime;

#endif // __COB_ENGINE_H__
