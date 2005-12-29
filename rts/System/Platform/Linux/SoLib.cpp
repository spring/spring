/*
 * SoLib.cpp
 * Linux Shared Object loader class implementation
 * Copyright (C) 2005 Christopher Han
 */
#include "SoLib.h"
#include <dlfcn.h>

SoLib::SoLib(const char *filename)
{
	so = dlopen(filename,RTLD_LAZY);
	if (so == NULL)
		fprintf(stderr, "%s:%d: SoLib::SoLib: %s\n", __FILE__, __LINE__, dlerror());
}

SoLib::~SoLib()
{
	dlclose(so);
}

void *SoLib::FindAddress(const char *symbol)
{
	void* p = dlsym(so,symbol);
	if (p == NULL)
		fprintf(stderr, "%s:%d: SoLib::FindAddress: %s\n", __FILE__, __LINE__, dlerror());
	return p;
}
