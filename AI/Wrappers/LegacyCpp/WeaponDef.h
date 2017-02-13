/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WEAPON_DEF_H
#define _WEAPON_DEF_H

#include <map>

#include "DamageArray.h"

#include "System/creg/creg_cond.h"
#include "System/float3.h"


namespace springLegacyAI {

struct WeaponDef
{
private:
	CR_DECLARE_STRUCT(WeaponDef)

public:
	WeaponDef()
		: range(0.0f)
		, heightmod(0.0f)
		, accuracy(0.0f)
		, sprayAngle(0.0f)
		, movingAccuracy(0.0f)
		, ownerExpAccWeight(0.0f)
		, targetMoveError(0.0f)
		, leadLimit(0.0f)
		, leadBonus(0.0f)
		, predictBoost(0.0f)
		, areaOfEffect(0.0f)
		, noSelfDamage(false)
		, fireStarter(0.0f)
		, edgeEffectiveness(0.0f)
		, size(0.0f)
		, sizeGrowth(0.0f)
		, collisionSize(0.0f)
		, salvosize(0)
		, salvodelay(0.0f)
		, reload(0.0f)
		, beamtime(0.0f)
		, beamburst(false)
		, waterBounce(false)
		, groundBounce(false)
		, bounceRebound(0.0f)
		, bounceSlip(0.0f)
		, numBounce(0)
		, maxAngle(0.0f)
		, uptime(0.0f)
		, flighttime(0)
		, metalcost(0.0f)
		, energycost(0.0f)
		, projectilespershot(0)
		, id(0)
		, tdfId(0)
		, turret(false)
		, onlyForward(false)
		, fixedLauncher(false)
		, waterweapon(false)
		, fireSubmersed(false)
		, submissile(false)
		, tracks(false)
		, dropped(false)
		, paralyzer(false)
		, impactOnly(false)
		, noAutoTarget(false)
		, manualfire(false)
		, interceptor(0)
		, targetable(0)
		, stockpile(false)
		, coverageRange(0.0f)
		, stockpileTime(0.0f)
		, intensity(0.0f)
		, thickness(0.0f)
		, laserflaresize(0.0f)
		, corethickness(0.0f)
		, duration(0.0f)
		, lodDistance(0)
		, falloffRate(0.0f)
		, graphicsType(0)
		, soundTrigger(false)
		, selfExplode(false)
		, gravityAffected(false)
		, highTrajectory(0)
		, myGravity(0.0f)
		, noExplode(false)
		, startvelocity(0.0f)
		, weaponacceleration(0.0f)
		, turnrate(0.0f)
		, maxvelocity(0.0f)
		, projectilespeed(0.0f)
		, explosionSpeed(0.0f)
		, onlyTargetCategory(0)
		, wobble(0.0f)
		, dance(0.0f)
		, trajectoryHeight(0.0f)
		, largeBeamLaser(false)
		, isShield(false)
		, shieldRepulser(false)
		, smartShield(false)
		, exteriorShield(false)
		, visibleShield(false)
		, visibleShieldRepulse(false)
		, visibleShieldHitFrames(0)
		, shieldEnergyUse(0.0f)
		, shieldRadius(0.0f)
		, shieldForce(0.0f)
		, shieldMaxSpeed(0.0f)
		, shieldPower(0.0f)
		, shieldPowerRegen(0.0f)
		, shieldPowerRegenEnergy(0.0f)
		, shieldStartingPower(0.0f)
		, shieldRechargeDelay(0.0f)
		, shieldGoodColor(ZeroVector)
		, shieldBadColor(ZeroVector)
		, shieldAlpha(0.0f)
		, shieldInterceptType(0)
		, interceptedByShieldType(0)
		, avoidFriendly(false)
		, avoidFeature(false)
		, avoidNeutral(false)
		, targetBorder(0.0f)
		, cylinderTargetting(0.0f)
		, minIntensity(0.0f)
		, heightBoostFactor(0.0f)
		, proximityPriority(0.0f)
		, collisionFlags(0)
		, sweepFire(false)
		, canAttackGround(false)
		, cameraShake(0.0f)
		, dynDamageExp(0.0f)
		, dynDamageMin(0.0f)
		, dynDamageRange(0.0f)
		, dynDamageInverted(false)
	{}

