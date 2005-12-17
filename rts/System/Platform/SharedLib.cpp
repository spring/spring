/*
 * SharedLib.cpp
 * Base shared library loading class implementation
 * Copyright (C) 2005 Christopher Han
 */
#ifdef _WIN32
#include "Win/DllLib.h"
#else
#include "Linux/SoLib.h"
#endif

SharedLib *SharedLib::instantiate(const char *filename)
{
#ifdef _WIN32
	return new DllLib(filename);
#else
	return new SoLib(filename);
#endif
}

SharedLib *SharedLib::instantiate(std::string filename)
{
	return instantiate(filename.c_str());
}
