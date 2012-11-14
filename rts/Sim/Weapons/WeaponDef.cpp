/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponDef.h"

#include "Game/TraceRay.h"
#include "Rendering/Models/IModelParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Misc/CommonDefHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
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
WEAPONTAG(std::string, cegTag).defaultValue("").description("The name, without prefixes, of a CEG to be emitted by the projectile each frame.");
WEAPONTAG(int, tdfId).externalName("id").defaultValue(0);
WEAPONDUMMYTAG(table, customParams);

// Collision & Avoidance
WEAPONTAG(bool, avoidFriendly).defaultValue(true);
WEAPONTAG(bool, avoidFeature).defaultValue(true);
WEAPONTAG(bool, avoidNeutral).defaultValue(false);
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
WEAPONTAG(float, intensity).defaultValue(0.9f);
WEAPONTAG(float, minIntensity).defaultValue(0.0f);
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

// Crater & Impuls
WEAPONTAG(float, impulseFactor, damages.impulseFactor).defaultValue(1.0f);
WEAPONTAG(float, impulseBoost, damages.impulseBoost).defaultValue(0.0f);
WEAPONTAG(float, craterMult, damages.craterMult).fallbackName("impulseFactor").defaultValue(1.0f);
WEAPONTAG(float, craterBoost, damages.craterBoost).defaultValue(0.0f);
WEAPONTAG(float, craterAreaOfEffect).externalName("areaOfEffect").defaultValue(8.0f).scaleValue(0.5f);

// Water
WEAPONTAG(bool, waterweapon).defaultValue(false);
WEAPONTAG(bool, fireSubmersed).fallbackName("waterweapon").defaultValue(false);
WEAPONTAG(bool, submissile).defaultValue(false);

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
WEAPONTAG(float, duration).defaultValue(0.05f);
WEAPONTAG(float, beamtime).defaultValue(1.0f);
WEAPONTAG(bool, beamburst).defaultValue(false);
WEAPONTAG(int, beamLaserTTL).externalName("beamTTL").defaultValue(0);
WEAPONTAG(bool, sweepFire).defaultValue(false).description("Makes the laser continue firing while it aims for a new target, 'sweeping' across the terrain. Somewhat unreliable.");
WEAPONTAG(bool, largeBeamLaser).defaultValue(false);

// FLAMETHROWER
WEAPONTAG(float, sizeGrowth).defaultValue(0.5f);
WEAPONDUMMYTAG(float, flameGfxTime);

// Eco
WEAPONTAG(float, metalcost).externalName("metalPerShot").defaultValue(0.0f);
WEAPONTAG(float, energycost).externalName("energyPerShot").defaultValue(0.0f);

// Other Properties
WEAPONTAG(float, fireStarter).defaultValue(0.0f).scaleValue(0.01f);
WEAPONTAG(bool, paralyzer).defaultValue(false);
WEAPONTAG(int, paralyzeTime,  damages.paralyzeDamageTime).defaultValue(10).minimumValue(0);
WEAPONTAG(bool, stockpile).defaultValue(false);
WEAPONTAG(float, stockpileTime).fallbackName("reload").defaultValue(1.0f);
WEAPONTAG(float, coverageRange).externalName("coverage").defaultValue(0.0f);

// Interceptor
WEAPONTAG(int, targetable).defaultValue(0);
WEAPONTAG(int, interceptor).defaultValue(0);
WEAPONDUMMYTAG(unsigned, interceptedByShieldType);

// Dynamic Damage
WEAPONTAG(bool, dynDamageInverted).defaultValue(false);
WEAPONTAG(float, dynDamageExp).defaultValue(0.0f);
WEAPONTAG(float, dynDamageMin).defaultValue(0.0f);
WEAPONTAG(float, dynDamageRange).defaultValue(0.0f);

