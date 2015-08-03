/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPSKIRMISHAIS_H
#define _CPPWRAPPER_WRAPPSKIRMISHAIS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "SkirmishAIs.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappSkirmishAIs : public SkirmishAIs {

private:
	int skirmishAIId;

	WrappSkirmishAIs(int skirmishAIId);
	virtual ~WrappSkirmishAIs();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static SkirmishAIs* GetInstance(int skirmishAIId);

	/**
	 * Returns the number of skirmish AIs in this game
	 */
public:
	// @Override
	virtual int GetSize();

	/**
	 * Returns the maximum number of skirmish AIs in any game
	 */
public:
	// @Override
	virtual int GetMax();
}; // class WrappSkirmishAIs

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPSKIRMISHAIS_H

