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
#include <stddef.h> /* for NULL */

#ifdef _WIN32
	#include <windows.h>
	typedef HINSTANCE sharedLib_t;
#else // _WIN32
	#include <dlfcn.h>
	typedef void*     sharedLib_t;
#endif // _WIN32

/**
 * Returns the platform specific shared library extension.
 * examples:
 * - Windows:      "dll"
 * - Unix & Linux: "so"
 * - OS X:         "dylib"
 */
const char* sharedLib_getLibExtension();

/**
 * Creates a full library file name out of a base name.
 * The result is platform dependent.
 * examples (input: "foo"):
 * - Windows:      "foo.dll"
 * - Unix & Linux: "libfoo.so"
 * - OS X:         "libfoo.dylib"
 */
void sharedLib_createFullLibName(const char* libBaseName,
		char* libFullName, const size_t libFullName_sizeMax);

/**
 * Loads a shared library from a file.
 * Use sharedLib_isLoaded() to check if loading was successfull.
 */
sharedLib_t sharedLib_load(const char* libFilePath);

/**
 * Unloads a shared library.
 */
void sharedLib_unload(sharedLib_t sharedLib);

/**
 * Returns <code>true</code> if the shared library is properly loaded,
 * <code>false</code> otherwise
 */
bool sharedLib_isLoaded(sharedLib_t sharedLib);

/**
 * Returns a pointer to a function in the shared library
 * with the name given in symbol.
 */
void* sharedLib_findAddress(sharedLib_t sharedLib, const char* symbol);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _SHAREDLIBRARY_H */