	WeaponDef(DamageArray damages)
		: range(0.0f)
		, heightmod(0.0f)
		, accuracy(0.0f)
		, sprayAngle(0.0f)
		, movingAccuracy(0.0f)
		, ownerExpAccWeight(0.0f)
		, targetMoveError(0.0f)
		, leadLimit(0.0f)
		, leadBonus(0.0f)
		, predictBoost(0.0f)
		, damages(damages)
		, areaOfEffect(0.0f)
		, noSelfDamage(false)
		, fireStarter(0.0f)
		, edgeEffectiveness(0.0f)
		, size(0.0f)
		, sizeGrowth(0.0f)
		, collisionSize(0.0f)
		, salvosize(0)
		, salvodelay(0.0f)
		, reload(0.0f)
		, beamtime(0.0f)
		, beamburst(false)
		, waterBounce(false)
		, groundBounce(false)
		, bounceRebound(0.0f)
		, bounceSlip(0.0f)
		, numBounce(0)
		, maxAngle(0.0f)
		, uptime(0.0f)
		, flighttime(0)
		, metalcost(0.0f)
		, energycost(0.0f)
		, projectilespershot(0)
		, id(0)
		, tdfId(0)
		, turret(false)
		, onlyForward(false)
		, fixedLauncher(false)
		, waterweapon(false)
		, fireSubmersed(false)
		, submissile(false)
		, tracks(false)
		, dropped(false)
		, paralyzer(false)
		, impactOnly(false)
		, noAutoTarget(false)
		, manualfire(false)
		, interceptor(0)
		, targetable(0)
		, stockpile(false)
		, coverageRange(0.0f)
		, stockpileTime(0.0f)
		, intensity(0.0f)
		, thickness(0.0f)
		, laserflaresize(0.0f)
		, corethickness(0.0f)
		, duration(0.0f)
		, lodDistance(0)
		, falloffRate(0.0f)
		, graphicsType(0)
		, soundTrigger(false)
		, selfExplode(false)
		, gravityAffected(false)
		, highTrajectory(0)
		, myGravity(0.0f)
		, noExplode(false)
		, startvelocity(0.0f)
		, weaponacceleration(0.0f)
		, turnrate(0.0f)
		, maxvelocity(0.0f)
		, projectilespeed(0.0f)
		, explosionSpeed(0.0f)
		, onlyTargetCategory(0)
		, wobble(0.0f)
		, dance(0.0f)
		, trajectoryHeight(0.0f)
		, largeBeamLaser(false)
		, isShield(false)
		, shieldRepulser(false)
		, smartShield(false)
		, exteriorShield(false)
		, visibleShield(false)
		, visibleShieldRepulse(false)
		, visibleShieldHitFrames(0)
		, shieldEnergyUse(0.0f)
		, shieldRadius(0.0f)
		, shieldForce(0.0f)
		, shieldMaxSpeed(0.0f)
		, shieldPower(0.0f)
		, shieldPowerRegen(0.0f)
		, shieldPowerRegenEnergy(0.0f)
		, shieldStartingPower(0.0f)
		, shieldRechargeDelay(0.0f)
		, shieldGoodColor(ZeroVector)
		, shieldBadColor(ZeroVector)
		, shieldAlpha(0.0f)
		, shieldInterceptType(0)
		, interceptedByShieldType(0)
		, avoidFriendly(false)
		, avoidFeature(false)
		, avoidNeutral(false)
		, targetBorder(0.0f)
		, cylinderTargetting(0.0f)
		, minIntensity(0.0f)
		, heightBoostFactor(0.0f)
		, proximityPriority(0.0f)
		, collisionFlags(0)
		, sweepFire(false)
		, canAttackGround(false)
		, cameraShake(0.0f)
		, dynDamageExp(0.0f)
		, dynDamageMin(0.0f)
		, dynDamageRange(0.0f)
		, dynDamageInverted(false)
	{}

	~WeaponDef() {}

	std::string name;
	std::string type;
	std::string description;
	std::string filename;
	std::string cegTag;        ///< tag of CEG that projectiles fired by this weapon should use

	float range;
	float heightmod;
	float accuracy;            ///< inaccuracy of whole burst
	float sprayAngle;          ///< inaccuracy of individual shots inside burst
	float movingAccuracy;      ///< inaccuracy while owner moving
	float ownerExpAccWeight;   ///< if 0, accuracy is not increased with owner experience (max. 1)
	float targetMoveError;     ///< fraction of targets move speed that is used as error offset
	float leadLimit;           ///< maximum distance the weapon will lead the target
	float leadBonus;           ///< factor for increasing the leadLimit with experience
	float predictBoost;        ///< replaces hardcoded behaviour for burnblow cannons

	DamageArray damages;
	float areaOfEffect;
	bool noSelfDamage;
	float fireStarter;
	float edgeEffectiveness;
	float size;
	float sizeGrowth;
	float collisionSize;

	int salvosize;
	float salvodelay;
	float reload;
	float beamtime;
	bool beamburst;

