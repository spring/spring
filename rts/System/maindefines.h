/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This file has to be C90 compatible, as it is not only used by the engine,
 * but also by AIs.
 */

#ifndef _MAIN_DEFINES_H
#define _MAIN_DEFINES_H

#include <stdio.h>

#if       !defined __cplusplus && !defined bool
// include the bool type (defines: bool, true, false)
#if defined _MSC_VER
#include "booldefines.h"
#else
#include <stdbool.h>
#endif
#endif // !defined __cplusplus && !defined bool

// define a common indicator for 32bit or 64bit-ness
#if defined _WIN64 || defined __LP64__ || defined __ppc64__ || defined __ILP64__ || defined __SILP64__ || defined __LLP64__ || defined(__sparcv9)
#define __arch64__
#define __archBits__ 64
#else
#define __arch32__
#define __archBits__ 32
#endif

// define a cross-platform/-compiler compatible "%z" format replacement for
// printf() style functions.
// "%z" being the propper way for size_t typed values,
// but support for it has not yet spread wide enough.
#if defined __arch64__
#define __SIZE_T_PRINTF_FORMAT__ "%lu"
#else
#define __SIZE_T_PRINTF_FORMAT__ "%u"
#endif
// a shorter form
#define _STPF_ __SIZE_T_PRINTF_FORMAT__

// Define an abreviation for the alignment attribute.
// This should only be used in case of a known misalignment problem,
// as it adds a performance hit, even though it is very very small.
#if defined(__GNUC__) && (__GNUC__ == 4) && !defined(__arch64__) && !defined(DEDICATED_NOSSE)
#define __ALIGN_ARG__ __attribute__ ((force_align_arg_pointer))
#else
#define __ALIGN_ARG__
#endif

#ifdef _MSC_VER
	// Microsoft Visual C++ 7.0: MSC_VER = 1300
	// Microsoft Visual C++ 7.1: MSC_VER = 1310
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define PRINTF    printf_s
		#define FPRINTF   fprintf_s
		#define SNPRINTF  sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY    strcpy
		#define STRCPYS   strcpy_s
		#define STRNCPY   strncpy_s
		#define STRCAT    strcat
		#define STRCATS   strcat_s
		#define STRNCAT   strncat_s
		#define FOPEN     fopen_s
	#else              // Visual Studio 2003
		#define PRINTF    _printf
		#define FPRINTF   _fprintf
		#define SNPRINTF  _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY    _strcpy
		#define STRCPYS(dst, dstSize, src) _strcpy(dst, src)
		#define STRNCPY   _strncpy
		#define STRCAT    _strcat
		#define STRCATS(dst, dstSize, src) _strcat(dst, src)
		#define STRNCAT   _strncat
		#define FOPEN     _fopen
	#endif
	#define STRCASECMP    stricmp
#else // _MSC_VER
	// assuming GCC
	#define PRINTF     printf
	#define FPRINTF    fprintf
	#define SNPRINTF   snprintf
	#define VSNPRINTF  vsnprintf
	#define STRCPY     strcpy
	#define STRCPYS(dst, dstSize, src) strcpy(dst, src)
	#define STRNCPY    strncpy
	#define STRCAT     strcat
	#define STRCATS(dst, dstSize, src) strcat(dst, src)
	#define STRNCAT    strncat
	#define FOPEN      fopen
	#define STRCASECMP strcasecmp
#endif // _MSC_VER

#define FREE(x) free(x); x = NULL;

// define a platform independent path separator C-string and char
#ifndef sPS
	#define sPS_WIN32 "\\"
	#define sPS_POSIX "/"

	#ifdef    _WIN32
	#define sPS sPS_WIN32
	#else  // _WIN32
	#define sPS sPS_POSIX
	#endif // _WIN32
#endif // sPS
#ifndef cPS
	#define cPS_WIN32 '\\'
	#define cPS_POSIX '/'

	#ifdef    _WIN32
	#define cPS cPS_WIN32
	#else  // _WIN32
	#define cPS cPS_POSIX
	#endif // _WIN32
#endif // cPS

// define a platform independent path delimitter C-string and char
#ifndef sPD
	#define sPD_WIN32 ";"
	#define sPD_POSIX ":"

	#ifdef    _WIN32
	#define sPD sPD_WIN32
	#else  // _WIN32
	#define sPD sPD_POSIX
	#endif // _WIN32
#endif // sPD
#ifndef cPD
	#define cPD_WIN32 ';'
	#define cPD_POSIX ':'

	#ifdef    _WIN32
	#define cPD cPD_WIN32
	#else  // _WIN32
	#define cPD cPD_POSIX
	#endif // _WIN32
#endif // cPD


#endif // _MAIN_DEFINES_H
