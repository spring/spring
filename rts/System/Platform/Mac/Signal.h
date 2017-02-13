/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIGNAL_H
#define SIGNAL_H


#include <mutex>
#include <atomic>
#include <condition_variable>
#include "System/Misc/SpringTime.h"


class mac_signal {
public:
	mac_signal() noexcept { sleepers = 0; }

	mac_signal(const mac_signal&) = delete;
	mac_signal& operator=(const mac_signal&) = delete;

	void wait();
	void wait_for(spring_time t);
	void notify_all(const int min_sleepers = 1);

protected:
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic_int sleepers;
};

#endif // SIGNAL_H
