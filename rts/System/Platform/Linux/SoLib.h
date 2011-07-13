/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOLIB_H
#define SOLIB_H

#include "System/Platform/SharedLib.h"

/**
 * @brief Linux shared library base
 *
 * This class loads *nix-type *.so shared objects.
 * Derived from the abstract SharedLib.
 */
class SoLib: public SharedLib
{
public:
	/**
	 * @brief Constructor
	 * @param fileName shared object to load
	 */
	SoLib(const char* fileName);

	/**
	 * Just dlcloses the shared object
	 * @brief unload
	 */
	virtual void Unload();

	virtual bool LoadFailed();

	/**
	 * @brief Destructor
	 */
	~SoLib();

	/**
	 * @brief Find address
	 * @param symbol Function (symbol) to locate
	 * @return void pointer to the function if found, NULL otherwise
	 */
	virtual void* FindAddress(const char* symbol);

private:
	/**
	 * @brief so pointer
	 * Stores the loaded shared object as a void pointer
	 * (as per the standard convention)
	 */
	void* so;
};

#endif // SOLIB_H
