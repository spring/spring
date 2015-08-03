/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_DAMAGE_H
#define _CPPWRAPPER_DAMAGE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Damage {

public:
	virtual ~Damage(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetWeaponDefId() const = 0;

public:
	virtual int GetParalyzeDamageTime() = 0;

public:
	virtual float GetImpulseFactor() = 0;

public:
	virtual float GetImpulseBoost() = 0;

public:
	virtual float GetCraterMult() = 0;

public:
	virtual float GetCraterBoost() = 0;

public:
	virtual std::vector<float> GetTypes() = 0;

}; // class Damage

}  // namespace springai

#endif // _CPPWRAPPER_DAMAGE_H

