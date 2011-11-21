/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_FRAME_PREFIXER_H
#define LOG_FRAME_PREFIXER_H

#include <stdio.h> // for size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Injects a reference to the current frame number.
 * This will only possibly be called when we are in an engine binary.
 */
void log_framePrefixer_setFrameNumReference(int* frameNumReference);

/**
 * Fills a string containing the frame number, if it is available.
 * Else fils in the empty string.
 * @return the number of chars written to the result buffer.
 */
size_t log_framePrefixer_createPrefix(char* result, size_t resultSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_FRAME_PREFIXER_H

