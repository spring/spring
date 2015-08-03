/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_LUA_H
#define _CPPWRAPPER_LUA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Lua {

public:
	virtual ~Lua(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * @param inData  Can be set to NULL to skip passing in a string
	 * @param inSize  If this is less than 0, the data size is calculated using strlen()
	 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
	 */
public:
	virtual std::string CallRules(const char* inData, int inSize) = 0;

	/**
	 * @param inData  Can be set to NULL to skip passing in a string
	 * @param inSize  If this is less than 0, the data size is calculated using strlen()
	 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
	 */
public:
	virtual std::string CallUI(const char* inData, int inSize) = 0;

}; // class Lua

}  // namespace springai

#endif // _CPPWRAPPER_LUA_H

