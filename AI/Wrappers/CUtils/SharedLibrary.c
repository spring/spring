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

#include "SharedLibrary.h"
#include "System/MainDefines.h"

#include <stdio.h>

const char* sharedLib_getLibExtension() {

#ifdef _WIN32
	return "dll";
#elif defined __APPLE__
	return "dylib";
#else
	return "so";
#endif
}

void sharedLib_createFullLibName(const char* libBaseName,
		char* libFullName, const size_t libFullName_sizeMax) {

#ifdef _WIN32
	static const char* prefix = "";
#else
	static const char* prefix = "lib";
#endif

	SNPRINTF(libFullName, libFullName_sizeMax, "%s%s.%s",
			prefix, libBaseName, sharedLib_getLibExtension());
}

sharedLib_t sharedLib_load(const char* libFilePath) {
	sharedLib_t lib = NULL;

#if defined _WIN32
	if ((lib = LoadLibrary(libFilePath)) == NULL) {
		fprintf(stderr, "[SharedLibrary.c::sharedLib_load(%s)] LoadLibrary() error %d\n", libFilePath, GetLastError());
	}
#else /* defined _WIN32 */
	if ((lib = dlopen(libFilePath, RTLD_LAZY)) == NULL) {
		fprintf(stderr, "[SharedLibrary.c::sharedLib_load(%s)] dlopen() error %s\n", libFilePath, dlerror());
	}
#endif /* else defined _WIN32 */
	return lib;
}

void sharedLib_unload(sharedLib_t sharedLib) {

	if (!sharedLib_isLoaded(sharedLib)) {
		return;
	}

#if defined _WIN32
	FreeLibrary(sharedLib);
#else /* defined _WIN32 */
	dlclose(sharedLib);
#endif /* else defined _WIN32 */
	sharedLib = NULL;
}

bool sharedLib_isLoaded(sharedLib_t sharedLib) {
	return (sharedLib != NULL);
}

void* sharedLib_findAddress(sharedLib_t sharedLib, const char* symbol) {

	if (!sharedLib_isLoaded(sharedLib)) {
		return NULL;
	}

#if defined _WIN32
	return (void*) GetProcAddress(sharedLib, symbol);
#else /* defined _WIN32 */
	return dlsym(sharedLib, symbol);
#endif /* else defined _WIN32 */
}
