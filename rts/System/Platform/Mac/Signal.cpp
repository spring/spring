/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Signal.h"
#include <chrono>
#include <algorithm>

void mac_signal::wait()
{
	sleepers++;
	{
		std::unique_lock<std::mutex> lk(mtx);
		cv.wait(lk);
	}
	sleepers--;
}


void mac_signal::wait_for(spring_time t)
{
	sleepers++;
	{
		std::chrono::nanoseconds ns(t.toNanoSecsi());
		std::unique_lock<std::mutex> lk(mtx);
		cv.wait_for(lk, ns);
	}
	sleepers--;
}


void mac_signal::notify_all(const int min_sleepers)
{
	if (sleepers.load() < std::max(1, min_sleepers))
		return;

	cv.notify_all();
}

