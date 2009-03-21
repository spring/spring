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

#ifndef _SHAREDLIBRARY_H
#define _SHAREDLIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
	#include <stdbool.h>
#endif
#include <stddef.h> // for NULL

#ifdef _WIN32
	#include <windows.h>
	#define sharedLib_t  HINSTANCE
#else // _WIN32
	#include <dlfcn.h>
	#define sharedLib_t  void*
#endif // _WIN32

sharedLib_t sharedLib_load(const char* libFilePath);

void sharedLib_unload(sharedLib_t sharedLib);

bool sharedLib_isLoaded(sharedLib_t sharedLib);

void* sharedLib_findAddress(sharedLib_t sharedLib, const char* symbol);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SHAREDLIBRARY_H
