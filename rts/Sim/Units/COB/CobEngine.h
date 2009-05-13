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
public:
	bool operator() (const CCobThread *const &a, const CCobThread *const &b) const {return a->GetWakeTime() > b->GetWakeTime();}
};


class CCobEngine
{
protected:
	std::list<CCobThread *> running;
	std::list<CCobThread *> wantToRun;				//Threads are added here if they are in Running. And moved to real running after running is empty
	std::priority_queue<CCobThread *, vector<CCobThread *>, CCobThreadPtr_less> sleeping;
	CCobThread *curThread;
	void TickThread(int deltaTime, CCobThread* thread);
public:
	CCobEngine();
	~CCobEngine();
	void AddThread(CCobThread *thread);
	void Tick(int deltaTime);
	void ShowScriptError(const std::string& msg);
};


class CCobFileHandler
{
protected:
	std::map<std::string, CCobFile *> cobFiles;
public:
	~CCobFileHandler();
	CCobFile* GetCobFile(const std::string& name);
	CCobFile* ReloadCobFile(const std::string& name);
	const CCobFile* GetScriptAddr(const std::string& name) const;
};


extern CCobEngine GCobEngine;
extern CCobFileHandler GCobFileHandler;
extern int GCurrentTime;

#endif // __COB_ENGINE_H__
