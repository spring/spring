/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Game/TraceRay.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"

CR_BIND(WeaponDef, );



static DefType WeaponDefs("WeaponDefs");

#define WEAPONTAG(T, name, ...) DEFTAG(WeaponDefs, WeaponDef, T, name, ##__VA_ARGS__)
#define WEAPONDUMMYTAG(T, name) DUMMYTAG(WeaponDefs, T, name)

// General
WEAPONTAG(std::string, description).externalName("name").defaultValue("Weapon").description("The descriptive name of the weapon as listed in FPS mode.");
WEAPONTAG(std::string, type).externalName("weaponType").defaultValue("Cannon");
WEAPONTAG(int, tdfId).externalName("id").defaultValue(0);
WEAPONDUMMYTAG(table, customParams);

// Collision & Avoidance
WEAPONTAG(bool, avoidFriendly).defaultValue(true);
WEAPONTAG(bool, avoidFeature).defaultValue(true);
WEAPONTAG(bool, avoidNeutral).defaultValue(false);
WEAPONTAG(bool, avoidGround).defaultValue(true);
WEAPONDUMMYTAG(bool, collideEnemy).defaultValue(true);
WEAPONDUMMYTAG(bool, collideFriendly).defaultValue(true);
WEAPONDUMMYTAG(bool, collideFeature).defaultValue(true);
WEAPONDUMMYTAG(bool, collideNeutral).defaultValue(true);
WEAPONDUMMYTAG(bool, collideGround).defaultValue(true);

// Damaging
WEAPONDUMMYTAG(table, damage);
WEAPONDUMMYTAG(float, damage.default).defaultValue(1.0f);
WEAPONDUMMYTAG(float, size);
WEAPONDUMMYTAG(float, explosionSpeed);
WEAPONTAG(bool, impactOnly).defaultValue(false);
WEAPONTAG(bool, noSelfDamage).defaultValue(false);
WEAPONTAG(bool, noExplode).defaultValue(false);
WEAPONTAG(bool, selfExplode).externalName("burnblow").defaultValue(false);
WEAPONTAG(float, damageAreaOfEffect).externalName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f);
WEAPONTAG(float, edgeEffectiveness).defaultValue(0.0f).maximumValue(0.999f);
WEAPONTAG(float, collisionSize).defaultValue(0.05f);

