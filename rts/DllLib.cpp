/*
 * DllLib.cpp
 * Windows Shared Object loader class implementation
 * Copyright (C) 2005 Christopher Han
 */
#include "DllLib.h"

DllLib::DllLib(const char *filename)
{
	dll = LoadLibrary(filename);
}

DllLib::~DllLib()
{
	FreeLibrary(dll);
}

void *DllLib::FindAddress(const char *symbol)
{
	return GetProcAddress(dll,symbol);
}
