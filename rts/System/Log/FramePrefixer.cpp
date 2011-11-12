/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * This eventually prefixes log records with the current frame number.
 */

#include "System/maindefines.h"

#include <cstdarg>
#include <cstring>


#ifdef __cplusplus
extern "C" {
#endif

static int* frameNum = NULL;

void log_framePrefixer_setFrameNumReference(int* frameNumReference)
{
	frameNum = frameNumReference;
}

size_t log_framePrefixer_createPrefix(char* result, size_t resultSize)
{
	if (frameNum == NULL) {
		if (resultSize > 0) {
			result[0] = '\0';
			return 1;
		}
		return 0;
	} else {
		return SNPRINTF(result, resultSize, "[f=%07d] ", *frameNum);
	}
}

#ifdef __cplusplus
} // extern "C"
#endif

