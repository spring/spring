/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GAME_JOB_DISPATCHER_H
#define _GAME_JOB_DISPATCHER_H

#include <functional>
#include <queue>

#include "System/Misc/SpringTime.h"

class JobDispatcher {
public:
	struct Job {
	public:
		Job(const spring_time t = spring_notime)
		: time(t.toMilliSecsf())
		, freq(0.0f)

		, startDirect(true)
		, catchUp(false)

		, name("")
		{}

		void UpdateTime(float msecs) { time = (catchUp? time: msecs) + (1000.0f / freq); }

		bool operator < (const Job& j) const { return (time < j.time); }
		bool operator > (const Job& j) const { return (time > j.time); }

	public:
		std::function<bool()> f; // allows us to use lambdas

		float time;
		float freq;

		bool startDirect;
		bool catchUp;

		const char* name;
	};

public:
	void AddTimedJob(const Job& j) { jobs.push(j); }
	void Update() {
		const spring_time now = spring_gettime();

		while (!jobs.empty()) {
			Job j = jobs.top();

			if (j.time > now.toMilliSecsf())
				break;

			jobs.pop();

			if (!j.f())
				continue;

			j.UpdateTime(now.toMilliSecsf());
			AddTimedJob(j);
		}
	}

private:
	// job with smallest next-to-execute time goes at the top
	std::priority_queue<Job, std::vector<Job>, std::greater<Job>> jobs;
};

#endif

