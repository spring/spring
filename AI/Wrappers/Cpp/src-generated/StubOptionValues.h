/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBOPTIONVALUES_H
#define _CPPWRAPPER_STUBOPTIONVALUES_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "OptionValues.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubOptionValues : public OptionValues {

protected:
	virtual ~StubOptionValues();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the number of option key-value pairs in the options map
	 * for this Skirmish AI.
	 */
private:
	int size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(int size);
	// @Override
	virtual int GetSize();
	/**
	 * Returns the key at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	// @Override
	virtual const char* GetKey(int optionIndex);
	/**
	 * Returns the value at index optionIndex in the options map
	 * for this Skirmish AI, or NULL if the optionIndex is invalid.
	 */
	// @Override
	virtual const char* GetValue(int optionIndex);
	/**
	 * Returns the value associated with the given key in the options map
	 * for this Skirmish AI, or NULL if not found.
	 */
	// @Override
	virtual const char* GetValueByKey(const char* const key);
}; // class StubOptionValues

}  // namespace springai

#endif // _CPPWRAPPER_STUBOPTIONVALUES_H

