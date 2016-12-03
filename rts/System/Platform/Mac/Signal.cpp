/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Signal.h"
#include <chrono>
#include <algorithm>


mac_signal::mac_signal()
{
	sleepers = 0;
}


mac_signal::~mac_signal()
{
}


void mac_signal::wait()
{
	sleepers++;
	{
		std::lock_guard<std::mutex> lk;
		cv.wait(lk);
	}
	sleepers--;
}


void mac_signal::wait_for(spring_time t)
{
	sleepers++;
	{
		std::chrono::nanoseconds ns(t.toNanoSecsi());
		std::lock_guard<std::mutex> lk;
		cv.wait_for(lk, ns);
	}
	sleepers--;
}


void mac_signal::notify_all(const int min_sleepers = 1)
{
	if (sleepers.load() < std::max(1, min_sleepers))
		return;

	cv.notify_all();
}
