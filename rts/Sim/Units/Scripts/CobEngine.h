/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COB_ENGINE_H
#define COB_ENGINE_H

/*
 * Simple VM responsible for "scheduling" and running COB threads.
 * It also manages reading and caching of the actual .cob files.
 */

#include <vector>

#include "CobThread.h"
#include "System/creg/creg_cond.h"
#include "System/creg/STL_Queue.h"
#include "System/creg/STL_Map.h"


class CCobThread;
class CCobInstance;
class CCobFile;
class CCobFileHandler;

class CCobEngine
{
	CR_DECLARE_STRUCT(CCobEngine)

private:
	struct SleepingThread {
		CR_DECLARE_STRUCT(SleepingThread)

		int id;
		int wt;
	};

	struct CCobThreadComp: public std::binary_function<const SleepingThread&, const SleepingThread&, bool> {
	public:
		bool operator() (const SleepingThread& a, const SleepingThread& b) const {
			return (a.wt > b.wt);
		}
	};

public:
	void Init() {
		threadInstances.reserve(2048);
		tickAddedThreads.reserve(128);

		runningThreadIDs.reserve(512);
		waitingThreadIDs.reserve(512);

		threadCounter = 0;
	}
	void Kill() {
		// threadInstances is never explicitly iterated, so
		// calling clear_unordered_map (between reloads) is
		// unnecessary here
		threadInstances.clear();
		tickAddedThreads.clear();

		runningThreadIDs.clear();
		waitingThreadIDs.clear();

		while (!sleepingThreadIDs.empty()) {
			sleepingThreadIDs.pop();
		}
	}

	void Tick(int deltaTime);
	void ShowScriptError(const std::string& msg);


	CCobThread* GetThread(int threadID) {
		const auto it = threadInstances.find(threadID);

		if (it == threadInstances.end())
			return nullptr;

		return &(it->second);
	}

	bool RemoveThread(int threadID) {
		const auto it = threadInstances.find(threadID);

		if (it != threadInstances.end()) {
			threadInstances.erase(it);
			return true;
		}

		return false;
	}

	int AddThread(CCobThread&& thread);
	int GenThreadID() { return (threadCounter++); }
	int GetCurrentTime() const { return currentTime; }

	void QueueAddThread(CCobThread&& thread) { tickAddedThreads.emplace_back(std::move(thread)); }
	void AddQueuedThreads() {
		// move new threads spawned by START into threadInstances;
		// their ID's will already have been scheduled into either
		// waitingThreadIDs or sleepingThreadIDs
		for (CCobThread& t: tickAddedThreads) {
			AddThread(std::move(t));
		}

		tickAddedThreads.clear();
	}

	void ScheduleThread(const CCobThread* thread);
	void SanityCheckThreads(const CCobInstance* owner);

private:
	void TickThread(CCobThread* thread);

	void WakeSleepingThreads();
	void TickRunningThreads() {
		// advance all currently running threads
		for (const int threadID: runningThreadIDs) {
			TickThread(GetThread(threadID));
		}

		// a thread can never go from running->running, so clear the list
		// note: if preemption was to be added, this would no longer hold
		// however, TA scripts can not run preemptively anyway since there
		// aren't any synchronization methods available
		runningThreadIDs.clear();

		// prepare threads that will run next frame
		std::swap(runningThreadIDs, waitingThreadIDs);
	}

private:
	// registry of every thread across all script instances
	spring::unordered_map<int, CCobThread> threadInstances;
	// threads that are spawned during Tick
	std::vector<CCobThread> tickAddedThreads;

	std::vector<int> runningThreadIDs;
	std::vector<int> waitingThreadIDs;

	// stores <id, waketime> pairs s.t. after waking up the ID can be checked
	// for validity; thread owner might get removed while a thread is sleeping
	std::priority_queue<SleepingThread, std::vector<SleepingThread>, CCobThreadComp> sleepingThreadIDs;

	CCobThread* curThread = nullptr;

	int currentTime = 0;
	int threadCounter = 0;
};


extern CCobEngine* cobEngine;

#endif // COB_ENGINE_H
