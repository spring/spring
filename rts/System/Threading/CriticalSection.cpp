/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CriticalSection.h"


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
