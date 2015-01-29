/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_JOB_DISPATCHER_H
#define _GAME_JOB_DISPATCHER_H

#include <functional>
#include <map>

#include "System/Misc/SpringTime.h"

class JobDispatcher {
public:
	struct Job {
		Job()
		: freq(0)
		//, inSim(false)
		//, inDraw(false)
		, startDirect(true)
		, catchUp(false)
		, name("")
		{}

		std::function<bool()> f; // allows us to use lambdas

		float freq;
		//bool inSim;
		//bool inDraw;
		bool startDirect;
		bool catchUp;

		const char* name;
	};

public:
	JobDispatcher() { jobs.clear(); }
	~JobDispatcher() { jobs.clear(); }

	void AddTimedJob(Job j, const spring_time t) {
		spring_time jobTime = t;

		// never overwrite one job by another (!)
		while (jobs.find(jobTime) != jobs.end())
			jobTime += spring_time(1);

		jobs[jobTime] = j;
	}

	void Update() {
		const spring_time now = spring_gettime();

		auto it = jobs.begin();

		while (it != jobs.end()) {
			if (it->first > now) {
				++it; continue;
			}

			Job* j = &it->second;

			if (j->f()) {
				AddTimedJob(*j, (j->catchUp ? it->first : spring_gettime()) + spring_time(1000.0f / j->freq));
			}

			jobs.erase(it); //FIXME remove by range? (faster!)
			it = jobs.begin();
		}
	}

private:
	std::map<spring_time, Job> jobs;
};

#endif

