/*
 * SharedLib.h
 * Base shared library loading class definitiion
 * Copyright (C) 2005 Christopher Han
 */
#ifndef SHAREDLIB_H
#define SHAREDLIB_H

#include <string>

#ifndef _WIN32
#define WINAPI
#endif

class SharedLib
{
public:
	static SharedLib *instantiate(const char *filename);
	static SharedLib *instantiate(std::string filename);
	virtual void *FindAddress(const char *symbol) = 0;
};

#endif /* SHAREDLIB_H */
