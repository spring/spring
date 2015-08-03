/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBTEAMS_H
#define _CPPWRAPPER_STUBTEAMS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Teams.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubTeams : public Teams {

protected:
	virtual ~StubTeams();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the number of teams in this game
	 */
private:
	int size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(int size);
	// @Override
	virtual int GetSize();
}; // class StubTeams

}  // namespace springai

#endif // _CPPWRAPPER_STUBTEAMS_H

