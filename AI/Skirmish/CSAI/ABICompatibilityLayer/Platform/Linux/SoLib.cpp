/**
 * @file SoLib.cpp
 * @brief Linux shared object loader implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Linux Shared Object loader class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "SoLib.h"
#include <dlfcn.h>

/**
 * Instantiates the loader, attempts to dlopen the
 * shared object lazily.
 */
SoLib::SoLib(const char *filename)
{
	so = dlopen(filename,RTLD_LAZY);
	if (so == NULL)
		fprintf(stderr, "%s:%d: SoLib::SoLib: %s\n", __FILE__, __LINE__, dlerror());
}

/**
 * Just dlcloses the shared object
 */
SoLib::~SoLib()
{
	dlclose(so);
}

/**
 * Attempts to locate the symbol address with dlsym
 */
void *SoLib::FindAddress(const char *symbol)
{
	void* p = dlsym(so,symbol);
	if (p == NULL)
		fprintf(stderr, "%s:%d: SoLib::FindAddress: %s\n", __FILE__, __LINE__, dlerror());
	return p;
}
