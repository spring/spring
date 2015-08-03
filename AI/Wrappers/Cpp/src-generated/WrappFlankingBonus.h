/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPFLANKINGBONUS_H
#define _CPPWRAPPER_WRAPPFLANKINGBONUS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "FlankingBonus.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappFlankingBonus : public FlankingBonus {

private:
	int skirmishAIId;
	int unitDefId;

	WrappFlankingBonus(int skirmishAIId, int unitDefId);
	virtual ~WrappFlankingBonus();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitDefId() const;
public:
	static FlankingBonus* GetInstance(int skirmishAIId, int unitDefId);

	/**
	 * The flanking bonus indicates how much additional damage you can inflict to
	 * a unit, if it gets attacked from different directions.
	 * See the spring source code if you want to know it more precisely.
	 * 
	 * @return  0: no flanking bonus
	 *          1: global coords, mobile
	 *          2: unit coords, mobile
	 *          3: unit coords, locked
	 */
public:
	// @Override
	virtual int GetMode();

	/**
	 * The unit takes less damage when attacked from this direction.
	 * This encourage flanking fire.
	 */
public:
	// @Override
	virtual springai::AIFloat3 GetDir();

	/**
	 * Damage factor for the least protected direction
	 */
public:
	// @Override
	virtual float GetMax();

	/**
	 * Damage factor for the most protected direction
	 */
public:
	// @Override
	virtual float GetMin();

	/**
	 * How much the ability of the flanking bonus direction to move builds up each
	 * frame.
	 */
public:
	// @Override
	virtual float GetMobilityAdd();
}; // class WrappFlankingBonus

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPFLANKINGBONUS_H

