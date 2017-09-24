/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MainDefines.h"
#include <string.h>

/*
 * This file has to be C90 compatible, as it is not only used by the engine,
 * but also by AIs, which might be compiled with compilers (for example VS)
 * that do not support C99.
 *
 * Using these functions also allows us to easily check where we are
 * sometimes writing to too small buffer, or where our source is bad,
 * for example when it is too long.
 */

#ifdef	__cplusplus
extern "C" {
#endif

char* safe_strcpy(char* destination, size_t destinationSize, const char* source)
{
	if ((destination != NULL) && (destinationSize > 0)) {
		destination[destinationSize - 1] = '\0';
		destination = STRNCPY(destination, source, destinationSize - 1);
	}

	return destination;
}

char* safe_strcat(char* destination, size_t destinationSize, const char* source)
{
	if ((destination != NULL) && (destinationSize > 0)) {
		destination[destinationSize - 1] = '\0';
		destination = STRNCAT(destination, source, destinationSize - strlen(destination) - 1);
	}

	return destination;
}

#ifdef	__cplusplus
} // extern "C"
#endif
