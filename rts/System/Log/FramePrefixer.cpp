/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This eventually prefixes log records with the current frame number.
 */

#include "System/MainDefines.h"

#include <cstdarg>
#include <cstring>


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
	if (frameNumRef == nullptr) {
		if (resultSize > 0) {
			result[0] = '\0';
			return 1;
		}
		return 0;
	}

	return (SNPRINTF(result, resultSize, "[f=%07d] ", *frameNumRef));
}

#ifdef __cplusplus
} // extern "C"
#endif

