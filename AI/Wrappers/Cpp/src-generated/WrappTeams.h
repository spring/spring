/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPTEAMS_H
#define _CPPWRAPPER_WRAPPTEAMS_H

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
class WrappTeams : public Teams {

private:
	int skirmishAIId;

	WrappTeams(int skirmishAIId);
	virtual ~WrappTeams();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Teams* GetInstance(int skirmishAIId);

	/**
	 * Returns the number of teams in this game
	 */
public:
	// @Override
	virtual int GetSize();
}; // class WrappTeams

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPTEAMS_H

