/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Game/TraceRay.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileTypes.h"
#include "Sim/Units/Scripts/CobInstance.h" // TAANG2RAD
#include "System/SpringMath.h"
#include "System/Log/ILog.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"


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
WEAPONTAG(bool, avoidCloaked).defaultValue(false);
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
WEAPONTAG(float, damageAreaOfEffect, damages.damageAreaOfEffect).fallbackName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f);
WEAPONTAG(float, edgeEffectiveness, damages.edgeEffectiveness).defaultValue(0.0f).maximumValue(0.999f);
WEAPONTAG(float, collisionSize).defaultValue(0.05f);

// Projectile Properties
WEAPONTAG(float, projectilespeed).externalName("weaponVelocity").fallbackName("maxVelocity").defaultValue(0.0f).minimumValue(0.01f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(float, startvelocity).defaultValue(0.0f).minimumValue(0.01f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(float, weaponacceleration).fallbackName("acceleration").defaultValue(0.0f).scaleValue(1.0f / (GAME_SPEED * GAME_SPEED));
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
WEAPONTAG(float, craterAreaOfEffect, damages.craterAreaOfEffect).fallbackName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f);

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
WEAPONTAG(float, maxAngle).externalName("tolerance").defaultValue(3000.0f).scaleValue(TAANG2RAD);
WEAPONDUMMYTAG(float, maxFireAngle).externalName("firetolerance").defaultValue(3640.0f).scaleValue(TAANG2RAD); // default value is 20degree
WEAPONTAG(int, highTrajectory).defaultValue(2);
WEAPONTAG(float, trajectoryHeight).defaultValue(0.0f);
WEAPONTAG(bool, tracks).defaultValue(false);
WEAPONTAG(float, wobble).defaultValue(0.0f).scaleValue(float(TAANG2RAD) / GAME_SPEED);
WEAPONTAG(float, dance).defaultValue(0.0f).scaleValue(1.0f / GAME_SPEED);
WEAPONTAG(bool, gravityAffected).defaultValue(false);
WEAPONTAG(float, myGravity).defaultValue(0.0f);
WEAPONTAG(bool, canAttackGround).defaultValue(true);
WEAPONTAG(float, uptime).externalName("weaponTimer").defaultValue(0.0f);
WEAPONDUMMYTAG(float, flighttime).defaultValue(0).scaleValue(GAME_SPEED).description("Flighttime of missiles in seconds."); // needs to be written as int and read as float
WEAPONTAG(float, turnrate).defaultValue(0.0f).scaleValue(float(TAANG2RAD) / GAME_SPEED);
WEAPONTAG(float, heightBoostFactor).defaultValue(-1.0f);
WEAPONTAG(float, proximityPriority).defaultValue(1.0f);
WEAPONTAG(bool, allowNonBlockingAim).defaultValue(false).description("When false, the weapon is blocked from firing until AimWeapon() returns.");

// Target Error
TAGFUNCTION(AccuracyToSin, float, math::sin(x * math::PI / 0xafff)) // should really be tan but TA seem to cap it somehow, should also be 7fff or ffff theoretically but neither seems good
WEAPONTAG(float, accuracy).defaultValue(0.0f).tagFunction(AccuracyToSin);
WEAPONTAG(float, sprayAngle).defaultValue(0.0f).tagFunction(AccuracyToSin);
WEAPONTAG(float, movingAccuracy).fallbackName("accuracy").defaultValue(0.0f).tagFunction(AccuracyToSin);
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
WEAPONTAG(float, stockpileTime).fallbackName("reload").defaultValue(1.0f).scaleValue(GAME_SPEED).description("The time in seconds taken to stockpile one round of the weapon.");

// Interceptor
WEAPONTAG(int, targetable).defaultValue(0).description("Bitmask representing the types of weapon that can intercept this weapon. Each digit of binary that is set to one means that a weapon with the corresponding digit in its interceptor tag will intercept this weapon. Instant-hitting weapons such as [#BeamLaser], [#LightningCannon] and [#Rifle] cannot be targeted.");
WEAPONTAG(int, interceptor).defaultValue(0).description("Bitmask representing the types of weapons that this weapon can intercept. Each digit of binary that is set to one means that a weapon with the corresponding digit in its targetable tag will be intercepted by this weapon.");
WEAPONDUMMYTAG(unsigned, interceptedByShieldType).description("");
WEAPONTAG(float, coverageRange).externalName("coverage").defaultValue(0.0f).description("The radius in elmos within which an interceptor weapon will fire on targetable weapons.");
WEAPONTAG(bool, interceptSolo).defaultValue(true).description("If true no other interceptors may target the same projectile.");

// Dynamic Damage
WEAPONTAG(bool, dynDamageInverted, damages.dynDamageInverted).defaultValue(false).description("If true the damage curve is inverted i.e. the weapon does more damage at greater ranges as opposed to less.");
WEAPONTAG(float, dynDamageExp, damages.dynDamageExp).defaultValue(0.0f).description("Exponent of the range-dependent damage formula, the default of 0.0 disables dynamic damage, 1.0 means linear scaling, 2.0 quadratic and so on.");
WEAPONTAG(float, dynDamageMin, damages.dynDamageMin).defaultValue(0.0f).description("The minimum floor value that range-dependent damage can drop to.");
WEAPONTAG(float, dynDamageRange, damages.dynDamageRange).defaultValue(0.0f).description("If set to non-zero values the weapon will use this value in the range-dependant damage formula instead of the actual range.");

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
WEAPONTAG(int, visibleShieldHitFrames).externalName("shield.visibleHitFrames").fallbackName("visibleShieldHitFrames")
	.defaultValue(0).description("The number of frames a shield becomes visible for when hit.");
WEAPONTAG(float4, shieldBadColor).externalName("shield.badColor").fallbackName("shieldBadColor")
	.defaultValue(float4(1.0f, 0.5f, 0.5f, 1.0f)).description("The RGBA colour the shield transitions to as its hit-points are reduced towards 0.");
WEAPONTAG(float4, shieldGoodColor).externalName("shield.goodColor").fallbackName("shieldGoodColor")
	.defaultValue(float4(0.5f, 0.5f, 1.0f, 1.0f)).description("The RGBA colour the shield transitions to as its hit-points are regenerated towards its maximum power.");
WEAPONTAG(float, shieldAlpha).externalName("shield.alpha").fallbackName("shieldAlpha")
	.defaultValue(0.2f).description("The alpha transparency of the shield whilst it is visible.");
WEAPONTAG(std::string, shieldArmorTypeName).externalName("shield.armorType").fallbackName("shieldArmorType")
	.defaultValue("default").description("Specifies the armorclass of the shield; you can input either an armorclass name OR a unitdef name to share that unit's armorclass");

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
WEAPONTAG(std::string, colormap, visuals.colorMapStr).defaultValue("");

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

	isNulled = false;
	isShield = false;
	noAutoTarget = false;
	onlyForward = false;

	damages.fromDef = true;

	WeaponDefs.Load(this, {});
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
	{
		WeaponDefs.Load(this, wdTable);
		WeaponDefs.ReportUnknownTags(name, wdTable);
	}


	if (wdTable.KeyExists("cylinderTargetting"))
		LOG_L(L_WARNING, "WeaponDef (%s) cylinderTargetting is deprecated and will be removed in the next release (use cylinderTargeting).", name.c_str());

	if (wdTable.KeyExists("color1") || wdTable.KeyExists("color2"))
		LOG_L(L_WARNING, "WeaponDef (%s) color1 & color2 (= hue & sat) are removed. Use rgbColor instead!", name.c_str());

	if (wdTable.KeyExists("isShield"))
		LOG_L(L_WARNING, "WeaponDef (%s) The \"isShield\" tag has been removed. Use the weaponType=\"Shield\" tag instead!", name.c_str());

	shieldRechargeDelay = int(wdTable.GetFloat("rechargeDelay", 0) * GAME_SPEED);
	shieldArmorType = damageArrayHandler.GetTypeFromName(shieldArmorTypeName);
	flighttime = int(wdTable.GetFloat("flighttime", 0.0f) * GAME_SPEED);
	maxFireAngle = math::cos(wdTable.GetFloat("firetolerance", 3640.0f) * TAANG2RAD);

	//FIXME may be smarter to merge the collideXYZ tags with avoidXYZ and removing the collisionFlags tag (and move the code into CWeapon)?
	collisionFlags = 0;
	collisionFlags |= (Collision::NOENEMIES    * (!wdTable.GetBool("collideEnemy",      true)));
	collisionFlags |= (Collision::NOFRIENDLIES * (!wdTable.GetBool("collideFriendly",   true)));
	collisionFlags |= (Collision::NOFEATURES   * (!wdTable.GetBool("collideFeature",    true)));
	collisionFlags |= (Collision::NONEUTRALS   * (!wdTable.GetBool("collideNeutral",    true)));
	collisionFlags |= (Collision::NOFIREBASES  * (!wdTable.GetBool("collideFireBase",  false)));
	collisionFlags |= (Collision::NONONTARGETS * (!wdTable.GetBool("collideNonTarget",  true)));
	collisionFlags |= (Collision::NOGROUND     * (!wdTable.GetBool("collideGround",     true)));
	collisionFlags |= (Collision::NOCLOAKED    * (!wdTable.GetBool("collideCloaked",    true)));

	//FIXME defaults depend on other tags
	{
		if (paralyzer)
			cameraShake = wdTable.GetFloat("cameraShake", 0.0f);

		if (selfExplode)
			predictBoost = wdTable.GetFloat("predictBoost", 0.5f);

		switch (hashString(type.c_str())) {
			case hashString("Melee"): {
				targetBorder = Clamp(wdTable.GetFloat("targetBorder", 1.0f), -1.0f, 1.0f);
				cylinderTargeting = Clamp(wdTable.GetFloat("cylinderTargeting", wdTable.GetFloat("cylinderTargetting", 1.0f)), 0.0f, 128.0f);
			} break;

			//TODO move to lua (for all other weapons this tag is named `duration` and has a different default)
			case hashString("Flame"): {
				duration = wdTable.GetFloat("flameGfxTime", 1.2f);
			} break;

			case hashString("Cannon"): {
				heightmod = wdTable.GetFloat("heightMod", 0.8f);
			} break;
			case hashString("BeamLaser"):
			case hashString("LightningCannon"): {
				heightmod = wdTable.GetFloat("heightMod", 1.0f);
			} break;

			case hashString("LaserCannon"): {
				// for lasers we want this to be true by default: it sets
				// projectile ttl values to the minimum required to hit a
				// target which prevents them overshooting (lasers travel
				// many elmos per frame and ttl's are rounded) at maximum
				// range
				selfExplode = wdTable.GetBool("burnblow", true);
			} break;

			default: {
			} break;
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
		damages.fromDef = true;

		if (!paralyzer)
			damages.paralyzeDamageTime = 0;


		static std::vector<std::pair<std::string, float>> dmgs;

		dmgs.clear();
		dmgs.reserve(32);
		dmgTable.GetPairs(dmgs);

		for (const auto& pair: dmgs) {
			const int type = damageArrayHandler.GetTypeFromName(pair.first);
			if (type == 0)
				continue;
			damages.Set(type, std::max(0.0001f, pair.second));
		}

		const float tempsize = 2.0f + std::min(defDamage * 0.0025f, damages.damageAreaOfEffect * 0.1f);
		const float gd = std::max(30.0f, defDamage / 20.0f);
		const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (math::sqrt(gd) * 0.7f)) * 0.5f;

		size = wdTable.GetFloat("size", tempsize);
		damages.explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);
		if (damages.dynDamageRange <= 0.0f)
			damages.dynDamageRange = range;
	}

	{
		// 0.78.2.1 backwards compatibility: non-burst beamlasers play one
		// sample per shot, not for each individual beam making up the shot
		const bool singleSampleShot = (type == "BeamLaser" && !beamburst);
		const bool singleShotWeapon = (type == "Melee" || type == "Rifle");

		soundTrigger = wdTable.GetBool("soundTrigger", singleSampleShot || singleShotWeapon);
	}

	{
		// get some weapon specific defaults
		defInterceptType = 0;

		ownerExpAccWeight = -1.0f;

		switch (hashString(type.c_str())) {
			case hashString("Cannon"): {
				// CExplosiveProjectile
				defInterceptType = 1;
				projectileType = WEAPON_EXPLOSIVE_PROJECTILE;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
				intensity = wdTable.GetFloat("intensity", 0.2f);
			} break;
			case hashString("Rifle"): {
				// no projectile or intercept type
				defInterceptType = 128;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
			} break;
			case hashString("Melee"): {
				// no projectile or intercept type
				defInterceptType = 256;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
			} break;
			case hashString("Flame"): {
				// CFlameProjectile
				projectileType = WEAPON_FLAME_PROJECTILE;
				defInterceptType = 16;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.2f);
				collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
			} break;
			case hashString("MissileLauncher"): {
				// CMissileProjectile
				projectileType = WEAPON_MISSILE_PROJECTILE;
				defInterceptType = 4;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
			} break;
			case hashString("LaserCannon"): {
				// CLaserProjectile
				projectileType = WEAPON_LASER_PROJECTILE;
				defInterceptType = 2;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
				collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
			} break;
			case hashString("BeamLaser"): {
				projectileType = largeBeamLaser? WEAPON_LARGEBEAMLASER_PROJECTILE: WEAPON_BEAMLASER_PROJECTILE;
				defInterceptType = 2;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
			} break;
			case hashString("LightningCannon"): {
				projectileType = WEAPON_LIGHTNING_PROJECTILE;
				defInterceptType = 64;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
			} break;
			case hashString("EmgCannon"): {
				// CEmgProjectile
				projectileType = WEAPON_EMG_PROJECTILE;
				defInterceptType = 1;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
				size = wdTable.GetFloat("size", 3.0f);
			} break;
			case hashString("TorpedoLauncher"): {
				// WeaponLoader will create either BombDropper with dropTorpedoes = true
				// (owner->unitDef->canfly && !weaponDef->submissile) or TorpedoLauncher
				// (both types of weapons will spawn TorpedoProjectile's)
				//
				projectileType = WEAPON_TORPEDO_PROJECTILE;
				defInterceptType = 32;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
				waterweapon = true;
			} break;
			case hashString("DGun"): {
				// CFireBallProjectile
				projectileType = WEAPON_FIREBALL_PROJECTILE;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
				collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
			} break;
			case hashString("StarburstLauncher"): {
				// CStarburstProjectile
				projectileType = WEAPON_STARBURST_PROJECTILE;
				defInterceptType = 4;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
			} break;
			case hashString("AircraftBomb"): {
				// WeaponLoader will create BombDropper with dropTorpedoes = false
				// BombDropper with dropTorpedoes=false spawns ExplosiveProjectile's
				//
				projectileType = WEAPON_EXPLOSIVE_PROJECTILE;
				defInterceptType = 8;

				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
			} break;
			default: {
				ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f);
			} break;
		}

		if (ownerExpAccWeight < 0.0f)
			LOG_L(L_ERROR, "[%s] weaponDef %s has negative ownerExpAccWeight %f", __func__, name.c_str(), ownerExpAccWeight);

		interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);
	}

	ParseWeaponSounds(wdTable);

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(customParams);

	// internal only
	isNulled = (strcasecmp(name.c_str(), "noweapon") == 0);
	isShield = (type == "Shield");
	noAutoTarget = (manualfire || interceptor || isShield);
	onlyForward = !turret && (projectileType != WEAPON_STARBURST_PROJECTILE);
}



