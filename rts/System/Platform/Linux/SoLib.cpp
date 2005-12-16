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
}

SoLib::~SoLib()
{
	dlclose(so);
}

void *SoLib::FindAddress(const char *symbol)
{
	return dlsym(so,symbol);
}
