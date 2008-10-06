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

#ifndef _EXPORTDEFINES_H
#define	_EXPORTDEFINES_H

#ifdef	__cplusplus
extern "C" {
#endif	/* __cplusplus */

#define SPRING_CALLING_CONVENTION stdcall
#define SPRING_CALLING_CONVENTION_2 __stdcall

// extern declaration that will work across
// different platforms and compilers
#ifndef SHARED_EXPORT
	#ifdef _WIN32
		#define SHARED_EXPORT extern "C" __declspec(dllexport)
		#define SPRING_API
	#elif __GNUC__ >= 4
		// Support for '-fvisibility=hidden'.
		#define SHARED_EXPORT extern "C" __attribute__ ((visibility("default")))
		#define SPRING_API __attribute__ ((visibility("default")))
	#else
		// Older versions of gcc have everything visible; no need for fancy stuff.
		#define SHARED_EXPORT extern "C"
		#define SPRING_API
	#endif
#endif	/* SHARED_EXPORT */

#ifndef DLL_EXPORT
	#define DLL_EXPORT SHARED_EXPORT
#endif	/* DLL_EXPORT */


// calling convention declaration that will work across
// different platforms and compilers
//#ifdef __GNUC__
//	#define __stdcall
//#else
#ifndef CALLING_CONV
	#ifdef _WIN32
		#define CALLING_CONV SPRING_CALLING_CONVENTION_2
	#elif __GNUC__
		#define CALLING_CONV __attribute__ ((SPRING_CALLING_CONVENTION))
	#elif __INTEL_COMPILER
		#define CALLING_CONV __attribute__ ((SPRING_CALLING_CONVENTION))
	#else
		#define CALLING_CONV SPRING_CALLING_CONVENTION_2
	#endif
#endif	/* CALLING_CONV */
	
#ifndef CALLING_CONV_FUNC_POINTER
	#ifdef __INTEL_COMPILER
		#define CALLING_CONV_FUNC_POINTER
	#else
		#define CALLING_CONV_FUNC_POINTER CALLING_CONV
	#endif
#endif	/* CALLING_CONV_FUNC_POINTER */


#define Export(type) SHARED_EXPORT type CALLING_CONV


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif	/* _EXPORTDEFINES_H */

