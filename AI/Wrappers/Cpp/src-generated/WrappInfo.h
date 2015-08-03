/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPINFO_H
#define _CPPWRAPPER_WRAPPINFO_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Info.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappInfo : public Info {

private:
	int skirmishAIId;

	WrappInfo(int skirmishAIId);
	virtual ~WrappInfo();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Info* GetInstance(int skirmishAIId);

	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
public:
	// @Override
	virtual int GetSize();

	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	// @Override
	virtual const char* GetKey(int infoIndex);

	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	// @Override
	virtual const char* GetValue(int infoIndex);

	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
public:
	// @Override
	virtual const char* GetDescription(int infoIndex);

	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
public:
	// @Override
	virtual const char* GetValueByKey(const char* const key);
}; // class WrappInfo

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPINFO_H

