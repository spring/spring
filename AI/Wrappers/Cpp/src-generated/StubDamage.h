/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBDAMAGE_H
#define _CPPWRAPPER_STUBDAMAGE_H

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
class StubDamage : public Damage {

protected:
	virtual ~StubDamage();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int weaponDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponDefId(int weaponDefId);
	// @Override
	virtual int GetWeaponDefId() const;
private:
	int paralyzeDamageTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetParalyzeDamageTime(int paralyzeDamageTime);
	// @Override
	virtual int GetParalyzeDamageTime();
private:
	float impulseFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetImpulseFactor(float impulseFactor);
	// @Override
	virtual float GetImpulseFactor();
private:
	float impulseBoost;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetImpulseBoost(float impulseBoost);
	// @Override
	virtual float GetImpulseBoost();
private:
	float craterMult;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCraterMult(float craterMult);
	// @Override
	virtual float GetCraterMult();
private:
	float craterBoost;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCraterBoost(float craterBoost);
	// @Override
	virtual float GetCraterBoost();
private:
	std::vector<float> types;/* = std::vector<float>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTypes(std::vector<float> types);
	// @Override
	virtual std::vector<float> GetTypes();
}; // class StubDamage

}  // namespace springai

#endif // _CPPWRAPPER_STUBDAMAGE_H

