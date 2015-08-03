/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBSKIRMISHAIS_H
#define _CPPWRAPPER_STUBSKIRMISHAIS_H

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
class StubSkirmishAIs : public SkirmishAIs {

protected:
	virtual ~StubSkirmishAIs();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the number of skirmish AIs in this game
	 */
private:
	int size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(int size);
	// @Override
	virtual int GetSize();
	/**
	 * Returns the maximum number of skirmish AIs in any game
	 */
private:
	int max;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMax(int max);
	// @Override
	virtual int GetMax();
}; // class StubSkirmishAIs

}  // namespace springai

#endif // _CPPWRAPPER_STUBSKIRMISHAIS_H

