/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Windows shared library loader implementation
 * Windows Shared Object loader class implementation
 */

#include "DllLib.h"

/**
 * Attempts to LoadLibrary on the given DLL
 */
DllLib::DllLib(const char* fileName) : dll(NULL)
{
	dll = LoadLibrary(fileName);
}

/**
 * Does a FreeLibrary on the given DLL
 */
void DllLib::Unload() {

	FreeLibrary(dll);
	dll = NULL;
}

bool DllLib::LoadFailed() {
	return dll == NULL;
}

/**
 * Does a FreeLibrary on the given DLL
 */
DllLib::~DllLib()
{
	Unload();
}

/**
 * Attempts to locate the given symbol with GetProcAddress
 */
void* DllLib::FindAddress(const char* symbol)
{
	return (void*) GetProcAddress(dll,symbol);
}
