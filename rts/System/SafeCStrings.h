/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SAFE_C_STRINGS_H
#define SAFE_C_STRINGS_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * @name safe_string_manipulation
 * These functions ensure that no buffer overrun occurs, and that the
 * destination strings are always terminated with '\0', as long as the
 * destination buffer has space for at least one character.
 */
///@{

/// @see strncpy
char* safe_strcpy(char* destination, size_t destinationSize, const char* source);

/// @see strncat
char* safe_strcat(char* destination, size_t destinationSize, const char* source);

#define STRCPY_T safe_strcpy
#define STRCAT_T safe_strcat
///@}

#ifdef	__cplusplus
} /* extern "C" */
#endif

#endif /* SAFE_C_STRINGS_H */
