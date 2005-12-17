/*
 * DllLib.h
 * Windows Shared Object loader class definition
 * Copyright (C) 2005 Christopher Han
 */
#ifndef DLLLIB_H
#define DLLLIB_H

#include "Platform/SharedLib.h"
#include <windows.h>

class DllLib: public SharedLib
{
public:
	DllLib(const char *filename);
	~DllLib();
	void *FindAddress(const char *symbol);
private:
	HINSTANCE dll;
};

#endif /* DLLLIB_H */