	bool waterBounce;
	bool groundBounce;
	float bounceRebound;
	float bounceSlip;
	int numBounce;

	float maxAngle;

	float uptime;
	int flighttime;

	float metalcost;
	float energycost;

	int projectilespershot;

	int id;
	int tdfId;                  ///< the id= tag in the tdf

	bool turret;
	bool onlyForward;
	bool fixedLauncher;
	bool waterweapon;
	bool fireSubmersed;
	bool submissile;            ///< Lets a torpedo travel above water like it does below water
	bool tracks;
	bool dropped;
	bool paralyzer;             ///< weapon will only paralyze not do real damage
	bool impactOnly;            ///< The weapon damages by impacting, not by exploding

	bool noAutoTarget;          ///< cant target stuff (for antinuke,dgun)
	bool manualfire;            ///< use dgun button
	int interceptor;            ///< anti nuke
	int targetable;             ///< nuke (can be shot by interceptor)
	bool stockpile;
	float coverageRange;        ///< range of anti nuke

	float stockpileTime;        ///< builtime of a missile

	float intensity;
	float thickness;
	float laserflaresize;
	float corethickness;
	float duration;
	int   lodDistance;
	float falloffRate;

	int graphicsType;
	bool soundTrigger;

	bool selfExplode;
	bool gravityAffected;
	int highTrajectory;         ///< Per-weapon high traj setting, 0=low, 1=high, 2=unit
	float myGravity;
	bool noExplode;
	float startvelocity;
	float weaponacceleration;
	float turnrate;
	float maxvelocity;

	float projectilespeed;
	float explosionSpeed;

	unsigned int onlyTargetCategory;

	float wobble;             ///< how much the missile will wobble around its course
	float dance;              ///< how much the missile will dance
	float trajectoryHeight;   ///< how high trajectory missiles will try to fly in

	bool largeBeamLaser;

	bool isShield;                   // if the weapon is a shield rather than a weapon
	bool shieldRepulser;             // if the weapon should be repulsed or absorbed
	bool smartShield;                // only affect enemy projectiles
	bool exteriorShield;             // only affect stuff coming from outside shield radius
	bool visibleShield;              // if the shield should be graphically shown
	bool visibleShieldRepulse;       // if a small graphic should be shown at each repulse
	int  visibleShieldHitFrames;     // number of frames to draw the shield after it has been hit
	float shieldEnergyUse;           // energy use per shot or per second depending on projectile
	float shieldRadius;              // size of shielded area
	float shieldForce;               // shield acceleration on plasma stuff
	float shieldMaxSpeed;            // max speed shield can repulse plasma like weapons with
	float shieldPower;               // how much damage the shield can reflect (0=infinite)
	float shieldPowerRegen;          // how fast the power regenerates per second
	float shieldPowerRegenEnergy;    // how much energy is needed to regenerate power per second
	float shieldStartingPower;       // how much power the shield has when first created
	int   shieldRechargeDelay;       // number of frames to delay recharging by after each hit
	float3 shieldGoodColor;          // color when shield at full power
	float3 shieldBadColor;           // color when shield is empty
	float shieldAlpha;               // shield alpha value

	unsigned int shieldInterceptType;      // type of shield (bitfield)
	unsigned int interceptedByShieldType;  // weapon can be affected by shields where (shieldInterceptType & interceptedByShieldType) is not zero

	bool avoidFriendly;     // if true, try to avoid friendly units while aiming
	bool avoidFeature;      // if true, try to avoid features while aiming
	bool avoidNeutral;      // if true, try to avoid neutral units while aiming
	/**
	 * If nonzero, targetting units will TryTarget at the edge of collision sphere
	 * (radius*tag value, [-1;1]) instead of its centre.
	 */
	float targetBorder;
	/**
	 * If greater than 0, the range will be checked in a cylinder
	 * (height=range*cylinderTargetting) instead of a sphere.
	 */
	float cylinderTargetting;
	/**
	 * For beam-lasers only - always hit with some minimum intensity
	 * (a damage coeffcient normally dependent on distance).
	 * Do not confuse this with the intensity tag, it i completely unrelated.
	 */
	float minIntensity;
	/**
	 * Controls cannon range height boost.
	 *
	 * default: -1: automatically calculate a more or less sane value
	 */
	float heightBoostFactor;
	float proximityPriority;     // multiplier for the distance to the target for priority calculations

	unsigned int collisionFlags;

	bool sweepFire;
	bool canAttackGround;

	float cameraShake;

	float dynDamageExp;
	float dynDamageMin;
	float dynDamageRange;
	bool dynDamageInverted;

	std::map<std::string, std::string> customParams;
};

} // namespace springLegacyAI

#endif // _WEAPON_DEF_H
