/**
 * @file DllLib.h
 * @brief Windows shared library loader
 * @author Christopher Han <xiphux@gmail.com>
 * 
 * Windows Shared Object loader class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#ifndef DLLLIB_H
#define DLLLIB_H

#include "Platform/SharedLib.h"
#include <windows.h>

/**
 * @brief Win32 DLL base
 *
 * This class loads Win32 DLLs.
 * Derived from the abstract SharedLib.
 */
class DllLib: public SharedLib
{
public:
	/**
	 * @brief Constructor
	 * @param fileName DLL to load
	 */
	DllLib(const char* fileName);

	/**
	 * Does a FreeLibrary on the given DLL
	 * @brief unload
	 */
	virtual void Unload();

	virtual bool LoadFailed();

	/**
	 * @brief Destructor
	 */
	~DllLib();

	/**
	 * @brief Find address
	 * @param symbol function to locate
	 * @return void pointer to the function if found, NULL otherwise
	 */
	virtual void* FindAddress(const char* symbol);

private:
	/**
	 * @brief dll pointer
	 *
	 * HINSTANCE used to point to the currently opened DLL.
	 */
	HINSTANCE dll;
};

#endif // DLLLIB_H
