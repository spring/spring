/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This file has to be C90 compatible, as it is not only used by the engine,
 * but also by AIs, which might be compiled with compilers (for example VS)
 * that do not support C99.
 */

#ifndef MAIN_DEFINES_H
#define MAIN_DEFINES_H

#include <stdio.h>

#if       !defined __cplusplus && !defined bool
/* include the bool type (defines: bool, true, false) */
#include <stdbool.h>
#endif /* !defined __cplusplus && !defined bool */

/* define if we have a X11 enviroment (:= linux/freebsd) */
#if !defined(__APPLE__) && !defined(_WIN32)
	//FIXME move this check to cmake, which has FindX11.cmake?
	#define _X11
#endif


#if (defined(__alpha__) || defined(__arm__) || defined(__aarch64__) || defined(__mips__) || defined(__powerpc__) || defined(__sparc__) || defined(__m68k__) || defined(__ia64__))
#define __is_x86_arch__ 0
#elif (defined(__i386__) || defined(__x86_64__) || defined(__amd64__) || defined(_M_AMD64) || defined(_M_IX86) || defined(_M_X64))
#define __is_x86_arch__ 1
#else
#error unknown CPU-architecture
#endif


/* define a common indicator for 32bit or 64bit-ness */
#if defined _WIN64 || defined __LP64__ || defined __ppc64__ || defined __ILP64__ || defined __SILP64__ || defined __LLP64__ || defined(__sparcv9)
#define __arch64__
#define __archBits__ 64
#else
#define __arch32__
#define __archBits__ 32
#endif


/*
  define a cross-platform/-compiler compatible "%z" format replacement for
  printf() style functions.
  "%z" being the propper way for size_t typed values,
  but support for it has not yet spread wide enough.
*/
#if defined __arch64__
	#if defined _WIN64
		#define __SIZE_T_PRINTF_FORMAT__ "%I64u"
	#else
		#define __SIZE_T_PRINTF_FORMAT__ "%lu"
	#endif
#else
	#define __SIZE_T_PRINTF_FORMAT__ "%u"
#endif
/* a shorter form */
#define _STPF_ __SIZE_T_PRINTF_FORMAT__


#if defined(_MSC_VER)
	#define _threadlocal __declspec(thread)
#else
	#define _threadlocal __thread
#endif


#ifdef __GNUC__
	#define _deprecated __attribute__ ((deprecated))
#else
	#define _deprecated
#endif


#ifdef _MSC_VER
	/* Microsoft Visual C++ 7.0: MSC_VER = 1300
	   Microsoft Visual C++ 7.1: MSC_VER = 1310 */
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define PRINTF    printf_s
		#define FPRINTF   fprintf_s
		#define SNPRINTF  _snprintf /* sprintf_s misbehaves in debug mode, triggering breakpoints */
		#define VSNPRINTF _vsnprintf /* vsprintf_s misbehaves in debug mode, triggering breakpoints */
		#define STRCPY    strcpy
		#define STRCPYS   strcpy_s
		#define STRNCPY   strncpy
		#define STRCAT    strcat
		#define STRCATS   strcat_s
		#define STRNCAT   strncat
		#define FOPEN     fopen_s
	#else              /* Visual Studio 2003 */
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
#else /* _MSC_VER */
	/* assuming GCC */
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
#endif /* _MSC_VER */

#define FREE(x) do { free(x); x = NULL; } while(false);

/* define a platform independent path separator C-string and char */
#ifndef sPS
	#define sPS_WIN32 "\\"
	#define sPS_POSIX "/"

	#ifdef    _WIN32
	#define sPS sPS_WIN32
	#else  // _WIN32
	#define sPS sPS_POSIX
	#endif // _WIN32
#endif /* sPS */
#ifndef cPS
	#define cPS_WIN32 '\\'
	#define cPS_POSIX '/'

	#ifdef    _WIN32
	#define cPS cPS_WIN32
	#else  /* _WIN32 */
	#define cPS cPS_POSIX
	#endif /* _WIN32 */
#endif /* cPS */

/* define a platform independent path delimitter C-string and char */
#ifndef sPD
	#define sPD_WIN32 ";"
	#define sPD_POSIX ":"

	#ifdef    _WIN32
	#define sPD sPD_WIN32
	#else  /* _WIN32 */
	#define sPD sPD_POSIX
	#endif /* _WIN32 */
#endif /* sPD */
#ifndef cPD
	#define cPD_WIN32 ';'
	#define cPD_POSIX ':'

	#ifdef    _WIN32
	#define cPD cPD_WIN32
	#else  /* _WIN32 */
	#define cPD cPD_POSIX
	#endif /* _WIN32 */
#endif /* cPD */

/* WORKAROUND (2013) a pthread stack alignment problem, else SSE code would crash
   more info: http://www.peterstock.co.uk/games/mingw_sse/ (TLDR: mingw-32 aligns
   thread stacks to 4 bytes but we want 16-byte alignment)
*/
#if (defined(__MINGW32__) && defined(__GNUC__) && (__GNUC__ >= 4))
#define __FORCE_ALIGN_STACK__ __attribute__ ((force_align_arg_pointer))
#else
#define __FORCE_ALIGN_STACK__
#endif

#endif /* MAIN_DEFINES_H */

