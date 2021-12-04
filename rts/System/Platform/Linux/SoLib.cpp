/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * Linux, Unix and Mac OS X Shared Object loader class implementation
 */

#include "SoLib.h"
#include <dlfcn.h>

/**
 * Instantiates the loader, attempts to dlopen the
 * shared object lazily.
 */
SoLib::SoLib(const char* fileName) : so(NULL)
{
	so = dlopen(fileName, RTLD_LAZY);
	if (so == NULL) {
		SharedLib::reportError(dlerror(), __FILE__, __LINE__, "SoLib::SoLib");
	}
}

/**
 * Just dlcloses the shared object
 */
void SoLib::Unload() {

	if (so != NULL) {
		dlclose(so);
		so = NULL;
	}
}

bool SoLib::LoadFailed() {
	return so == NULL;
}

/**
 * Just dlcloses the shared object
 */
SoLib::~SoLib()
{
	Unload();
}

/**
 * Attempts to locate the symbol address with dlsym
 */
void* SoLib::FindAddress(const char* symbol)
{
	if (so != NULL) {
		void* p = dlsym(so, symbol);
		if (p == NULL) {
			//SharedLib::reportError(dlerror(), __FILE__, __LINE__, "SoLib::FindAddress");
		}
		return p;
	}
	return NULL;
}
