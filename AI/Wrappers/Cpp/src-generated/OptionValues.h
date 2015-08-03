/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_OPTIONVALUES_H
#define _CPPWRAPPER_OPTIONVALUES_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class OptionValues {

public:
	virtual ~OptionValues(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns the number of option key-value pairs in the options map
	 * for this Skirmish AI.
	 */
public:
	virtual int GetSize() = 0;

	/**
	 * Returns the key at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
public:
	virtual const char* GetKey(int optionIndex) = 0;

	/**
	 * Returns the value at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
public:
	virtual const char* GetValue(int optionIndex) = 0;

	/**
	 * Returns the value associated with the given key in the options map
	 * for this Skirmish AI, or NULL if not found.
	 */
public:
	virtual const char* GetValueByKey(const char* const key) = 0;

}; // class OptionValues

}  // namespace springai

#endif // _CPPWRAPPER_OPTIONVALUES_H

