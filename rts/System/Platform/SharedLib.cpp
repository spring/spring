/**
 * @file SharedLib.cpp
 * @brief shared library loader implementation
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Base shared library loading class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#include "StdAfx.h"
#include "LogOutput.h"
#ifdef _WIN32
#include "Win/DllLib.h"
#else
#include "Linux/SoLib.h"
#endif

/**
 * Used to create a platform-specific shared library handler.
 */
SharedLib* SharedLib::Instantiate(const char *filename)
{
	SharedLib* lib = NULL;
	
#ifdef _WIN32
	lib = SAFE_NEW DllLib(filename);
#else
	lib = SAFE_NEW SoLib(filename);
#endif
	
	if (lib == NULL || lib->LoadFailed()) {
		lib = NULL;
	}
	
	return lib;
}

/**
 * Used to create a platform-specific shared library handler.
 */
SharedLib *SharedLib::Instantiate(std::string filename)
{
	return Instantiate(filename.c_str());
}


const char *SharedLib::GetLibExtension() 
{
#ifdef WIN32
	return "dll";
#elif defined(__APPLE__)
	return "dylib";
#else
	return "so";
#endif
}

void SharedLib::reportError(const char* errorMsg, const char* fileName, int lineNumber, const char* function) {
	
/*
	const int MAX_MSG_LENGTH = 511;
	char s_msg[MAX_MSG_LENGTH + 1];
	SNPRINTF(s_msg, MAX_MSG_LENGTH, "%s:%d: %s: %s", fileName, lineNumber, function, errorMsg);
	handleerror(NULL, s_msg, "Shared Library Error", MBF_OK | MBF_EXCL);
*/
	logOutput.Print("%s:%d: %s: %s", fileName, lineNumber, function, errorMsg);
}

SharedLib::~SharedLib() {
    // subclasses will call Unload in thier destructors.
}