// Shield
WEAPONTAG(bool, shieldRepulser).externalName("shield.repulser").fallbackName("shieldRepulser").defaultValue(false);
WEAPONTAG(bool, smartShield).externalName("shield.smart").fallbackName("smartShield").defaultValue(false);
WEAPONTAG(bool, exteriorShield).externalName("shield.exterior").fallbackName("exteriorShield").defaultValue(false);
WEAPONTAG(bool, visibleShield).externalName("shield.visible").fallbackName("visibleShield").defaultValue(false);
WEAPONTAG(bool, visibleShieldRepulse).externalName("shield.visibleRepulse").fallbackName("visibleShieldRepulse").defaultValue(false);
WEAPONTAG(int, visibleShieldHitFrames).externalName("visibleHitFrames").fallbackName("visibleShieldHitFrames").defaultValue(0);
WEAPONTAG(float, shieldEnergyUse).externalName("shield.energyUse").fallbackName("shieldEnergyUse").defaultValue(0.0f);
WEAPONTAG(float, shieldForce).externalName("shield.force").fallbackName("shieldForce").defaultValue(0.0f);
WEAPONTAG(float, shieldRadius).externalName("shield.radius").fallbackName("shieldRadius").defaultValue(0.0f);
WEAPONTAG(float, shieldMaxSpeed).externalName("shield.maxSpeed").fallbackName("shieldMaxSpeed").defaultValue(0.0f);
WEAPONTAG(float, shieldPower).externalName("shield.power").fallbackName("shieldPower").defaultValue(0.0f);
WEAPONTAG(float, shieldPowerRegen).externalName("shield.powerRegen").fallbackName("shieldPowerRegen").defaultValue(0.0f);
WEAPONTAG(float, shieldPowerRegenEnergy).externalName("shield.powerRegenEnergy").fallbackName("shieldPowerRegenEnergy").defaultValue(0.0f);
WEAPONDUMMYTAG(float, shieldRechargeDelay).externalName("rechargeDelay").fallbackName("shieldRechargeDelay").defaultValue(0).scaleValue(GAME_SPEED); // must be read as float
WEAPONTAG(float, shieldStartingPower).externalName("shield.startingPower").fallbackName("shieldStartingPower").defaultValue(0.0f);
WEAPONTAG(unsigned int, shieldInterceptType).externalName("shield.interceptType").fallbackName("shieldInterceptType").defaultValue(0);
WEAPONTAG(float3, shieldBadColor).externalName("shield.badColor").fallbackName("shieldBadColor").defaultValue(float3(1.0f, 0.5f, 0.5f));
WEAPONTAG(float3, shieldGoodColor).externalName("shield.goodColor").fallbackName("shieldGoodColor").defaultValue(float3(0.5f, 0.5f, 1.0f));
WEAPONTAG(float, shieldAlpha).externalName("shield.alpha").fallbackName("shieldAlpha").defaultValue(0.2f);



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
WEAPONDUMMYTAG(std::string, colormap);

WEAPONTAG(std::string, textures1, visuals.texNames[0]).externalName("textures.1").fallbackName("texture1").defaultValue("");
WEAPONTAG(std::string, textures2, visuals.texNames[1]).externalName("textures.2").fallbackName("texture2").defaultValue("");
WEAPONTAG(std::string, textures3, visuals.texNames[2]).externalName("textures.3").fallbackName("texture3").defaultValue("");
WEAPONTAG(std::string, textures4, visuals.texNames[3]).externalName("textures.4").fallbackName("texture4").defaultValue("");
WEAPONTAG(std::string, explosionGenerator, visuals.expGenTag).defaultValue("");
WEAPONTAG(std::string, bounceExplosionGenerator, visuals.bounceExpGenTag).defaultValue("");

// Sound
WEAPONDUMMYTAG(bool, soundTrigger);
WEAPONDUMMYTAG(std::string, soundStart).defaultValue("");
WEAPONDUMMYTAG(float, soundStartVolume).defaultValue(-1.0f);
WEAPONDUMMYTAG(std::string, soundHitDry).fallbackName("soundHit").defaultValue("");
WEAPONDUMMYTAG(float, soundHitDryVolume).fallbackName("soundHitVolume").defaultValue(-1.0f);
WEAPONDUMMYTAG(std::string, soundHitWet).fallbackName("soundHit").defaultValue("");
WEAPONDUMMYTAG(float, soundHitWetVolume).fallbackName("soundHitVolume").defaultValue(-1.0f);


WeaponDef::WeaponDef()
{
	id = 0;
	collisionFlags = 0;
	explosionGenerator = NULL;
	bounceExplosionGenerator = NULL;

	const LuaTable wdTable;
	WeaponDefs.Load(this, wdTable);
}


