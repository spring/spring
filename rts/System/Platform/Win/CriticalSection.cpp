/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CriticalSection.h"
#include <algorithm>


CriticalSection::CriticalSection() noexcept
{
	InitializeCriticalSection(&mtx);
}


CriticalSection::~CriticalSection()
{
	DeleteCriticalSection(&mtx);
}


void CriticalSection::lock()
{
	EnterCriticalSection(&mtx);
}


bool CriticalSection::try_lock() noexcept
{
	return TryEnterCriticalSection(&mtx);
}


void CriticalSection::unlock()
{
	LeaveCriticalSection(&mtx);
}



win_signal::win_signal() noexcept
: sleepers(false)
{
	event = CreateEvent(NULL, true, false, NULL);
}

win_signal::~win_signal()
{
	CloseHandle(event);
}

void win_signal::wait()
{
	wait_for(spring_notime);
}

void win_signal::wait_for(spring_time t)
{
	++sleepers;

	const DWORD timeout_milliseconds = t.toMilliSecsi();
	DWORD dwWaitResult;
	do {
		dwWaitResult = WaitForSingleObject(event, timeout_milliseconds);
	} while (dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_TIMEOUT);

	--sleepers;
}

void win_signal::notify_all(const int min_sleepers)
{
	if (sleepers.load() < std::max(1, min_sleepers))
		return;

	PulseEvent(event);
}
