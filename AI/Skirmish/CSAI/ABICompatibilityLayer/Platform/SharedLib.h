/**
 * @file SharedLib.h
 * @brief shared library loader
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Base shared library loading class definitiion
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef SHAREDLIB_H
#define SHAREDLIB_H

#include <string>

#ifndef _WIN32
/**
 * @brief WINAPI define
 *
 * On non-windows, defines the WINAPI modifier used in
 * Windows DLLs to empty, so it'll ignore them
 */
#define WINAPI
#endif

/**
 * @brief shared library base
 *
 * This is the abstract shared library class used for
 * polymorphic loading.  Platform-specifics should
 * derive from this.
 */
class SharedLib
{
public:
	/**
	 * @brief instantiate
	 * @param filename file name as a C string
	 * @return platform-specific shared library class
	 */
	static SharedLib *instantiate(const char *filename);

	/**
	 * @brief instantiate
	 * @param filename file name as a C++ string
	 * @return platform-specific shared library class
	 */
	static SharedLib *instantiate(std::string filename);

	/**
	 * @brief Find Address
	 * @param symbol function name (symbol) to locate
	 *
	 * Abstract so it must be implemented specifically by all platforms.
	 */
	virtual void *FindAddress(const char *symbol) = 0;

	virtual ~SharedLib();
};

#endif /* SHAREDLIB_H */