// Projectile Properties
WEAPONTAG(float, projectilespeed).externalName("weaponVelocity").defaultValue(0.0f).minimumValue(0.01f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(float, startvelocity).defaultValue(0.0f).minimumValue(0.01f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(float, weaponacceleration).defaultValue(0.0f).scaleValue(1.0f / (GAME_SPEED * GAME_SPEED));
WEAPONTAG(float, reload).externalName("reloadTime").defaultValue(1.0f);
WEAPONTAG(float, salvodelay).externalName("burstRate").defaultValue(0.1f);
WEAPONTAG(int, salvosize).externalName("burst").defaultValue(1);
WEAPONTAG(int, projectilespershot).externalName("projectiles").defaultValue(1);

// Bounce
WEAPONTAG(bool, waterBounce).defaultValue(false);
WEAPONTAG(bool, groundBounce).defaultValue(false);
WEAPONTAG(float, bounceSlip).defaultValue(1.0f);
WEAPONTAG(float, bounceRebound).defaultValue(1.0f);
WEAPONTAG(int, numBounce).defaultValue(-1);

// Crater & Impulse
WEAPONTAG(float, impulseFactor, damages.impulseFactor).defaultValue(1.0f);
WEAPONTAG(float, impulseBoost, damages.impulseBoost).defaultValue(0.0f);
WEAPONTAG(float, craterMult, damages.craterMult).fallbackName("impulseFactor").defaultValue(1.0f);
WEAPONTAG(float, craterBoost, damages.craterBoost).defaultValue(0.0f);
WEAPONTAG(float, craterAreaOfEffect).externalName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f);

// Water
WEAPONTAG(bool, waterweapon).defaultValue(false);
WEAPONTAG(bool, submissile).defaultValue(false);
WEAPONTAG(bool, fireSubmersed).fallbackName("waterweapon").defaultValue(false);

// Targeting
WEAPONTAG(bool, manualfire).externalName("commandfire").defaultValue(false);
WEAPONTAG(float, range).defaultValue(10.0f);
WEAPONTAG(float, heightmod).defaultValue(0.2f);
WEAPONTAG(float, targetBorder).defaultValue(0.0f).minimumValue(-1.0f).maximumValue(1.0f);
WEAPONTAG(float, cylinderTargeting).fallbackName("cylinderTargetting").defaultValue(0.0f).minimumValue(0.0f).maximumValue(128.0f);
WEAPONTAG(bool, turret).defaultValue(false).description("Does the unit aim within an arc (up-to and including full 360° turret traverse) or always aim along the owner's heading?");
WEAPONTAG(bool, fixedLauncher).defaultValue(false);
WEAPONTAG(float, maxAngle).externalName("tolerance").defaultValue(3000.0f).scaleValue(180.0f / COBSCALEHALF);
WEAPONTAG(int, highTrajectory).defaultValue(2);
WEAPONTAG(float, trajectoryHeight).defaultValue(0.0f);
WEAPONTAG(bool, tracks).defaultValue(false);
WEAPONTAG(float, wobble).defaultValue(0.0f).scaleValue(float(TAANG2RAD) / GAME_SPEED);
WEAPONTAG(float, dance).defaultValue(0.0f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(bool, gravityAffected).defaultValue(false);
WEAPONTAG(float, myGravity).defaultValue(0.0f);
WEAPONTAG(bool, canAttackGround).defaultValue(true);
WEAPONTAG(float, uptime).externalName("weaponTimer").defaultValue(0.0f);
WEAPONDUMMYTAG(float, flighttime).defaultValue(0).scaleValue(32); // needs to be written as int and read as float
WEAPONTAG(float, turnrate).defaultValue(0.0f).scaleValue(float(TAANG2RAD) / GAME_SPEED);
WEAPONTAG(float, heightBoostFactor).defaultValue(-1.0f);
WEAPONTAG(float, proximityPriority).defaultValue(1.0f);

// Target Error
TAGFUNCTION(AccuracyToSin, float, math::sin(x * PI / 0xafff)); // should really be tan but TA seem to cap it somehow, should also be 7fff or ffff theoretically but neither seems good
WEAPONTAG(float, accuracy).defaultValue(0.0f).tagFunction(AccuracyToSin);
WEAPONTAG(float, sprayAngle).defaultValue(0.0f).tagFunction(AccuracyToSin);
WEAPONTAG(float, movingAccuracy).externalName("accuracy").defaultValue(0.0f).tagFunction(AccuracyToSin);
WEAPONTAG(float, targetMoveError).defaultValue(0.0f);
WEAPONTAG(float, leadLimit).defaultValue(-1.0f);
WEAPONTAG(float, leadBonus).defaultValue(0.0f);
WEAPONTAG(float, predictBoost).defaultValue(0.0f);
WEAPONDUMMYTAG(float, ownerExpAccWeight);

// Laser Stuff
WEAPONTAG(float, minIntensity).defaultValue(0.0f).description("The minimum percentage the weapon's damage can fall-off to over its range. Setting to 1.0 will disable fall off entirely. Unrelated to the visual-only intensity tag.");
WEAPONTAG(float, duration).defaultValue(0.05f);
WEAPONTAG(float, beamtime).defaultValue(1.0f);
WEAPONTAG(bool, beamburst).defaultValue(false);
WEAPONTAG(int, beamLaserTTL).externalName("beamTTL").defaultValue(0);
WEAPONTAG(bool, sweepFire).defaultValue(false).description("Makes BeamLasers continue firing while aiming for a new target, 'sweeping' across the terrain.");
WEAPONTAG(bool, largeBeamLaser).defaultValue(false);

// FLAMETHROWER
WEAPONTAG(float, sizeGrowth).defaultValue(0.5f);
WEAPONDUMMYTAG(float, flameGfxTime);

// Eco
WEAPONTAG(float, metalcost).externalName("metalPerShot").defaultValue(0.0f);
WEAPONTAG(float, energycost).externalName("energyPerShot").defaultValue(0.0f);

// Other Properties
WEAPONTAG(float, fireStarter).defaultValue(0.0f).minimumValue(0.0f).maximumValue(100.0f).scaleValue(0.01f)
	.description("The percentage chance of the weapon setting fire to static map features on impact.");
WEAPONTAG(bool, paralyzer).defaultValue(false).description("Is the weapon a paralyzer? If true the weapon only stuns enemy units and does not cause damage in the form of lost hit-points.");
WEAPONTAG(int, paralyzeTime,  damages.paralyzeDamageTime).defaultValue(10).minimumValue(0).description("Determines the maximum length of time in seconds that the target will be paralyzed. The timer is restarted every time the target is hit by the weapon. Cannot be less than 0.");
WEAPONTAG(bool, stockpile).defaultValue(false).description("Does each round of the weapon have to be built and stockpiled by the player? Will only correctly function for the first of each stockpiled weapons a unit has.");
WEAPONTAG(float, stockpileTime).fallbackName("reload").defaultValue(1.0f).description("The time in seconds taken to stockpile one round of the weapon.");

// Interceptor
WEAPONTAG(int, targetable).defaultValue(0).description("Bitmask representing the types of weapon that can intercept this weapon. Each digit of binary that is set to one means that a weapon with the corresponding digit in its interceptor tag will intercept this weapon. Instant-hitting weapons such as [#BeamLaser], [#LightningCannon] and [#Rifle] cannot be targeted.");
WEAPONTAG(int, interceptor).defaultValue(0).description("Bitmask representing the types of weapons that this weapon can intercept. Each digit of binary that is set to one means that a weapon with the corresponding digit in its targetable tag will be intercepted by this weapon.");
WEAPONDUMMYTAG(unsigned, interceptedByShieldType).description("");
WEAPONTAG(float, coverageRange).externalName("coverage").defaultValue(0.0f).description("The radius in elmos within which an interceptor weapon will fire on targetable weapons.");
WEAPONTAG(bool, interceptSolo).defaultValue(true).description("If true no other interceptors may target the same projectile.");

// Dynamic Damage
WEAPONTAG(bool, dynDamageInverted).defaultValue(false).description("If true the damage curve is inverted i.e. the weapon does more damage at greater ranges as opposed to less.");
WEAPONTAG(float, dynDamageExp).defaultValue(0.0f).description("Exponent of the range-dependent damage formula, the default of 0.0 disables dynamic damage, 1.0 means linear scaling, 2.0 quadratic and so on.");
WEAPONTAG(float, dynDamageMin).defaultValue(0.0f).description("The minimum floor value that range-dependent damage can drop to.");
WEAPONTAG(float, dynDamageRange).defaultValue(0.0f).description("If set to non-zero values the weapon will use this value in the range-dependant damage formula instead of the actual range.");

// Shield
WEAPONTAG(bool, shieldRepulser).externalName("shield.repulser").fallbackName("shieldRepulser")
	.defaultValue(false).description("Does the shield repulse (deflect) projectiles or absorb them?");
WEAPONTAG(bool, smartShield).externalName("shield.smart").fallbackName("smartShield")
	.defaultValue(false).description("Determines whether or not projectiles fired by allied units can pass through the shield (true) or are intercepted as enemy weapons are (false).");
WEAPONTAG(bool, exteriorShield).externalName("shield.exterior").fallbackName("exteriorShield")
	.defaultValue(false).description("Determines whether or not projectiles fired within the shield's radius can pass through the shield (true) or are intercepted (false).");

WEAPONTAG(float, shieldMaxSpeed).externalName("shield.maxSpeed").fallbackName("shieldMaxSpeed")
	.defaultValue(0.0f).description("The maximum speed the repulsor will impart to deflected projectiles.");
WEAPONTAG(float, shieldForce).externalName("shield.force").fallbackName("shieldForce")
	.defaultValue(0.0f).description("The force applied by the repulsor to the weapon - higher values will deflect weapons away at higher velocities.");
WEAPONTAG(float, shieldRadius).externalName("shield.radius").fallbackName("shieldRadius")
	.defaultValue(0.0f).description("The radius of the circular area the shield covers.");
WEAPONTAG(float, shieldPower).externalName("shield.power").fallbackName("shieldPower")
	.defaultValue(0.0f).description("Essentially the maximum allowed hit-points of the shield - reduced by the damage of a weapon upon impact.");
WEAPONTAG(float, shieldStartingPower).externalName("shield.startingPower").fallbackName("shieldStartingPower")
	.defaultValue(0.0f).description("How many hit-points the shield starts with - otherwise the shield must regenerate from 0 until it reaches maximum power.");
WEAPONTAG(float, shieldPowerRegen).externalName("shield.powerRegen").fallbackName("shieldPowerRegen")
	.defaultValue(0.0f).description("How many hit-points the shield regenerates each second.");
WEAPONTAG(float, shieldPowerRegenEnergy).externalName("shield.powerRegenEnergy").fallbackName("shieldPowerRegenEnergy")
	.defaultValue(0.0f).description("How much energy resource is consumed to regenerate each hit-point.");
WEAPONTAG(float, shieldEnergyUse).externalName("shield.energyUse").fallbackName("shieldEnergyUse")
	.defaultValue(0.0f).description("The amount of the energy resource consumed by the shield to absorb or repulse weapons, continually drained by a repulsor as long as the projectile is in range.");
WEAPONDUMMYTAG(float, shieldRechargeDelay).externalName("rechargeDelay").fallbackName("shieldRechargeDelay").defaultValue(0)
	.scaleValue(GAME_SPEED).description("The delay in seconds before a shield begins to regenerate after it is hit."); // must be read as float
WEAPONTAG(unsigned int, shieldInterceptType).externalName("shield.interceptType").fallbackName("shieldInterceptType")
	.defaultValue(0).description("Bitmask representing the types of weapons that this shield can intercept. Each digit of binary that is set to one means that a weapon with the corresponding digit in its interceptedByShieldType will be intercepted by this shield (See [[Shield Interception Tag]] Use).");

WEAPONTAG(bool, visibleShield).externalName("shield.visible").fallbackName("visibleShield")
	.defaultValue(false).description("Is the shield visible or not?");
WEAPONTAG(bool, visibleShieldRepulse).externalName("shield.visibleRepulse").fallbackName("visibleShieldRepulse")
	.defaultValue(false).description("Is the (hard-coded) repulse effect rendered or not?");
WEAPONTAG(int, visibleShieldHitFrames).externalName("visibleHitFrames").fallbackName("visibleShieldHitFrames")
	.defaultValue(0).description("The number of frames a shield becomes visible for when hit.");
WEAPONTAG(float3, shieldBadColor).externalName("shield.badColor").fallbackName("shieldBadColor")
	.defaultValue(float3(1.0f, 0.5f, 0.5f)).description("The RGB colour the shield transitions to as its hit-points are reduced towards 0.");
WEAPONTAG(float3, shieldGoodColor).externalName("shield.goodColor").fallbackName("shieldGoodColor")
	.defaultValue(float3(0.5f, 0.5f, 1.0f)).description("The RGB colour the shield transitions to as its hit-points are regenerated towards its maximum power.");
WEAPONTAG(float, shieldAlpha).externalName("shield.alpha").fallbackName("shieldAlpha")
	.defaultValue(0.2f).description("The alpha transparency of the shield whilst it is visible.");



// Unsynced (= Visuals)
WEAPONTAG(std::string, model, visuals.modelName).defaultValue("");
WEAPONTAG(bool, explosionScar, visuals.explosionScar).defaultValue(true);
WEAPONTAG(bool, alwaysVisible, visuals.alwaysVisible).defaultValue(false);
WEAPONTAG(float, cameraShake).fallbackName("damage.default").defaultValue(0.0f).minimumValue(0.0f);

// Missile
WEAPONTAG(bool, smokeTrail, visuals.smokeTrail).defaultValue(false);

// Cannon
WEAPONTAG(float, sizeDecay, visuals.sizeDecay).defaultValue(0.0f);
WEAPONTAG(float, alphaDecay, visuals.alphaDecay).defaultValue(1.0f);
WEAPONTAG(float, separation, visuals.separation).defaultValue(1.0f);
WEAPONTAG(bool, noGap, visuals.noGap).defaultValue(true);
WEAPONTAG(int, stages, visuals.stages).defaultValue(5);

// Laser
WEAPONTAG(int, lodDistance, visuals.lodDistance).defaultValue(1000);
WEAPONTAG(float, thickness, visuals.thickness).defaultValue(2.0f);
WEAPONTAG(float, coreThickness, visuals.corethickness).defaultValue(0.25f);
WEAPONTAG(float, laserFlareSize, visuals.laserflaresize).defaultValue(15.0f);
WEAPONTAG(float, tileLength, visuals.tilelength).defaultValue(200.0f);
WEAPONTAG(float, scrollSpeed, visuals.scrollspeed).defaultValue(5.0f);
WEAPONTAG(float, pulseSpeed, visuals.pulseSpeed).defaultValue(1.0f);
WEAPONTAG(float, beamDecay, visuals.beamdecay).defaultValue(1.0f);
WEAPONTAG(float, falloffRate).defaultValue(0.5f);
WEAPONTAG(bool, laserHardStop).externalName("hardstop").defaultValue(false);

// Color
WEAPONTAG(float3, rgbColor, visuals.color).defaultValue(float3(1.0f, 0.5f, 0.0f));
WEAPONTAG(float3, rgbColor2, visuals.color2).defaultValue(float3(1.0f, 1.0f, 1.0f));
WEAPONTAG(float, intensity).defaultValue(0.9f).description("Alpha transparency for non-model projectiles. Lower values are more opaque, but 0.0 will cause the projectile to disappear entirely.");
WEAPONDUMMYTAG(std::string, colormap);

WEAPONTAG(std::string, textures1, visuals.texNames[0]).externalName("textures.1").fallbackName("texture1").defaultValue("");
WEAPONTAG(std::string, textures2, visuals.texNames[1]).externalName("textures.2").fallbackName("texture2").defaultValue("");
WEAPONTAG(std::string, textures3, visuals.texNames[2]).externalName("textures.3").fallbackName("texture3").defaultValue("");
WEAPONTAG(std::string, textures4, visuals.texNames[3]).externalName("textures.4").fallbackName("texture4").defaultValue("");

WEAPONTAG(std::string, cegTag,                   visuals.ptrailExpGenTag).defaultValue("").description("The name, without prefixes, of a CEG to be emitted by the projectile each frame.");
WEAPONTAG(std::string, explosionGenerator,       visuals.impactExpGenTag).defaultValue("");
WEAPONTAG(std::string, bounceExplosionGenerator, visuals.bounceExpGenTag).defaultValue("");

// Sound
WEAPONDUMMYTAG(bool, soundTrigger);
WEAPONDUMMYTAG(std::string, soundStart).defaultValue("");
WEAPONDUMMYTAG(std::string, soundHitDry).fallbackName("soundHit").defaultValue("");
WEAPONDUMMYTAG(std::string, soundHitWet).fallbackName("soundHit").defaultValue("");
WEAPONDUMMYTAG(float, soundStartVolume).defaultValue(-1.0f);
WEAPONDUMMYTAG(float, soundHitDryVolume).fallbackName("soundHitVolume").defaultValue(-1.0f);
WEAPONDUMMYTAG(float, soundHitWetVolume).fallbackName("soundHitVolume").defaultValue(-1.0f);


WeaponDef::WeaponDef()
{
	id = 0;
	projectileType = WEAPON_BASE_PROJECTILE;
	collisionFlags = 0;

	// set later by ProjectileDrawer
	ptrailExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;
	impactExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_STANDARD;
	bounceExplosionGeneratorID = CExplosionGeneratorHandler::EXPGEN_ID_INVALID;

	isShield = false;
	noAutoTarget = false;
	onlyForward = false;

	const LuaTable wdTable;
	WeaponDefs.Load(this, wdTable);
}

WeaponDef::WeaponDef(const LuaTable& wdTable, const std::string& name_, int id_)
	: name(name_)

	, ptrailExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID)
	, impactExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_STANDARD)
	, bounceExplosionGeneratorID(CExplosionGeneratorHandler::EXPGEN_ID_INVALID)

	, id(id_)
	, projectileType(WEAPON_BASE_PROJECTILE)
	, collisionFlags(0)
{
	WeaponDefs.Load(this, wdTable);

	if (wdTable.KeyExists("cylinderTargetting"))
		LOG_L(L_WARNING, "WeaponDef (%s) cylinderTargetting is deprecated and will be removed in the next release (use cylinderTargeting).", name.c_str());

	if (wdTable.KeyExists("color1") || wdTable.KeyExists("color2"))
		LOG_L(L_WARNING, "WeaponDef (%s) color1 & color2 (= hue & sat) are removed. Use rgbColor instead!", name.c_str());

	if (wdTable.KeyExists("isShield"))
		LOG_L(L_WARNING, "WeaponDef (%s) The \"isShield\" tag has been removed. Use the weaponType=\"Shield\" tag instead!", name.c_str());

	shieldRechargeDelay = int(wdTable.GetFloat("rechargeDelay", 0) * GAME_SPEED);
	flighttime = int(wdTable.GetFloat("flighttime", 0.0f) * 32);

	//FIXME may be smarter to merge the collideXYZ tags with avoidXYZ and removing the collisionFlags tag (and move the code into CWeapon)?
	collisionFlags = 0;
	if (!wdTable.GetBool("collideEnemy",    true)) { collisionFlags |= Collision::NOENEMIES;    }
	if (!wdTable.GetBool("collideFriendly", true)) { collisionFlags |= Collision::NOFRIENDLIES; }
	if (!wdTable.GetBool("collideFeature",  true)) { collisionFlags |= Collision::NOFEATURES;   }
	if (!wdTable.GetBool("collideNeutral",  true)) { collisionFlags |= Collision::NONEUTRALS;   }
	if (!wdTable.GetBool("collideGround",   true)) { collisionFlags |= Collision::NOGROUND;     }

	//FIXME defaults depend on other tags
	{
		if (paralyzer)
			cameraShake = wdTable.GetFloat("cameraShake", 0.0f);

		if (selfExplode)
			predictBoost = wdTable.GetFloat("predictBoost", 0.5f);

		if (type == "Melee") {
			targetBorder = Clamp(wdTable.GetFloat("targetBorder", 1.0f), -1.0f, 1.0f);
			cylinderTargeting = Clamp(wdTable.GetFloat("cylinderTargeting", wdTable.GetFloat("cylinderTargetting", 1.0f)), 0.0f, 128.0f);
		}

		if (type == "Flame") {
			//FIXME move to lua (for all other weapons this tag is named `duration` and has a different default)
			duration = wdTable.GetFloat("flameGfxTime", 1.2f);
		}

		if (type == "Cannon") {
			heightmod = wdTable.GetFloat("heightMod", 0.8f);
		} else if (type == "BeamLaser" || type == "LightningCannon") {
			heightmod = wdTable.GetFloat("heightMod", 1.0f);
		}
	}

	// setup the default damages
	{
		const LuaTable dmgTable = wdTable.SubTable("damage");
		float defDamage = dmgTable.GetFloat("default", 1.0f);

		// avoid division by zeros
		if (defDamage == 0.0f)
			defDamage = 1.0f;

		damages.SetDefaultDamage(defDamage);

		if (!paralyzer)
			damages.paralyzeDamageTime = 0;

		std::map<string, float> dmgs;
		std::map<string, float>::const_iterator di;

		dmgTable.GetMap(dmgs);

		for (di = dmgs.begin(); di != dmgs.end(); ++di) {
			const int type = damageArrayHandler->GetTypeFromName(di->first);
			if (type != 0) {
				float dmg = di->second;
				if (dmg != 0.0f) {
					damages[type] = dmg;
				} else {
					damages[type] = 1.0f;
				}
			}
		}

		const float tempsize = 2.0f + std::min(defDamage * 0.0025f, damageAreaOfEffect * 0.1f);
		const float gd = std::max(30.0f, defDamage / 20.0f);
		const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (math::sqrt(gd) * 0.7f)) * 0.5f;

		size = wdTable.GetFloat("size", tempsize);
		explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);
	}

	{
		// 0.78.2.1 backwards compatibility: non-burst beamlasers play one
		// sample per shot, not for each individual beam making up the shot
		const bool singleSampleShot = (type == "BeamLaser" && !beamburst);
		const bool singleShotWeapon = (type == "Melee" || type == "Rifle");

		soundTrigger = wdTable.GetBool("soundTrigger", singleSampleShot || singleShotWeapon);
	}

	// get some weapon specific defaults
	int defInterceptType = 0;

	if (type == "Cannon") {
		// CExplosiveProjectile
		defInterceptType = 1;
		projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
		intensity = wdTable.GetFloat("intensity", 0.2f);
	} else if (type == "Rifle") {
		// no projectile or intercept type
		defInterceptType = 128;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
	} else if (type == "Melee") {
		// no projectile or intercept type
		defInterceptType = 256;
	} else if (type == "Flame") {
		// CFlameProjectile
		projectileType = WEAPON_FLAME_PROJECTILE;
		defInterceptType = 16;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.2f);
		collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (type == "MissileLauncher") {
		// CMissileProjectile
		projectileType = WEAPON_MISSILE_PROJECTILE;
		defInterceptType = 4;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (type == "LaserCannon") {
		// CLaserProjectile
		projectileType = WEAPON_LASER_PROJECTILE;
		defInterceptType = 2;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
		collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (type == "BeamLaser") {
		projectileType = largeBeamLaser? WEAPON_LARGEBEAMLASER_PROJECTILE: WEAPON_BEAMLASER_PROJECTILE;
		defInterceptType = 2;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else if (type == "LightningCannon") {
		projectileType = WEAPON_LIGHTNING_PROJECTILE;
		defInterceptType = 64;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (type == "EmgCannon") {
		// CEmgProjectile
		projectileType = WEAPON_EMG_PROJECTILE;
		defInterceptType = 1;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		size = wdTable.GetFloat("size", 3.0f);
	} else if (type == "TorpedoLauncher") {
		// WeaponLoader will create either BombDropper with dropTorpedoes = true
		// (owner->unitDef->canfly && !weaponDef->submissile) or TorpedoLauncher
		// (both types of weapons will spawn TorpedoProjectile's)
		//
		projectileType = WEAPON_TORPEDO_PROJECTILE;
		defInterceptType = 32;

		waterweapon = true;
	} else if (type == "DGun") {
		// CFireBallProjectile
		projectileType = WEAPON_FIREBALL_PROJECTILE;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
	} else if (type == "StarburstLauncher") {
		// CStarburstProjectile
		projectileType = WEAPON_STARBURST_PROJECTILE;
		defInterceptType = 4;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else if (type == "AircraftBomb") {
		// WeaponLoader will create BombDropper with dropTorpedoes = false
		// BombDropper with dropTorpedoes=false spawns ExplosiveProjectile's
		//
		projectileType = WEAPON_EXPLOSIVE_PROJECTILE;
		defInterceptType = 8;

		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
	} else {
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f);
	}

	interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);

	const std::string& colormap = wdTable.GetString("colormap", "");

	if (!colormap.empty()) {
		visuals.colorMap = CColorMap::LoadFromDefString(colormap);
	} else {
		visuals.colorMap = NULL;
	}

	ParseWeaponSounds(wdTable);

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(customParams);

	// internal only
	isShield = (type == "Shield");
	noAutoTarget = (manualfire || interceptor || isShield);
	onlyForward = !turret && (type != "StarburstLauncher");
}



void WeaponDef::ParseWeaponSounds(const LuaTable& wdTable) {
	LoadSound(wdTable, "soundStart",  0, fireSound.sounds);
	LoadSound(wdTable, "soundHitDry", 0, hitSound.sounds);
	LoadSound(wdTable, "soundHitWet", 1, hitSound.sounds);

	// FIXME: do we still want or need any of this?
	const bool forceSetVolume =
		(fireSound.getVolume(0) == -1.0f) ||
		(hitSound.getVolume(0) == -1.0f)  ||
		(hitSound.getVolume(1) == -1.0f);

	if (!forceSetVolume)
		return;

	if (damages[0] <= 50.0f) {
		fireSound.setVolume(0, 5.0f);
		hitSound.setVolume(0, 5.0f);
		hitSound.setVolume(1, 5.0f);
	} else {
		float fireSoundVolume = math::sqrt(damages[0] * 0.5f) * ((type == "LaserCannon")? 0.5f: 1.0f);
		float hitSoundVolume = fireSoundVolume;

		if (fireSoundVolume > 100.0f) {
			if (type == "MissileLauncher" || type == "StarburstLauncher") {
				fireSoundVolume = 10.0f * math::sqrt(hitSoundVolume);
			}
		}

		if (damageAreaOfEffect > 8.0f) {
			hitSoundVolume *= 2.0f;
		}
		if (type == "DGun") {
			hitSoundVolume *= 0.15f;
		}

		if (fireSound.getVolume(0) == -1.0f) { fireSound.setVolume(0, fireSoundVolume); }
		if (hitSound.getVolume(0) == -1.0f) { hitSound.setVolume(0, hitSoundVolume); }
		if (hitSound.getVolume(1) == -1.0f) { hitSound.setVolume(1, hitSoundVolume); }
	}
}



void WeaponDef::LoadSound(
	const LuaTable& wdTable,
	const std::string& soundKey,
	const unsigned int soundIdx,
	std::vector<GuiSoundSet::Data>& soundData)
{
	string name = "";
	int id = -1;
	float volume = -1.0f;

	soundData.push_back(GuiSoundSet::Data(name, id, volume));
	assert(soundIdx < soundData.size());
	assert(soundData[soundIdx].id == -1);

	if (soundKey == "soundStart") {
		name   = wdTable.GetString(soundKey, "");
		volume = wdTable.GetFloat(soundKey + "Volume", -1.0f);
	}
	else if (soundKey == "soundHitDry") {
		name   = wdTable.GetString(soundKey, wdTable.GetString("soundHit", ""));
		volume = wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", -1.0f));
	}
	else if (soundKey == "soundHitWet") {
		name   = wdTable.GetString(soundKey, wdTable.GetString("soundHit", ""));
		volume = wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", -1.0f));
	}

	if (name.empty())
		return;

	if ((id = CommonDefHandler::LoadSoundFile(name)) <= 0)
		return;

	soundData[soundIdx] = GuiSoundSet::Data(name, id, volume);
}


S3DModel* WeaponDef::LoadModel()
{
	if (visuals.model == NULL) {
		if (!visuals.modelName.empty()) {
			visuals.model = modelParser->Load3DModel(visuals.modelName);
		} else {
			// not useful, too much spam
			// LOG_L(L_WARNING, "[WeaponDef::%s] weapon \"%s\" has no model defined", __FUNCTION__, name.c_str());
		}
	} else {
		eventHandler.LoadedModelRequested();
	}

	return visuals.model;
}

S3DModel* WeaponDef::LoadModel() const {
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

