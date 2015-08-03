/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPSHIELD_H
#define _CPPWRAPPER_WRAPPSHIELD_H

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
class WrappShield : public Shield {

private:
	int skirmishAIId;
	int weaponDefId;

	WrappShield(int skirmishAIId, int weaponDefId);
	virtual ~WrappShield();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetWeaponDefId() const;
public:
	static Shield* GetInstance(int skirmishAIId, int weaponDefId);

	/**
	 * Amount of the resource used per shot or per second,
	 * depending on the type of projectile.
	 */
public:
	// @Override
	virtual float GetResourceUse(Resource* resource);

	/**
	 * Size of shield covered area
	 */
public:
	// @Override
	virtual float GetRadius();

	/**
	 * Shield acceleration on plasma stuff.
	 * How much will plasma be accelerated into the other direction
	 * when it hits the shield.
	 */
public:
	// @Override
	virtual float GetForce();

	/**
	 * Maximum speed to which the shield can repulse plasma.
	 */
public:
	// @Override
	virtual float GetMaxSpeed();

	/**
	 * Amount of damage the shield can reflect. (0=infinite)
	 */
public:
	// @Override
	virtual float GetPower();

	/**
	 * Amount of power that is regenerated per second.
	 */
public:
	// @Override
	virtual float GetPowerRegen();

	/**
	 * How much of a given resource is needed to regenerate power
	 * with max speed per second.
	 */
public:
	// @Override
	virtual float GetPowerRegenResource(Resource* resource);

	/**
	 * How much power the shield has when it is created.
	 */
public:
	// @Override
	virtual float GetStartingPower();

	/**
	 * Number of frames to delay recharging by after each hit.
	 */
public:
	// @Override
	virtual int GetRechargeDelay();

	/**
	 * The color of the shield when it is at full power.
	 */
public:
	// @Override
	virtual springai::AIColor GetGoodColor();

	/**
	 * The color of the shield when it is empty.
	 */
public:
	// @Override
	virtual springai::AIColor GetBadColor();

	/**
	 * The shields alpha value.
	 */
public:
	// @Override
	virtual short GetAlpha();

	/**
	 * The type of the shield (bitfield).
	 * Defines what weapons can be intercepted by the shield.
	 * 
	 * @see  getInterceptedByShieldType()
	 */
public:
	// @Override
	virtual int GetInterceptType();
}; // class WrappShield

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPSHIELD_H

