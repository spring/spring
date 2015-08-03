/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_INFO_H
#define _CPPWRAPPER_INFO_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Info {

public:
	virtual ~Info(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
public:
	virtual int GetSize() = 0;

	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	virtual const char* GetKey(int infoIndex) = 0;

	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	virtual const char* GetValue(int infoIndex) = 0;

	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	virtual const char* GetDescription(int infoIndex) = 0;

	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
public:
	virtual const char* GetValueByKey(const char* const key) = 0;

}; // class Info

}  // namespace springai

#endif // _CPPWRAPPER_INFO_H

