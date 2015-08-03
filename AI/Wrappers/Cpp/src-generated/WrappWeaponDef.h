/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPWEAPONDEF_H
#define _CPPWRAPPER_WRAPPWEAPONDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "WeaponDef.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappWeaponDef : public WeaponDef {

private:
	int skirmishAIId;
	int weaponDefId;

	WrappWeaponDef(int skirmishAIId, int weaponDefId);
	virtual ~WrappWeaponDef();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetWeaponDefId() const;
public:
	static WeaponDef* GetInstance(int skirmishAIId, int weaponDefId);

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual const char* GetType();

public:
	// @Override
	virtual const char* GetDescription();

public:
	// @Override
	virtual const char* GetFileName();

public:
	// @Override
	virtual const char* GetCegTag();

public:
	// @Override
	virtual float GetRange();

public:
	// @Override
	virtual float GetHeightMod();

	/**
	 * Inaccuracy of whole burst
	 */
public:
	// @Override
	virtual float GetAccuracy();

	/**
	 * Inaccuracy of individual shots inside burst
	 */
public:
	// @Override
	virtual float GetSprayAngle();

	/**
	 * Inaccuracy while owner moving
	 */
public:
	// @Override
	virtual float GetMovingAccuracy();

	/**
	 * Fraction of targets move speed that is used as error offset
	 */
public:
	// @Override
	virtual float GetTargetMoveError();

	/**
	 * Maximum distance the weapon will lead the target
	 */
public:
	// @Override
	virtual float GetLeadLimit();

	/**
	 * Factor for increasing the leadLimit with experience
	 */
public:
	// @Override
	virtual float GetLeadBonus();

	/**
	 * Replaces hardcoded behaviour for burnblow cannons
	 */
public:
	// @Override
	virtual float GetPredictBoost();

public:
	// @Override
	virtual int GetNumDamageTypes();

public:
	// @Override
	virtual float GetAreaOfEffect();

public:
	// @Override
	virtual bool IsNoSelfDamage();

public:
	// @Override
	virtual float GetFireStarter();

public:
	// @Override
	virtual float GetEdgeEffectiveness();

public:
	// @Override
	virtual float GetSize();

public:
	// @Override
	virtual float GetSizeGrowth();

public:
	// @Override
	virtual float GetCollisionSize();

public:
	// @Override
	virtual int GetSalvoSize();

public:
	// @Override
	virtual float GetSalvoDelay();

public:
	// @Override
	virtual float GetReload();

public:
	// @Override
	virtual float GetBeamTime();

public:
	// @Override
	virtual bool IsBeamBurst();

public:
	// @Override
	virtual bool IsWaterBounce();

public:
	// @Override
	virtual bool IsGroundBounce();

public:
	// @Override
	virtual float GetBounceRebound();

public:
	// @Override
	virtual float GetBounceSlip();

public:
	// @Override
	virtual int GetNumBounce();

public:
	// @Override
	virtual float GetMaxAngle();

public:
	// @Override
	virtual float GetUpTime();

public:
	// @Override
	virtual int GetFlightTime();

public:
	// @Override
	virtual float GetCost(Resource* resource);

public:
	// @Override
	virtual int GetProjectilesPerShot();

public:
	// @Override
	virtual bool IsTurret();

public:
	// @Override
	virtual bool IsOnlyForward();

public:
	// @Override
	virtual bool IsFixedLauncher();

public:
	// @Override
	virtual bool IsWaterWeapon();

public:
	// @Override
	virtual bool IsFireSubmersed();

	/**
	 * Lets a torpedo travel above water like it does below water
	 */
public:
	// @Override
	virtual bool IsSubMissile();

public:
	// @Override
	virtual bool IsTracks();

public:
	// @Override
	virtual bool IsDropped();

	/**
	 * The weapon will only paralyze, not do real damage.
	 */
public:
	// @Override
	virtual bool IsParalyzer();

	/**
	 * The weapon damages by impacting, not by exploding.
	 */
public:
	// @Override
	virtual bool IsImpactOnly();

	/**
	 * Can not target anything (for example: anti-nuke, D-Gun)
	 */
public:
	// @Override
	virtual bool IsNoAutoTarget();

	/**
	 * Has to be fired manually (by the player or an AI, example: D-Gun)
	 */
public:
	// @Override
	virtual bool IsManualFire();

	/**
	 * Can intercept targetable weapons shots.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getTargetable()
	 */
public:
	// @Override
	virtual int GetInterceptor();

	/**
	 * Shoots interceptable projectiles.
	 * Shots can be intercepted by interceptors.
	 * 
	 * example: nuke
	 * 
	 * @see  getInterceptor()
	 */
public:
	// @Override
	virtual int GetTargetable();

public:
	// @Override
	virtual bool IsStockpileable();

	/**
	 * Range of interceptors.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getInterceptor()
	 */
