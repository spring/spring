/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_ENGINE_H
#define COB_ENGINE_H

/*
 * The cob engine is responsible for "scheduling" and running threads that are
 * running in infinite loops.
 * It also manages reading and caching of the actual .cob files
 */

#include "CobThread.h"
#include "System/creg/creg_cond.h"

#include "System/creg/STL_Queue.h"
#include <vector>
#include <map>

class CCobThread;
class CCobInstance;
class CCobFile;


class CCobThreadPtr_less : public std::binary_function<CCobThread*, CCobThread*, bool> {
public:
	bool operator() (const CCobThread* const& a, const CCobThread* const& b) const {
		return a->GetWakeTime() > b->GetWakeTime();
	}
};


class CCobEngine
{
	CR_DECLARE_STRUCT(CCobEngine)
protected:
	std::vector<CCobThread*> running;
	/**
	 * Threads are added here if they are in Running.
	 * And moved to real running after running is empty.
	 */
	std::vector<CCobThread*> wantToRun;
	std::priority_queue<CCobThread*, std::vector<CCobThread*>, CCobThreadPtr_less> sleeping;
	CCobThread* curThread;
	int currentTime;
	void TickThread(CCobThread* thread);
public:
	CCobEngine();
	~CCobEngine();
	void AddThread(CCobThread* thread);
	void Tick(int deltaTime);
	void ShowScriptError(const std::string& msg);
	int GetCurrentTime() { return currentTime; }
};


class CCobFileHandler
{
protected:
	std::map<std::string, CCobFile*> cobFiles;
public:
	~CCobFileHandler();
	CCobFile* GetCobFile(const std::string& name);
	CCobFile* ReloadCobFile(const std::string& name);
	const CCobFile* GetScriptAddr(const std::string& name) const;
};


extern CCobEngine* cobEngine;
extern CCobFileHandler* cobFileHandler;

#endif // COB_ENGINE_H
