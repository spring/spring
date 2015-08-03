/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBINFO_H
#define _CPPWRAPPER_STUBINFO_H

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
class StubInfo : public Info {

protected:
	virtual ~StubInfo();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the number of info key-value pairs in the info map
	 * for this Skirmish AI.
	 */
private:
	int size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(int size);
	// @Override
	virtual int GetSize();
	/**
	 * Returns the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	// @Override
	virtual const char* GetKey(int infoIndex);
	/**
	 * Returns the value at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	// @Override
	virtual const char* GetValue(int infoIndex);
	/**
	 * Returns the description of the key at index infoIndex in the info map
	 * for this Skirmish AI, or NULL if the infoIndex is invalid.
	 */
	// @Override
	virtual const char* GetDescription(int infoIndex);
	/**
	 * Returns the value associated with the given key in the info map
	 * for this Skirmish AI, or NULL if not found.
	 */
	// @Override
	virtual const char* GetValueByKey(const char* const key);
}; // class StubInfo

}  // namespace springai

#endif // _CPPWRAPPER_STUBINFO_H

