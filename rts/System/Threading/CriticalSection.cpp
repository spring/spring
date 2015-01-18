/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CriticalSection.h"


criticalsection::criticalsection() noexcept
{
	InitializeCriticalSection(&mtx);
}


criticalsection::~criticalsection()
{
	DeleteCriticalSection(&mtx);
}


void criticalsection::lock()
{
	EnterCriticalSection(&mtx);
}


bool criticalsection::try_lock() noexcept
{
	TryEnterCriticalSection(&mtx);
}


void criticalsection::unlock()
{
	LeaveCriticalSection(&mtx);
}
