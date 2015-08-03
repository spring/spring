/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_CHEATS_H
#define _CPPWRAPPER_CHEATS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Cheats {

public:
	virtual ~Cheats(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * Returns whether this AI may use active cheats.
	 */
public:
	virtual bool IsEnabled() = 0;

	/**
	 * Set whether this AI may use active cheats.
	 */
public:
	virtual bool SetEnabled(bool enable) = 0;

	/**
	 * Set whether this AI may receive cheat events.
	 * When enabled, you would for example get informed when enemy units are
	 * created, even without sensor coverage.
	 */
public:
	virtual bool SetEventsEnabled(bool enabled) = 0;

	/**
	 * Returns whether cheats will desync if used by an AI.
	 * @return always true, unless we are both the host and the only client.
	 */
public:
	virtual bool IsOnlyPassive() = 0;

	/**
	 * Allows one to give an income (dis-)advantage to the team
	 * controlled by the Skirmish AI.
	 * This value can also be set through the GameSetup script,
	 * with the difference that it causes an instant desync when set here.
	 * @param factor  default: 1.0; common: [0.0, 2.0]; valid: [0.0, FLOAT_MAX]
	 */
public:
	virtual void SetMyIncomeMultiplier(float factor) = 0;

	/**
	 * The AI team receives the specified amount of units of the specified resource.
	 */
public:
	virtual void GiveMeResource(Resource* resource, float amount) = 0;

	/**
	 * Creates a new unit with the selected name at pos,
	 * and returns its unit ID in ret_newUnitId.
	 */
public:
	virtual int GiveMeUnit(UnitDef* unitDef, const springai::AIFloat3& pos) = 0;

}; // class Cheats

}  // namespace springai

#endif // _CPPWRAPPER_CHEATS_H

