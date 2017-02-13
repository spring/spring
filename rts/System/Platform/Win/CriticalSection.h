/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CRITICALSECTION_H
#define CRITICALSECTION_H

#if   defined(_WIN32)

#include <windows.h>
#include <atomic>
#include "System/Misc/SpringTime.h"


class CriticalSection
{
private:
	typedef CRITICAL_SECTION native_type;

public:
	typedef native_type* native_handle_type;

	CriticalSection() noexcept;
	~CriticalSection();

	CriticalSection(const CriticalSection&) = delete;
	CriticalSection& operator=(const CriticalSection&) = delete;

	void lock();
	bool try_lock() noexcept;
	void unlock();

	native_handle_type native_handle() { return &mtx; }

protected:
	native_type mtx;
};



class win_signal {
public:
	win_signal() noexcept;
	~win_signal();

	win_signal(const win_signal&) = delete;
	win_signal& operator=(const win_signal&) = delete;

	void wait();
	void wait_for(spring_time t);
	void notify_all(const int min_sleepers = 1);

protected:
	std::atomic_int sleepers;
	HANDLE event;
};

#endif

#endif // CRITICALSECTION_H
