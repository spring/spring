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
SoLib::SoLib(const char* fileName) : so(nullptr)
{
	so = dlopen(fileName, RTLD_LAZY);
	if (so == nullptr) {
		SharedLib::reportError(dlerror(), __FILE__, __LINE__, "SoLib::SoLib");
	}
}

/**
 * Just dlcloses the shared object
 */
void SoLib::Unload() {

	if (so != nullptr) {
		dlclose(so);
		so = nullptr;
	}
}

bool SoLib::LoadFailed() {
	return so == nullptr;
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
	if (so != nullptr) {
		void* p = dlsym(so, symbol);
		if (p == nullptr) {
			//SharedLib::reportError(dlerror(), __FILE__, __LINE__, "SoLib::FindAddress");
		}
		return p;
	}
	return nullptr;
}
