/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DLLLIB_H
#define DLLLIB_H

#ifdef WIN32

#include "System/Platform/SharedLib.h"
#include <windows.h>

/**
 * @brief Windows shared library loader
 *
 * Windows Shared Object loader class definition.
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

#endif

#endif // DLLLIB_H
