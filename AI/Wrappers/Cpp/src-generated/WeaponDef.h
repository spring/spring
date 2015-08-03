/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WEAPONDEF_H
#define _CPPWRAPPER_WEAPONDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WeaponDef {

public:
	virtual ~WeaponDef(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetWeaponDefId() const = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual const char* GetType() = 0;

public:
	virtual const char* GetDescription() = 0;

public:
	virtual const char* GetFileName() = 0;

public:
	virtual const char* GetCegTag() = 0;

public:
	virtual float GetRange() = 0;

public:
	virtual float GetHeightMod() = 0;

	/**
	 * Inaccuracy of whole burst
	 */
public:
	virtual float GetAccuracy() = 0;

	/**
	 * Inaccuracy of individual shots inside burst
	 */
public:
	virtual float GetSprayAngle() = 0;

	/**
	 * Inaccuracy while owner moving
	 */
public:
	virtual float GetMovingAccuracy() = 0;

	/**
	 * Fraction of targets move speed that is used as error offset
	 */
public:
	virtual float GetTargetMoveError() = 0;

	/**
	 * Maximum distance the weapon will lead the target
	 */
public:
	virtual float GetLeadLimit() = 0;

	/**
	 * Factor for increasing the leadLimit with experience
	 */
public:
	virtual float GetLeadBonus() = 0;

	/**
	 * Replaces hardcoded behaviour for burnblow cannons
	 */
public:
	virtual float GetPredictBoost() = 0;

public:
	virtual int GetNumDamageTypes() = 0;

public:
	virtual float GetAreaOfEffect() = 0;

public:
	virtual bool IsNoSelfDamage() = 0;

public:
	virtual float GetFireStarter() = 0;

public:
	virtual float GetEdgeEffectiveness() = 0;

public:
	virtual float GetSize() = 0;

public:
	virtual float GetSizeGrowth() = 0;

public:
	virtual float GetCollisionSize() = 0;

public:
	virtual int GetSalvoSize() = 0;

public:
	virtual float GetSalvoDelay() = 0;

public:
	virtual float GetReload() = 0;

public:
	virtual float GetBeamTime() = 0;

public:
	virtual bool IsBeamBurst() = 0;

public:
	virtual bool IsWaterBounce() = 0;

public:
	virtual bool IsGroundBounce() = 0;

public:
	virtual float GetBounceRebound() = 0;

public:
	virtual float GetBounceSlip() = 0;

public:
	virtual int GetNumBounce() = 0;

public:
	virtual float GetMaxAngle() = 0;

public:
	virtual float GetUpTime() = 0;

public:
	virtual int GetFlightTime() = 0;

public:
	virtual float GetCost(Resource* resource) = 0;

public:
	virtual int GetProjectilesPerShot() = 0;

public:
	virtual bool IsTurret() = 0;

public:
	virtual bool IsOnlyForward() = 0;

public:
	virtual bool IsFixedLauncher() = 0;

public:
	virtual bool IsWaterWeapon() = 0;

public:
	virtual bool IsFireSubmersed() = 0;

	/**
	 * Lets a torpedo travel above water like it does below water
	 */
public:
	virtual bool IsSubMissile() = 0;

public:
	virtual bool IsTracks() = 0;

public:
	virtual bool IsDropped() = 0;

	/**
	 * The weapon will only paralyze, not do real damage.
	 */
public:
	virtual bool IsParalyzer() = 0;

	/**
	 * The weapon damages by impacting, not by exploding.
	 */
public:
	virtual bool IsImpactOnly() = 0;

	/**
	 * Can not target anything (for example: anti-nuke, D-Gun)
	 */
public:
	virtual bool IsNoAutoTarget() = 0;

	/**
	 * Has to be fired manually (by the player or an AI, example: D-Gun)
	 */
public:
	virtual bool IsManualFire() = 0;

	/**
	 * Can intercept targetable weapons shots.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getTargetable()
	 */
public:
	virtual int GetInterceptor() = 0;

	/**
	 * Shoots interceptable projectiles.
	 * Shots can be intercepted by interceptors.
	 * 
	 * example: nuke
	 * 
	 * @see  getInterceptor()
	 */
public:
	virtual int GetTargetable() = 0;

public:
	virtual bool IsStockpileable() = 0;

	/**
	 * Range of interceptors.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getInterceptor()
	 */
public:
	virtual float GetCoverageRange() = 0;

	/**
	 * Build time of a missile
	 */
public:
	virtual float GetStockpileTime() = 0;

public:
	virtual float GetIntensity() = 0;

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	virtual float GetThickness() = 0;

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	virtual float GetLaserFlareSize() = 0;

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	virtual float GetCoreThickness() = 0;

public:
	virtual float GetDuration() = 0;

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	virtual int GetLodDistance() = 0;

public:
	virtual float GetFalloffRate() = 0;

	/**
	 * @deprecated only relevant for visualization
	 */
public:
	virtual int GetGraphicsType() = 0;

public:
	virtual bool IsSoundTrigger() = 0;

public:
	virtual bool IsSelfExplode() = 0;

public:
	virtual bool IsGravityAffected() = 0;

	/**
	 * Per weapon high trajectory setting.
	 * UnitDef also has this property.
	 * 
	 * @return  0: low
	 *          1: high
	 *          2: unit
	 */
public:
	virtual int GetHighTrajectory() = 0;

public:
	virtual float GetMyGravity() = 0;

public:
	virtual bool IsNoExplode() = 0;

public:
	virtual float GetStartVelocity() = 0;

public:
	virtual float GetWeaponAcceleration() = 0;

public:
	virtual float GetTurnRate() = 0;

public:
	virtual float GetMaxVelocity() = 0;

public:
	virtual float GetProjectileSpeed() = 0;

public:
	virtual float GetExplosionSpeed() = 0;

	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
public:
	virtual int GetOnlyTargetCategory() = 0;

	/**
	 * How much the missile will wobble around its course.
	 */
public:
	virtual float GetWobble() = 0;

	/**
	 * How much the missile will dance.
	 */
public:
	virtual float GetDance() = 0;

	/**
	 * How high trajectory missiles will try to fly in.
	 */
public:
	virtual float GetTrajectoryHeight() = 0;

public:
	virtual bool IsLargeBeamLaser() = 0;

	/**
	 * If the weapon is a shield rather than a weapon.
	 */
public:
	virtual bool IsShield() = 0;

	/**
	 * If the weapon should be repulsed or absorbed.
	 */
public:
	virtual bool IsShieldRepulser() = 0;

	/**
	 * If the shield only affects enemy projectiles.
	 */
public:
	virtual bool IsSmartShield() = 0;

	/**
	 * If the shield only affects stuff coming from outside shield radius.
	 */
public:
	virtual bool IsExteriorShield() = 0;

	/**
	 * If the shield should be graphically shown.
	 */
public:
	virtual bool IsVisibleShield() = 0;

	/**
	 * If a small graphic should be shown at each repulse.
	 */
public:
	virtual bool IsVisibleShieldRepulse() = 0;

	/**
	 * The number of frames to draw the shield after it has been hit.
	 */
public:
	virtual int GetVisibleShieldHitFrames() = 0;

	/**
	 * The type of shields that can intercept this weapon (bitfield).
	 * The weapon can be affected by shields if:
	 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
	 * 
	 * @see  getInterceptType()
	 */
public:
	virtual int GetInterceptedByShieldType() = 0;

	/**
	 * Tries to avoid friendly units while aiming?
	 */
public:
	virtual bool IsAvoidFriendly() = 0;

	/**
	 * Tries to avoid features while aiming?
	 */
public:
	virtual bool IsAvoidFeature() = 0;

	/**
	 * Tries to avoid neutral units while aiming?
	 */
public:
	virtual bool IsAvoidNeutral() = 0;

	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
public:
	virtual float GetTargetBorder() = 0;

	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
public:
	virtual float GetCylinderTargetting() = 0;

	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
public:
	virtual float GetMinIntensity() = 0;

	/**
	 * Controls cannon range height boost.
	 * 
	 * default: -1: automatically calculate a more or less sane value
	 */
public:
	virtual float GetHeightBoostFactor() = 0;

	/**
	 * Multiplier for the distance to the target for priority calculations.
	 */
public:
	virtual float GetProximityPriority() = 0;

public:
	virtual int GetCollisionFlags() = 0;

public:
	virtual bool IsSweepFire() = 0;

public:
	virtual bool IsAbleToAttackGround() = 0;

public:
	virtual float GetCameraShake() = 0;

public:
	virtual float GetDynDamageExp() = 0;

public:
	virtual float GetDynDamageMin() = 0;

public:
	virtual float GetDynDamageRange() = 0;

public:
	virtual bool IsDynDamageInverted() = 0;

public:
	virtual std::map<std::string,std::string> GetCustomParams() = 0;

public:
	virtual springai::Damage* GetDamage() = 0;

public:
	virtual springai::Shield* GetShield() = 0;

}; // class WeaponDef

}  // namespace springai

#endif // _CPPWRAPPER_WEAPONDEF_H