public:
	// @Override
	virtual float GetCoverageRange();

	/**
	 * Build time of a missile
	 */
public:
	// @Override
	virtual float GetStockpileTime();

public:
	// @Override
	virtual float GetIntensity();

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	// @Override
	virtual float GetThickness();

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	// @Override
	virtual float GetLaserFlareSize();

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	// @Override
	virtual float GetCoreThickness();

public:
	// @Override
	virtual float GetDuration();

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	// @Override
	virtual int GetLodDistance();

public:
	// @Override
	virtual float GetFalloffRate();

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	// @Override
	virtual int GetGraphicsType();

public:
	// @Override
	virtual bool IsSoundTrigger();

public:
	// @Override
	virtual bool IsSelfExplode();

public:
	// @Override
	virtual bool IsGravityAffected();

	/**
	 * Per weapon high trajectory setting.
	 * UnitDef also has this property.
	 * 
	 * @return  0: low
	 *          1: high
	 *          2: unit
	 */
public:
	// @Override
	virtual int GetHighTrajectory();

public:
	// @Override
	virtual float GetMyGravity();

public:
	// @Override
	virtual bool IsNoExplode();

public:
	// @Override
	virtual float GetStartVelocity();

public:
	// @Override
	virtual float GetWeaponAcceleration();

public:
	// @Override
	virtual float GetTurnRate();

public:
	// @Override
	virtual float GetMaxVelocity();

public:
	// @Override
	virtual float GetProjectileSpeed();

public:
	// @Override
	virtual float GetExplosionSpeed();

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	// @Override
	virtual int GetOnlyTargetCategory();

	/**
	 * How much the missile will wobble around its course.
	 */
public:
	// @Override
	virtual float GetWobble();

	/**
	 * How much the missile will dance.
	 */
public:
	// @Override
	virtual float GetDance();

	/**
	 * How high trajectory missiles will try to fly in.
	 */
public:
	// @Override
	virtual float GetTrajectoryHeight();

public:
	// @Override
	virtual bool IsLargeBeamLaser();

	/**
	 * If the weapon is a shield rather than a weapon.
	 */
public:
	// @Override
	virtual bool IsShield();

	/**
	 * If the weapon should be repulsed or absorbed.
	 */
public:
	// @Override
	virtual bool IsShieldRepulser();

	/**
	 * If the shield only affects enemy projectiles.
	 */
public:
	// @Override
	virtual bool IsSmartShield();

	/**
	 * If the shield only affects stuff coming from outside shield radius.
	 */
public:
	// @Override
	virtual bool IsExteriorShield();

	/**
	 * If the shield should be graphically shown.
	 */
public:
	// @Override
	virtual bool IsVisibleShield();

	/**
	 * If a small graphic should be shown at each repulse.
	 */
public:
	// @Override
	virtual bool IsVisibleShieldRepulse();

	/**
	 * The number of frames to draw the shield after it has been hit.
	 */
public:
	// @Override
	virtual int GetVisibleShieldHitFrames();

	/**
	 * The type of shields that can intercept this weapon (bitfield).
	 * The weapon can be affected by shields if:
	 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
	 * 
	 * @see  getInterceptType()
	 */
public:
	// @Override
	virtual int GetInterceptedByShieldType();

	/**
	 * Tries to avoid friendly units while aiming?
	 */
public:
	// @Override
	virtual bool IsAvoidFriendly();

	/**
	 * Tries to avoid features while aiming?
	 */
public:
	// @Override
	virtual bool IsAvoidFeature();

	/**
	 * Tries to avoid neutral units while aiming?
	 */
public:
	// @Override
	virtual bool IsAvoidNeutral();

	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
public:
	// @Override
	virtual float GetTargetBorder();

	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
public:
	// @Override
	virtual float GetCylinderTargetting();

	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
public:
	// @Override
	virtual float GetMinIntensity();

	/**
	 * Controls cannon range height boost.
	 * 
	 * default: -1: automatically calculate a more or less sane value
	 */
public:
	// @Override
	virtual float GetHeightBoostFactor();

	/**
	 * Multiplier for the distance to the target for priority calculations.
	 */
public:
	// @Override
	virtual float GetProximityPriority();

public:
	// @Override
	virtual int GetCollisionFlags();

public:
	// @Override
	virtual bool IsSweepFire();

public:
	// @Override
	virtual bool IsAbleToAttackGround();

public:
	// @Override
	virtual float GetCameraShake();

public:
	// @Override
	virtual float GetDynDamageExp();

public:
	// @Override
	virtual float GetDynDamageMin();

public:
	// @Override
	virtual float GetDynDamageRange();

public:
	// @Override
	virtual bool IsDynDamageInverted();

public:
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();

public:
	// @Override
	virtual springai::Damage* GetDamage();

public:
	// @Override
	virtual springai::Shield* GetShield();
}; // class WrappWeaponDef

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPWEAPONDEF_H

