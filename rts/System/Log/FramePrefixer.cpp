/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This eventually prefixes log records with the current frame number.
 */

#include "System/MainDefines.h"

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <chrono>

constexpr static int64_t  SECS_TO_NANOSECS = 1000 * 1000 * 1000;
constexpr static int64_t  MINS_TO_NANOSECS = SECS_TO_NANOSECS * 60;
constexpr static int64_t HOURS_TO_NANOSECS = MINS_TO_NANOSECS * 60;


#ifdef __cplusplus
extern "C" {
#endif

// GlobalSynced makes sure this can not be dangling
static int* frameNumRef = nullptr;

void log_framePrefixer_setFrameNumReference(int* frameNumReference)
{
	frameNumRef = frameNumReference;
}

size_t log_framePrefixer_createPrefix(char* result, size_t resultSize)
{
	const static auto refTime = std::chrono::high_resolution_clock::now();
	const        auto curTime = std::chrono::high_resolution_clock::now();

	int64_t ns = (curTime - refTime).count();

	// prefix with engine running-time in hh:mm:ss.us format since first log call
	const int32_t hh = ns / HOURS_TO_NANOSECS; ns %= HOURS_TO_NANOSECS;
	const int32_t mm = ns /  MINS_TO_NANOSECS; ns %=  MINS_TO_NANOSECS;
	const int32_t ss = ns /  SECS_TO_NANOSECS; ns %=  SECS_TO_NANOSECS;

	assert(resultSize != 0);
	using nsCastType = long long int;

	if (frameNumRef == nullptr)
		return (SNPRINTF(result, resultSize, "[t=%02d:%02d:%02d.%06lld] ", hh, mm, ss, static_cast<nsCastType>((ns / 1000) % 1000000)));

	return (SNPRINTF(result, resultSize, "[t=%02d:%02d:%02d.%06lld][f=%07d] ", hh, mm, ss, static_cast<nsCastType>((ns / 1000) % 1000000), *frameNumRef));
}

#ifdef __cplusplus
} // extern "C"
#endif

