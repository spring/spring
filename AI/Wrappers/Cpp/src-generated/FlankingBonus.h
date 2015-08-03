/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_FLANKINGBONUS_H
#define _CPPWRAPPER_FLANKINGBONUS_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class FlankingBonus {

public:
	virtual ~FlankingBonus(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitDefId() const = 0;

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
	virtual int GetMode() = 0;

	/**
	 * The unit takes less damage when attacked from this direction.
	 * This encourage flanking fire.
	 */
public:
	virtual springai::AIFloat3 GetDir() = 0;

	/**
	 * Damage factor for the least protected direction
	 */
public:
	virtual float GetMax() = 0;

	/**
	 * Damage factor for the most protected direction
	 */
public:
	virtual float GetMin() = 0;

	/**
	 * How much the ability of the flanking bonus direction to move builds up each
	 * frame.
	 */
public:
	virtual float GetMobilityAdd() = 0;

}; // class FlankingBonus

}  // namespace springai

#endif // _CPPWRAPPER_FLANKINGBONUS_H

