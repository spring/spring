/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBSHIELD_H
#define _CPPWRAPPER_STUBSHIELD_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Shield.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubShield : public Shield {

protected:
	virtual ~StubShield();
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
	/**
	 * Amount of the resource used per shot or per second,
	 * depending on the type of projectile.
	 */
	// @Override
	virtual float GetResourceUse(Resource* resource);
	/**
	 * Size of shield covered area
	 */
private:
	float radius;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRadius(float radius);
	// @Override
	virtual float GetRadius();
	/**
	 * Shield acceleration on plasma stuff.
	 * How much will plasma be accelerated into the other direction
	 * when it hits the shield.
	 */
private:
	float force;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetForce(float force);
	// @Override
	virtual float GetForce();
	/**
	 * Maximum speed to which the shield can repulse plasma.
	 */
private:
	float maxSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxSpeed(float maxSpeed);
	// @Override
	virtual float GetMaxSpeed();
	/**
	 * Amount of damage the shield can reflect. (0=infinite)
	 */
private:
	float power;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPower(float power);
	// @Override
	virtual float GetPower();
	/**
	 * Amount of power that is regenerated per second.
	 */
private:
	float powerRegen;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPowerRegen(float powerRegen);
	// @Override
	virtual float GetPowerRegen();
	/**
	 * How much of a given resource is needed to regenerate power
	 * with max speed per second.
	 */
	// @Override
	virtual float GetPowerRegenResource(Resource* resource);
	/**
	 * How much power the shield has when it is created.
	 */
private:
	float startingPower;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStartingPower(float startingPower);
	// @Override
	virtual float GetStartingPower();
	/**
	 * Number of frames to delay recharging by after each hit.
	 */
private:
	int rechargeDelay;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRechargeDelay(int rechargeDelay);
	// @Override
	virtual int GetRechargeDelay();
	/**
	 * The color of the shield when it is at full power.
	 */
private:
	springai::AIColor goodColor;/* = springai::AIColor::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGoodColor(springai::AIColor goodColor);
	// @Override
	virtual springai::AIColor GetGoodColor();
	/**
	 * The color of the shield when it is empty.
	 */
private:
	springai::AIColor badColor;/* = springai::AIColor::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBadColor(springai::AIColor badColor);
	// @Override
	virtual springai::AIColor GetBadColor();
	/**
	 * The shields alpha value.
	 */
private:
	short alpha;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAlpha(short alpha);
	// @Override
	virtual short GetAlpha();
	/**
	 * The type of the shield (bitfield).
	 * Defines what weapons can be intercepted by the shield.
	 * 
	 * @see  getInterceptedByShieldType()
	 */
private:
	int interceptType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetInterceptType(int interceptType);
	// @Override
	virtual int GetInterceptType();
}; // class StubShield

}  // namespace springai

#endif // _CPPWRAPPER_STUBSHIELD_H

