/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPDAMAGE_H
#define _CPPWRAPPER_WRAPPDAMAGE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Damage.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappDamage : public Damage {

private:
	int skirmishAIId;
	int weaponDefId;

	WrappDamage(int skirmishAIId, int weaponDefId);
	virtual ~WrappDamage();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetWeaponDefId() const;
public:
	static Damage* GetInstance(int skirmishAIId, int weaponDefId);

public:
	// @Override
	virtual int GetParalyzeDamageTime();

public:
	// @Override
	virtual float GetImpulseFactor();

public:
	// @Override
	virtual float GetImpulseBoost();

public:
	// @Override
	virtual float GetCraterMult();

public:
	// @Override
	virtual float GetCraterBoost();

public:
	// @Override
	virtual std::vector<float> GetTypes();
}; // class WrappDamage

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPDAMAGE_H

