/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * This file has to be C99 compatible, as it is not only used by the engine,
 * but also by AIs.
 */

#ifndef _MAINDEFINES_H
#define _MAINDEFINES_H

// include the bool type
#if !defined __cplusplus && !defined bool
#include <stdbool.h> // defines: bool, true, false
#endif // !defined __cplusplus && !defined bool

#ifdef _MSC_VER
	// Microsoft Visual C++ 7.0: MSC_VER = 1300
	// Microsoft Visual C++ 7.1: MSC_VER = 1310
	#if _MSC_VER > 1310 // >= Visual Studio 2005
		#define PRINTF printf_s
		#define FPRINTF fprintf_s
		#define SNPRINTF sprintf_s
		#define VSNPRINTF vsprintf_s
		#define STRCPY strcpy_s
		#define STRCAT strcat_s
		#define STRNCAT strncat_s
		#define FOPEN fopen_s
	#else              // Visual Studio 2003
		#define PRINTF _printf
		#define FPRINTF _fprintf
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
		#define STRCPY strcpy
		#define STRCAT strcat
		#define STRNCAT strncat
		#define FOPEN fopen
	#endif
	#define STRCASECMP stricmp
#else // _MSC_VER
	// assuming GCC
	#define PRINTF printf
	#define FPRINTF fprintf
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define STRCPY strcpy
	#define STRCAT strcat
	#define STRNCAT strncat
	#define FOPEN fopen
	#define STRCASECMP strcasecmp
#endif // _MSC_VER

// define a platform independent path separator C-string
#ifndef sPS
#ifdef _WIN32
#define sPS "\\"
#else // _WIN32
#define sPS "/"
#endif // _WIN32
#endif

#endif // _MAINDEFINES_H
