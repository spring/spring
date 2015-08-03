/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBFLANKINGBONUS_H
#define _CPPWRAPPER_STUBFLANKINGBONUS_H

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
class StubFlankingBonus : public FlankingBonus {

protected:
	virtual ~StubFlankingBonus();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitDefId(int unitDefId);
	// @Override
	virtual int GetUnitDefId() const;
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
private:
	int mode;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMode(int mode);
	// @Override
	virtual int GetMode();
	/**
	 * The unit takes less damage when attacked from this direction.
	 * This encourage flanking fire.
	 */
private:
	springai::AIFloat3 dir;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDir(springai::AIFloat3 dir);
	// @Override
	virtual springai::AIFloat3 GetDir();
	/**
	 * Damage factor for the least protected direction
	 */
private:
	float max;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMax(float max);
	// @Override
	virtual float GetMax();
	/**
	 * Damage factor for the most protected direction
	 */
private:
	float min;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMin(float min);
	// @Override
	virtual float GetMin();
	/**
	 * How much the ability of the flanking bonus direction to move builds up each
	 * frame.
	 */
private:
	float mobilityAdd;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMobilityAdd(float mobilityAdd);
	// @Override
	virtual float GetMobilityAdd();
}; // class StubFlankingBonus

}  // namespace springai

#endif // _CPPWRAPPER_STUBFLANKINGBONUS_H