void WeaponDef::ParseWeaponSounds(const LuaTable& wdTable) {
	LoadSound(wdTable, "soundStart" , fireSound);
	LoadSound(wdTable, "soundHitDry",  hitSound);
	LoadSound(wdTable, "soundHitWet",  hitSound);

	// FIXME: do we still want or need any of this?
	const bool forceSetVolume =
		(fireSound.getVolume(0) == -1.0f) ||
		( hitSound.getVolume(0) == -1.0f) ||
		( hitSound.getVolume(1) == -1.0f);

	if (!forceSetVolume)
		return;

	if (damages.GetDefault() <= 50.0f) {
		fireSound.setVolume(0, 5.0f);
		hitSound.setVolume(0, 5.0f);
		hitSound.setVolume(1, 5.0f);
		return;
	}

	const float fireSoundVolume = math::sqrt(damages.GetDefault() * 0.5f) * ((type == "LaserCannon")? 0.5f: 1.0f);
	const float hitSoundVolume = fireSoundVolume;

	if (fireSound.getVolume(0) == -1.0f)
		fireSound.setVolume(0, fireSoundVolume);
	if (hitSound.getVolume(0) == -1.0f)
		hitSound.setVolume(0, hitSoundVolume);
	if (hitSound.getVolume(1) == -1.0f)
		hitSound.setVolume(1, hitSoundVolume);
}



void WeaponDef::LoadSound(
	const LuaTable& wdTable,
	const std::string& soundKey,
	GuiSoundSet& soundData
) {
	switch (hashString(soundKey.c_str())) {
		case hashString("soundStart"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, ""), wdTable.GetFloat(soundKey + "Volume", -1.0f));
		} break;

		case hashString("soundHitDry"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, wdTable.GetString("soundHit", "")), wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", -1.0f)));
		} break;
		case hashString("soundHitWet"): {
			CommonDefHandler::AddSoundSetData(soundData, wdTable.GetString(soundKey, wdTable.GetString("soundHit", "")), wdTable.GetFloat(soundKey + "Volume", wdTable.GetFloat("soundHitVolume", -1.0f)));
		} break;

		default: {
		} break;
	}
}


S3DModel* WeaponDef::LoadModel()
{
	if (visuals.model != nullptr)
		return visuals.model;

	if (!visuals.modelName.empty())
		return (visuals.model = modelLoader.LoadModel(visuals.modelName));

	// not useful, too much spam
	// LOG_L(L_WARNING, "[WeaponDef::%s] weapon \"%s\" has no model defined", __FUNCTION__, name.c_str());
	return nullptr;
}

S3DModel* WeaponDef::LoadModel() const {
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

