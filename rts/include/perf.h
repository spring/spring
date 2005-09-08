/*
 * perf.h
 * Performance testing based on platform
 * Copyright (C) 2005 Christopher Han
 */
#ifndef PERF_H
#define PERF_H

#ifdef _WIN32

#include <windows.h>

static inline bool perfCounter(Uint64 *count)
{
	LARGE_INTEGER c;
	bool ret = QueryPerformanceCounter(c);
	*count = c.QuadPart;
	return ret;
}

static inline bool perfFrequency(Uint64 *frequence)
{
	LARGE_INTEGER f;
	bool ret = QueryPerformanceFrequency(f);
	*frequence = f.QuadPart;
	return ret;
}

#else

#include <sys/time.h>
extern Uint64 init_time;

static inline bool perfcount(Uint64 *count, Uint64 *freq)
{
	struct timeval now;
	if (!count)
		return false;
	gettimeofday(&now,NULL);
	*count = (((now.tv_usec - init_time) * (Uint32)1000000 + now.tv_usec) * 105) / 88;
	if (freq)
		*freq = 1193182;
	return true;
}

static inline bool perfCounter(Uint64 *count)
{
	return perfcount(count, NULL);
}

static inline bool perfFrequency(Uint64 *frequence)
{
	Uint64 counter;
	return perfcount(&counter,frequence);
}

#endif

#endif
