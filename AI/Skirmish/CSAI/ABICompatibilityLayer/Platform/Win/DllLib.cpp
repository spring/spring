/**
 * @file DllLib.cpp
 * @brief Windows shared library loader implementation
 * @author Christopher Han <xiphux@gmail.com>
 * 
 * Windows Shared Object loader class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include "DllLib.h"

/**
 * Attempts to LoadLibrary on the given DLL
 */
DllLib::DllLib(const char *filename)
{
	dll = LoadLibrary(filename);
}

/**
 * Does a FreeLibrary on the given DLL
 */
void DllLib::Unload() {
	
	if (dll != NULL) {
		FreeLibrary(dll);
		dll = NULL;
	}
}

bool DllLib::LoadFailed() {
	return dll == NULL;
}

DllLib::~DllLib()
{
	Unload();
}

/**
 * Attempts to locate the given symbol with GetProcAddress
 */
void *DllLib::FindAddress(const char *symbol)
{
	return (void*) GetProcAddress(dll,symbol);
}
