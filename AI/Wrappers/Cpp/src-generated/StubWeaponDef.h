/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBWEAPONDEF_H
#define _CPPWRAPPER_STUBWEAPONDEF_H

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
class StubWeaponDef : public WeaponDef {

protected:
	virtual ~StubWeaponDef();
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
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	const char* type;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetType(const char* type);
	// @Override
	virtual const char* GetType();
private:
	const char* description;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDescription(const char* description);
	// @Override
	virtual const char* GetDescription();
private:
	const char* fileName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFileName(const char* fileName);
	// @Override
	virtual const char* GetFileName();
private:
	const char* cegTag;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCegTag(const char* cegTag);
	// @Override
	virtual const char* GetCegTag();
private:
	float range;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetRange(float range);
	// @Override
	virtual float GetRange();
private:
	float heightMod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHeightMod(float heightMod);
	// @Override
	virtual float GetHeightMod();
	/**
	 * Inaccuracy of whole burst
	 */
private:
	float accuracy;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAccuracy(float accuracy);
	// @Override
	virtual float GetAccuracy();
	/**
	 * Inaccuracy of individual shots inside burst
	 */
private:
	float sprayAngle;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSprayAngle(float sprayAngle);
	// @Override
	virtual float GetSprayAngle();
	/**
	 * Inaccuracy while owner moving
	 */
private:
	float movingAccuracy;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMovingAccuracy(float movingAccuracy);
	// @Override
	virtual float GetMovingAccuracy();
	/**
	 * Fraction of targets move speed that is used as error offset
	 */
private:
	float targetMoveError;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTargetMoveError(float targetMoveError);
	// @Override
	virtual float GetTargetMoveError();
	/**
	 * Maximum distance the weapon will lead the target
	 */
private:
	float leadLimit;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLeadLimit(float leadLimit);
	// @Override
	virtual float GetLeadLimit();
	/**
	 * Factor for increasing the leadLimit with experience
	 */
private:
	float leadBonus;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLeadBonus(float leadBonus);
	// @Override
	virtual float GetLeadBonus();
	/**
	 * Replaces hardcoded behaviour for burnblow cannons
	 */
private:
	float predictBoost;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPredictBoost(float predictBoost);
	// @Override
	virtual float GetPredictBoost();
private:
	int numDamageTypes;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNumDamageTypes(int numDamageTypes);
	// @Override
	virtual int GetNumDamageTypes();
private:
	float areaOfEffect;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAreaOfEffect(float areaOfEffect);
	// @Override
	virtual float GetAreaOfEffect();
private:
	bool isNoSelfDamage;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNoSelfDamage(bool isNoSelfDamage);
	// @Override
	virtual bool IsNoSelfDamage();
private:
	float fireStarter;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFireStarter(float fireStarter);
	// @Override
	virtual float GetFireStarter();
private:
	float edgeEffectiveness;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetEdgeEffectiveness(float edgeEffectiveness);
	// @Override
	virtual float GetEdgeEffectiveness();
private:
	float size;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSize(float size);
	// @Override
	virtual float GetSize();
private:
	float sizeGrowth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSizeGrowth(float sizeGrowth);
	// @Override
	virtual float GetSizeGrowth();
private:
	float collisionSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCollisionSize(float collisionSize);
	// @Override
	virtual float GetCollisionSize();
private:
	int salvoSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSalvoSize(int salvoSize);
	// @Override
	virtual int GetSalvoSize();
private:
	float salvoDelay;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSalvoDelay(float salvoDelay);
	// @Override
	virtual float GetSalvoDelay();
private:
	float reload;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReload(float reload);
	// @Override
	virtual float GetReload();
private:
	float beamTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBeamTime(float beamTime);
	// @Override
	virtual float GetBeamTime();
private:
	bool isBeamBurst;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBeamBurst(bool isBeamBurst);
	// @Override
	virtual bool IsBeamBurst();
private:
	bool isWaterBounce;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWaterBounce(bool isWaterBounce);
	// @Override
	virtual bool IsWaterBounce();
private:
	bool isGroundBounce;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGroundBounce(bool isGroundBounce);
	// @Override
	virtual bool IsGroundBounce();
private:
	float bounceRebound;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBounceRebound(float bounceRebound);
	// @Override
	virtual float GetBounceRebound();
private:
	float bounceSlip;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBounceSlip(float bounceSlip);
	// @Override
	virtual float GetBounceSlip();
private:
	int numBounce;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNumBounce(int numBounce);
	// @Override
	virtual int GetNumBounce();
private:
	float maxAngle;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxAngle(float maxAngle);
	// @Override
	virtual float GetMaxAngle();
private:
	float upTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUpTime(float upTime);
	// @Override
	virtual float GetUpTime();
private:
	int flightTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFlightTime(int flightTime);
	// @Override
	virtual int GetFlightTime();
	// @Override
	virtual float GetCost(Resource* resource);
private:
	int projectilesPerShot;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetProjectilesPerShot(int projectilesPerShot);
	// @Override
	virtual int GetProjectilesPerShot();
private:
	bool isTurret;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurret(bool isTurret);
	// @Override
	virtual bool IsTurret();
private:
	bool isOnlyForward;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOnlyForward(bool isOnlyForward);
	// @Override
	virtual bool IsOnlyForward();
private:
	bool isFixedLauncher;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFixedLauncher(bool isFixedLauncher);
	// @Override
	virtual bool IsFixedLauncher();
private:
	bool isWaterWeapon;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWaterWeapon(bool isWaterWeapon);
	// @Override
	virtual bool IsWaterWeapon();
private:
	bool isFireSubmersed;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFireSubmersed(bool isFireSubmersed);
	// @Override
	virtual bool IsFireSubmersed();
	/**
	 * Lets a torpedo travel above water like it does below water
	 */
private:
	bool isSubMissile;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSubMissile(bool isSubMissile);
	// @Override
	virtual bool IsSubMissile();
private:
	bool isTracks;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTracks(bool isTracks);
	// @Override
	virtual bool IsTracks();
private:
	bool isDropped;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDropped(bool isDropped);
	// @Override
	virtual bool IsDropped();
	/**
	 * The weapon will only paralyze, not do real damage.
	 */
private:
	bool isParalyzer;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetParalyzer(bool isParalyzer);
	// @Override
	virtual bool IsParalyzer();
	/**
	 * The weapon damages by impacting, not by exploding.
	 */
private:
	bool isImpactOnly;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetImpactOnly(bool isImpactOnly);
	// @Override
	virtual bool IsImpactOnly();
	/**
	 * Can not target anything (for example: anti-nuke, D-Gun)
	 */
private:
	bool isNoAutoTarget;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNoAutoTarget(bool isNoAutoTarget);
	// @Override
	virtual bool IsNoAutoTarget();
	/**
	 * Has to be fired manually (by the player or an AI, example: D-Gun)
	 */
private:
	bool isManualFire;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetManualFire(bool isManualFire);
	// @Override
	virtual bool IsManualFire();
	/**
	 * Can intercept targetable weapons shots.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getTargetable()
	 */
private:
	int interceptor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetInterceptor(int interceptor);
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
private:
	int targetable;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTargetable(int targetable);
	// @Override
	virtual int GetTargetable();
private:
	bool isStockpileable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStockpileable(bool isStockpileable);
	// @Override
	virtual bool IsStockpileable();
	/**
	 * Range of interceptors.
	 * 
	 * example: anti-nuke
	 * 
	 * @see  getInterceptor()
	 */
private:
	float coverageRange;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCoverageRange(float coverageRange);
	// @Override
	virtual float GetCoverageRange();
	/**
	 * Build time of a missile
	 */
private:
	float stockpileTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStockpileTime(float stockpileTime);
	// @Override
	virtual float GetStockpileTime();
private:
	float intensity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetIntensity(float intensity);
	// @Override
	virtual float GetIntensity();
	/**
	 * @deprecated only relevant for visualization
	 */
private:
	float thickness;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetThickness(float thickness);
	// @Override
	virtual float GetThickness();
	/**
	 * @deprecated only relevant for visualization
	 */
private:
	float laserFlareSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLaserFlareSize(float laserFlareSize);
	// @Override
	virtual float GetLaserFlareSize();
	/**
	 * @deprecated only relevant for visualization
	 */
private:
	float coreThickness;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCoreThickness(float coreThickness);
	// @Override
	virtual float GetCoreThickness();
private:
	float duration;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDuration(float duration);
	// @Override
	virtual float GetDuration();
	/**
	 * @deprecated only relevant for visualization
	 */
private:
	int lodDistance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLodDistance(int lodDistance);
	// @Override
	virtual int GetLodDistance();
private:
	float falloffRate;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFalloffRate(float falloffRate);
	// @Override
	virtual float GetFalloffRate();
	/**
	 * @deprecated only relevant for visualization
	 */
private:
	int graphicsType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGraphicsType(int graphicsType);
	// @Override
	virtual int GetGraphicsType();
private:
	bool isSoundTrigger;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSoundTrigger(bool isSoundTrigger);
	// @Override
	virtual bool IsSoundTrigger();
private:
	bool isSelfExplode;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSelfExplode(bool isSelfExplode);
	// @Override
	virtual bool IsSelfExplode();
private:
	bool isGravityAffected;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGravityAffected(bool isGravityAffected);
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
private:
	int highTrajectory;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHighTrajectory(int highTrajectory);
	// @Override
	virtual int GetHighTrajectory();
private:
	float myGravity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMyGravity(float myGravity);
	// @Override
	virtual float GetMyGravity();
private:
	bool isNoExplode;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNoExplode(bool isNoExplode);
	// @Override
	virtual bool IsNoExplode();
private:
	float startVelocity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetStartVelocity(float startVelocity);
	// @Override
	virtual float GetStartVelocity();
private:
	float weaponAcceleration;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWeaponAcceleration(float weaponAcceleration);
	// @Override
	virtual float GetWeaponAcceleration();
private:
	float turnRate;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTurnRate(float turnRate);
	// @Override
	virtual float GetTurnRate();
private:
	float maxVelocity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxVelocity(float maxVelocity);
	// @Override
	virtual float GetMaxVelocity();
private:
	float projectileSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetProjectileSpeed(float projectileSpeed);
	// @Override
	virtual float GetProjectileSpeed();
private:
	float explosionSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetExplosionSpeed(float explosionSpeed);
	// @Override
	virtual float GetExplosionSpeed();
	/**
	 * Returns the bit field value denoting the categories this weapon should
	 * target, excluding all others.
	 * @see Game#getCategoryFlag
	 * @see Game#getCategoryName
	 */
private:
	int onlyTargetCategory;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOnlyTargetCategory(int onlyTargetCategory);
	// @Override
	virtual int GetOnlyTargetCategory();
	/**
	 * How much the missile will wobble around its course.
	 */
private:
	float wobble;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetWobble(float wobble);
	// @Override
	virtual float GetWobble();
	/**
	 * How much the missile will dance.
	 */
private:
	float dance;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDance(float dance);
	// @Override
	virtual float GetDance();
	/**
	 * How high trajectory missiles will try to fly in.
	 */
private:
	float trajectoryHeight;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTrajectoryHeight(float trajectoryHeight);
	// @Override
	virtual float GetTrajectoryHeight();
private:
	bool isLargeBeamLaser;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetLargeBeamLaser(bool isLargeBeamLaser);
	// @Override
	virtual bool IsLargeBeamLaser();
	/**
	 * If the weapon is a shield rather than a weapon.
	 */
private:
	bool isShield;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShield(bool isShield);
	// @Override
	virtual bool IsShield();
	/**
	 * If the weapon should be repulsed or absorbed.
	 */
private:
	bool isShieldRepulser;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShieldRepulser(bool isShieldRepulser);
	// @Override
	virtual bool IsShieldRepulser();
	/**
	 * If the shield only affects enemy projectiles.
	 */
private:
	bool isSmartShield;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSmartShield(bool isSmartShield);
	// @Override
	virtual bool IsSmartShield();
	/**
	 * If the shield only affects stuff coming from outside shield radius.
	 */
private:
	bool isExteriorShield;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetExteriorShield(bool isExteriorShield);
	// @Override
	virtual bool IsExteriorShield();
	/**
	 * If the shield should be graphically shown.
	 */
private:
	bool isVisibleShield;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVisibleShield(bool isVisibleShield);
	// @Override
	virtual bool IsVisibleShield();
	/**
	 * If a small graphic should be shown at each repulse.
	 */
private:
	bool isVisibleShieldRepulse;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVisibleShieldRepulse(bool isVisibleShieldRepulse);
	// @Override
	virtual bool IsVisibleShieldRepulse();
	/**
	 * The number of frames to draw the shield after it has been hit.
	 */
private:
	int visibleShieldHitFrames;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetVisibleShieldHitFrames(int visibleShieldHitFrames);
	// @Override
	virtual int GetVisibleShieldHitFrames();
	/**
	 * The type of shields that can intercept this weapon (bitfield).
	 * The weapon can be affected by shields if:
	 * (shield.getInterceptType() & weapon.getInterceptedByShieldType()) != 0
	 * 
	 * @see  getInterceptType()
	 */
private:
	int interceptedByShieldType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetInterceptedByShieldType(int interceptedByShieldType);
	// @Override
	virtual int GetInterceptedByShieldType();
	/**
	 * Tries to avoid friendly units while aiming?
	 */
private:
	bool isAvoidFriendly;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAvoidFriendly(bool isAvoidFriendly);
	// @Override
	virtual bool IsAvoidFriendly();
	/**
	 * Tries to avoid features while aiming?
	 */
private:
	bool isAvoidFeature;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAvoidFeature(bool isAvoidFeature);
	// @Override
	virtual bool IsAvoidFeature();
	/**
	 * Tries to avoid neutral units while aiming?
	 */
private:
	bool isAvoidNeutral;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAvoidNeutral(bool isAvoidNeutral);
	// @Override
	virtual bool IsAvoidNeutral();
	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
private:
	float targetBorder;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTargetBorder(float targetBorder);
	// @Override
	virtual float GetTargetBorder();
	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
private:
	float cylinderTargetting;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCylinderTargetting(float cylinderTargetting);
	// @Override
	virtual float GetCylinderTargetting();
	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
private:
	float minIntensity;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMinIntensity(float minIntensity);
	// @Override
	virtual float GetMinIntensity();
	/**
	 * Controls cannon range height boost.
	 * 
	 * default: -1: automatically calculate a more or less sane value
	 */
private:
	float heightBoostFactor;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHeightBoostFactor(float heightBoostFactor);
	// @Override
	virtual float GetHeightBoostFactor();
	/**
	 * Multiplier for the distance to the target for priority calculations.
	 */
private:
	float proximityPriority;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetProximityPriority(float proximityPriority);
	// @Override
	virtual float GetProximityPriority();
private:
	int collisionFlags;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCollisionFlags(int collisionFlags);
	// @Override
	virtual int GetCollisionFlags();
private:
	bool isSweepFire;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSweepFire(bool isSweepFire);
	// @Override
	virtual bool IsSweepFire();
private:
	bool isAbleToAttackGround;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetAbleToAttackGround(bool isAbleToAttackGround);
	// @Override
	virtual bool IsAbleToAttackGround();
private:
	float cameraShake;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCameraShake(float cameraShake);
	// @Override
	virtual float GetCameraShake();
private:
	float dynDamageExp;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDynDamageExp(float dynDamageExp);
	// @Override
	virtual float GetDynDamageExp();
private:
	float dynDamageMin;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDynDamageMin(float dynDamageMin);
	// @Override
	virtual float GetDynDamageMin();
private:
	float dynDamageRange;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDynDamageRange(float dynDamageRange);
	// @Override
	virtual float GetDynDamageRange();
private:
	bool isDynDamageInverted;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDynDamageInverted(bool isDynDamageInverted);
	// @Override
	virtual bool IsDynDamageInverted();
private:
	std::map<std::string,std::string> customParams;/* = std::map<std::string,std::string>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCustomParams(std::map<std::string,std::string> customParams);
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();
private:
	springai::Damage* damage;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDamage(springai::Damage* damage);
	// @Override
	virtual springai::Damage* GetDamage();
private:
	springai::Shield* shield;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShield(springai::Shield* shield);
	// @Override
	virtual springai::Shield* GetShield();
}; // class StubWeaponDef

}  // namespace springai

#endif // _CPPWRAPPER_STUBWEAPONDEF_H

