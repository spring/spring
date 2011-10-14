/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_DEFAULT_FORMATTER_H
#define LOG_DEFAULT_FORMATTER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Injects a reference to the current frame number.
 * This will only possibly be called when we are in an engine binary.
 */
void log_formatter_setFrameNumReference(int* frameNumReference);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LOG_DEFAULT_FORMATTER_H

