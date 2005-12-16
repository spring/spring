/*
 * SoLib.h
 * Linux Shared Object loader class definition
 * Copyright (C) 2005 Christopher Han
 */
#ifndef SOLIB_H
#define SOLIB_H

#include "SharedLib.h"

class SoLib: public SharedLib
{
public:
	SoLib(const char *filename);
	~SoLib();
	void *FindAddress(const char *symbol);
private:
	void *so;
};

#endif /* SOLIB_H */
