/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_SHIELD_H
#define _CPPWRAPPER_SHIELD_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Shield {

public:
	virtual ~Shield(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetWeaponDefId() const = 0;

	/**
	 * Amount of the resource used per shot or per second,
	 * depending on the type of projectile.
	 */
public:
	virtual float GetResourceUse(Resource* resource) = 0;

	/**
	 * Size of shield covered area
	 */
public:
	virtual float GetRadius() = 0;

	/**
	 * Shield acceleration on plasma stuff.
	 * How much will plasma be accelerated into the other direction
	 * when it hits the shield.
	 */
public:
	virtual float GetForce() = 0;

	/**
	 * Maximum speed to which the shield can repulse plasma.
	 */
public:
	virtual float GetMaxSpeed() = 0;

	/**
	 * Amount of damage the shield can reflect. (0=infinite)
	 */
public:
	virtual float GetPower() = 0;

	/**
	 * Amount of power that is regenerated per second.
	 */
public:
	virtual float GetPowerRegen() = 0;

	/**
	 * How much of a given resource is needed to regenerate power
	 * with max speed per second.
	 */
public:
	virtual float GetPowerRegenResource(Resource* resource) = 0;

	/**
	 * How much power the shield has when it is created.
	 */
public:
	virtual float GetStartingPower() = 0;

	/**
	 * Number of frames to delay recharging by after each hit.
	 */
public:
	virtual int GetRechargeDelay() = 0;

	/**
	 * The color of the shield when it is at full power.
	 */
public:
	virtual springai::AIColor GetGoodColor() = 0;

	/**
	 * The color of the shield when it is empty.
	 */
public:
	virtual springai::AIColor GetBadColor() = 0;

	/**
	 * The shields alpha value.
	 */
public:
	virtual short GetAlpha() = 0;

	/**
	 * The type of the shield (bitfield).
	 * Defines what weapons can be intercepted by the shield.
	 * 
	 * @see  getInterceptedByShieldType()
	 */
public:
	virtual int GetInterceptType() = 0;

}; // class Shield

}  // namespace springai

#endif // _CPPWRAPPER_SHIELD_H

