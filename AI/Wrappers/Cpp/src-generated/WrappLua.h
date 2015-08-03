/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPLUA_H
#define _CPPWRAPPER_WRAPPLUA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Lua.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappLua : public Lua {

private:
	int skirmishAIId;

	WrappLua(int skirmishAIId);
	virtual ~WrappLua();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Lua* GetInstance(int skirmishAIId);

	/**
	 * @param inData  Can be set to NULL to skip passing in a string
	 * @param inSize  If this is less than 0, the data size is calculated using strlen()
	 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
	 */
public:
	// @Override
	virtual std::string CallRules(const char* inData, int inSize);

	/**
	 * @param inData  Can be set to NULL to skip passing in a string
	 * @param inSize  If this is less than 0, the data size is calculated using strlen()
	 * @param ret_outData  this is subject to Lua garbage collection, copy it if you wish to continue using it
	 */
public:
	// @Override
	virtual std::string CallUI(const char* inData, int inSize);
}; // class WrappLua

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPLUA_H

