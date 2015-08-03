/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPCHEATS_H
#define _CPPWRAPPER_WRAPPCHEATS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Cheats.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappCheats : public Cheats {

private:
	int skirmishAIId;

	WrappCheats(int skirmishAIId);
	virtual ~WrappCheats();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Cheats* GetInstance(int skirmishAIId);

	/**
	 * Returns whether this AI may use active cheats.
	 */
public:
	// @Override
	virtual bool IsEnabled();

	/**
	 * Set whether this AI may use active cheats.
	 */
public:
	// @Override
	virtual bool SetEnabled(bool enable);

	/**
	 * Set whether this AI may receive cheat events.
	 * When enabled, you would for example get informed when enemy units are
	 * created, even without sensor coverage.
	 */
public:
	// @Override
	virtual bool SetEventsEnabled(bool enabled);

	/**
	 * Returns whether cheats will desync if used by an AI.
	 * @return always true, unless we are both the host and the only client.
	 */
public:
	// @Override
	virtual bool IsOnlyPassive();

	/**
	 * Allows one to give an income (dis-)advantage to the team
	 * controlled by the Skirmish AI.
	 * This value can also be set through the GameSetup script,
	 * with the difference that it causes an instant desync when set here.
	 * @param factor  default: 1.0; common: [0.0, 2.0]; valid: [0.0, FLOAT_MAX]
	 */
public:
	// @Override
	virtual void SetMyIncomeMultiplier(float factor);

	/**
	 * The AI team receives the specified amount of units of the specified resource.
	 */
public:
	// @Override
	virtual void GiveMeResource(Resource* resource, float amount);

	/**
	 * Creates a new unit with the selected name at pos,
	 * and returns its unit ID in ret_newUnitId.
	 */
public:
	// @Override
	virtual int GiveMeUnit(UnitDef* unitDef, const springai::AIFloat3& pos);
}; // class WrappCheats

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPCHEATS_H