WeaponDef::WeaponDef(const LuaTable& wdTable, const std::string& name_, int id_)
	: name(name_)
	, id(id_)
	, collisionFlags(0)
	, explosionGenerator(NULL)
	, bounceExplosionGenerator(NULL)
{
	WeaponDefs.Load(this, wdTable);

	noAutoTarget = (manualfire || interceptor || isShield);
	onlyForward = !turret && (type != "StarburstLauncher");

	if (wdTable.KeyExists("cylinderTargetting"))
		LOG_L(L_WARNING, "WeaponDef cylinderTargetting is deprecated and will be removed in the next release (use cylinderTargeting).");

	if (wdTable.KeyExists("color1") || wdTable.KeyExists("color2"))
		LOG_L(L_WARNING, "WeaponDef color1 & color2 (= hue & sat) are removed. Use rgbColor instead!");

	if (wdTable.KeyExists("isShield"))
		LOG_L(L_WARNING, "WeaponDef isShield is removed. Use weaponType=\"Shield\" instead!");

	if (!paralyzer)
		damages.paralyzeDamageTime = 0;

	shieldRechargeDelay = int(wdTable.GetFloat("rechargeDelay", shieldRechargeDelay) * GAME_SPEED);
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
		
		if (type == "Shield") {
			isShield = true;
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
		if (defDamage == 0.0f) {
			defDamage = 1.0f; //avoid division by zeros
		}
		damages.SetDefaultDamage(defDamage);

		std::map<string, float> dmgs;
		dmgTable.GetMap(dmgs);

		std::map<string, float>::const_iterator di;
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
		size = wdTable.GetFloat("size", tempsize);

		const float gd = std::max(30.0f, defDamage / 20.0f);
		const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (math::sqrt(gd) * 0.7f)) * 0.5f;
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
	if ((type == "Cannon") || (type == "EmgCannon")) {
		defInterceptType = 1;
	} else if ((type == "LaserCannon") || (type == "BeamLaser")) {
		defInterceptType = 2;
	} else if ((type == "StarburstLauncher") || (type == "MissileLauncher")) {
		defInterceptType = 4;
	} else if (type == "AircraftBomb") {
		defInterceptType = 8;
	} else if (type == "Flame") {
		defInterceptType = 16;
	} else if (type == "TorpedoLauncher") {
		defInterceptType = 32;
	} else if (type == "LightningCannon") {
		defInterceptType = 64;
	} else if (type == "Rifle") {
		defInterceptType = 128;
	} else if (type == "Melee") {
		defInterceptType = 256;
	}
	interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);

	if (type == "Cannon") {
		// CExplosiveProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
		intensity = wdTable.GetFloat("intensity", 0.2f);
	} else if (type == "Rifle") {
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
	} else if (type == "Melee") {
		// ...
	} else if (type == "Flame") {
		// CFlameProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.2f);
		collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (type == "MissileLauncher") {
		// CMissileProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (type == "LaserCannon") {
		// CLaserProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
		collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (type == "BeamLaser") {
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else if (type == "LightningCannon") {
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (type == "EmgCannon") {
		// CEmgProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		size = wdTable.GetFloat("size", 3.0f);
	} else if (type == "DGun") {
		// CFireBallProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
	} else if (type == "StarburstLauncher") {
		// CStarburstProjectile
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else {
		ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f);
	}

	const std::string& colormap = wdTable.GetString("colormap", "");
	if (!colormap.empty()) {
		visuals.colorMap = CColorMap::LoadFromDefString(colormap);
	} else {
		visuals.colorMap = NULL;
	}

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(customParams);

	ParseWeaponSounds(wdTable);
}


WeaponDef::~WeaponDef()
{
	if (explosionGenerator != NULL) {
		explGenHandler->UnloadGenerator(explosionGenerator);
		explosionGenerator = NULL;
	}
	if (bounceExplosionGenerator != NULL) {
		explGenHandler->UnloadGenerator(bounceExplosionGenerator);
		bounceExplosionGenerator = NULL;
	}
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

	if (forceSetVolume) {
		if (damages[0] <= 50.0f) {
			fireSound.setVolume(0, 5.0f);
			hitSound.setVolume(0, 5.0f);
			hitSound.setVolume(1, 5.0f);
		} else {
			float fireSoundVolume = math::sqrt(damages[0] * 0.5f);

			if (type == "LaserCannon") {
				fireSoundVolume *= 0.5f;
			}

			float hitSoundVolume = fireSoundVolume;

			if ((fireSoundVolume > 100.0f) &&
			    ((type == "MissileLauncher") ||
			     (type == "StarburstLauncher"))) {
				fireSoundVolume = 10.0f * math::sqrt(hitSoundVolume);
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

	if ((id = CommonDefHandler::LoadSoundFile(name)) > 0) {
		soundData[soundIdx] = GuiSoundSet::Data(name, id, volume);
	}
}


S3DModel* WeaponDef::LoadModel()
{
	if ((visuals.model == NULL) && !visuals.modelName.empty()) {
		visuals.model = modelParser->Load3DModel(visuals.modelName);
	} else {
		eventHandler.LoadedModelRequested();
	}

	return visuals.model;
}

S3DModel* WeaponDef::LoadModel() const {
	//not very sweet, but still better than replacing "const WeaponDef" _everywhere_
	return const_cast<WeaponDef*>(this)->LoadModel();
}

