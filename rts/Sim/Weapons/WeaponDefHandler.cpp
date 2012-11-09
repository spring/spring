/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include "WeaponDefHandler.h"

#include "Game/Game.h"
#include "Game/TraceRay.h"
#include "Lua/LuaParser.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/DamageArrayHandler.h"
#include "Sim/Misc/DefinitionTag.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"


CR_BIND(WeaponDef, );


CWeaponDefHandler* weaponDefHandler = NULL;


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
WEAPONDUMMYTAG(bool, collideEnemy);
WEAPONDUMMYTAG(bool, collideFriendly);
WEAPONDUMMYTAG(bool, collideFeature);
WEAPONDUMMYTAG(bool, collideNeutral);
WEAPONDUMMYTAG(bool, collideGround);

// Damaging
WEAPONDUMMYTAG(table, damage);
WEAPONDUMMYTAG(float, damage.default);
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
WEAPONDUMMYTAG(float, heightMod);
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
WEAPONTAG(int, flighttime).defaultValue(0).scaleValue(32);
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
WEAPONDUMMYTAG(bool, isShield);
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
//FIXME move those to lua!
WEAPONDUMMYTAG(int, color);
WEAPONDUMMYTAG(int, color2);
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




CWeaponDefHandler::CWeaponDefHandler()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("WeaponDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}

	vector<string> weaponNames;
	rootTable.GetKeys(weaponNames);

	weaponDefs.resize(weaponNames.size());

	for (int wid = 0; wid < weaponDefs.size(); wid++) {
		WeaponDef& wd = weaponDefs[wid];
		wd.id = wid;
		wd.name = weaponNames[wid];
		weaponID[wd.name] = wid;

		const LuaTable wdTable = rootTable.SubTable(wd.name);
		ParseWeapon(wdTable, wd);
	}
}


CWeaponDefHandler::~CWeaponDefHandler()
{
}


void CWeaponDefHandler::ParseWeapon(const LuaTable& wdTable, WeaponDef& wd)
{
	WeaponDefs.Load(&wd, wdTable);

	wd.noAutoTarget = (wd.manualfire || wd.interceptor || wd.isShield);
	wd.onlyForward = !wd.turret && (wd.type != "StarburstLauncher");
	wd.restTime = 0.0f; //FIXME remove var?

	if (wdTable.KeyExists("cylinderTargetting"))
		LOG_L(L_WARNING, "WeaponDef cylinderTargetting is deprecated and will be removed in the next release (use cylinderTargeting).");


	if (!wd.paralyzer)
		wd.damages.paralyzeDamageTime = 0;

	//FIXME defaults depend on other tags
	{
		if (wd.paralyzer)
			wd.cameraShake = wdTable.GetFloat("cameraShake", 0.0f);

		if (wd.selfExplode)
			wd.predictBoost = wdTable.GetFloat("predictBoost", 0.5f);

		if (wd.type == "Melee") {
			wd.targetBorder = Clamp(wdTable.GetFloat("targetBorder", 1.0f), -1.0f, 1.0f);
			wd.cylinderTargeting = Clamp(wdTable.GetFloat("cylinderTargeting", wdTable.GetFloat("cylinderTargetting", 1.0f)), 0.0f, 128.0f);
		}
		
		if (wd.type == "Shield") {
			wd.isShield = true;
		}

		if (wd.type == "Flame") {
			//FIXME move to lua (for all other weapons this tag is named `duration` and has a different default)
			wd.duration = wdTable.GetFloat("flameGfxTime", 1.2f);
		}

		wd.shieldRechargeDelay = int(wdTable.GetFloat("rechargeDelay", wd.shieldRechargeDelay) * GAME_SPEED);
	}

	//FIXME may be smarter to merge the collideXYZ tags with avoidXYZ and removing the collisionFlags tag (and move the code into CWeapon)?
	if (!wdTable.GetBool("collideEnemy",    true)) { wd.collisionFlags |= Collision::NOENEMIES;    }
	if (!wdTable.GetBool("collideFriendly", true)) { wd.collisionFlags |= Collision::NOFRIENDLIES; }
	if (!wdTable.GetBool("collideFeature",  true)) { wd.collisionFlags |= Collision::NOFEATURES;   }
	if (!wdTable.GetBool("collideNeutral",  true)) { wd.collisionFlags |= Collision::NONEUTRALS;   }
	if (!wdTable.GetBool("collideGround",   true)) { wd.collisionFlags |= Collision::NOGROUND;     }

	// setup the default damages
	{
		const LuaTable dmgTable = wdTable.SubTable("damage");
		float defDamage = dmgTable.GetFloat("default", 0.0f);
		if (defDamage == 0.0f) {
			defDamage = 1.0f; //avoid division by zeros
		}
		wd.damages = DamageArray(defDamage);

		map<string, float> damages;
		dmgTable.GetMap(damages);

		map<string, float>::const_iterator di;
		for (di = damages.begin(); di != damages.end(); ++di) {
			const int type = damageArrayHandler->GetTypeFromName(di->first);
			if (type != 0) {
				float dmg = di->second;
				if (dmg != 0.0f) {
					wd.damages[type] = dmg;
				} else {
					wd.damages[type] = 1.0f;
				}
			}
		}

		const float tempsize = 2.0f + std::min(defDamage * 0.0025f, wd.damageAreaOfEffect * 0.1f);
		wd.size = wdTable.GetFloat("size", tempsize);

		const float gd = std::max(30.0f, defDamage / 20.0f);
		const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (math::sqrt(gd) * 0.7f)) * 0.5f;
		wd.explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);
	}

	{
		// 0.78.2.1 backwards compatibility: non-burst beamlasers play one
		// sample per shot, not for each individual beam making up the shot
		const bool singleSampleShot = (wd.type == "BeamLaser" && !wd.beamburst);
		const bool singleShotWeapon = (wd.type == "Melee" || wd.type == "Rifle");

		wd.soundTrigger = wdTable.GetBool("soundTrigger", singleSampleShot || singleShotWeapon);
	}

	// get some weapon specific defaults
	int defInterceptType = 0;
	if ((wd.type == "Cannon") || (wd.type == "EmgCannon")) {
		defInterceptType = 1;
	} else if ((wd.type == "LaserCannon") || (wd.type == "BeamLaser")) {
		defInterceptType = 2;
	} else if ((wd.type == "StarburstLauncher") || (wd.type == "MissileLauncher")) {
		defInterceptType = 4;
	} else if (wd.type == "AircraftBomb") {
		defInterceptType = 8;
	} else if (wd.type == "Flame") {
		defInterceptType = 16;
	} else if (wd.type == "TorpedoLauncher") {
		defInterceptType = 32;
	} else if (wd.type == "LightningCannon") {
		defInterceptType = 64;
	} else if (wd.type == "Rifle") {
		defInterceptType = 128;
	} else if (wd.type == "Melee") {
		defInterceptType = 256;
	}
	wd.interceptedByShieldType = wdTable.GetInt("interceptedByShieldType", defInterceptType);

	if (wd.type == "Cannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.8f);
	} else if (wd.type == "BeamLaser" || wd.type == "LightningCannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 1.0f);
	} else {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.2f);
	}

	if (wd.type == "Cannon") {
		// CExplosiveProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
		wd.intensity = wdTable.GetFloat("intensity", 0.2f);
	} else if (wd.type == "Rifle") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.9f);
	} else if (wd.type == "Melee") {
		// ...
	} else if (wd.type == "Flame") {
		// CFlameProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.2f);
		wd.collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (wd.type == "MissileLauncher") {
		// CMissileProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (wd.type == "LaserCannon") {
		// CLaserProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
		wd.collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
	} else if (wd.type == "BeamLaser") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else if (wd.type == "LightningCannon") {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (wd.type == "EmgCannon") {
		// CEmgProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		wd.size = wdTable.GetFloat("size", 3.0f);
	} else if (wd.type == "DGun") {
		// CFireBallProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
		wd.collisionSize = wdTable.GetFloat("collisionSize", 10.0f);
	} else if (wd.type == "StarburstLauncher") {
		// CStarburstProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
	} else {
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.0f);
	}

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(wd.customParams);
	ParseWeaponVisuals(wdTable, wd);
	ParseWeaponSounds(wdTable, wd);
}

void CWeaponDefHandler::ParseWeaponVisuals(const LuaTable& wdTable, WeaponDef& wd) {
	const float color1Hue = wdTable.GetInt("color",  0) / 255.0f;
	const float color1Sat = wdTable.GetInt("color2", 0) / 255.0f;

	const float3 defColors[4] = {
		hs2rgb(color1Hue, color1Sat), // default rgbColor1 for all weapon-types except cannons
		float3(1.0f, 1.0f, 1.0f),     // default rgbColor2 for all weapon-types
		float3(1.0f, 0.5f, 0.0f),     // default rgbColor1 for Cannons
		float3(0.9f, 0.9f, 0.2f),     // default rgbColor1 for EMGCannons
	};

	WeaponDef::Visuals& visuals = wd.visuals;
	visuals.color  = wdTable.GetFloat3("rgbColor",  defColors[0]);
	visuals.color2 = wdTable.GetFloat3("rgbColor2", defColors[1]);
	if (wd.type ==    "Cannon") { visuals.color = wdTable.GetFloat3("rgbColor", defColors[2]); }
	if (wd.type == "EmgCannon") { visuals.color = wdTable.GetFloat3("rgbColor", defColors[3]); }

	const std::string& colormap = wdTable.GetString("colormap", "");
	if (!colormap.empty()) {
		visuals.colorMap = CColorMap::LoadFromDefString(colormap);
	} else {
		visuals.colorMap = NULL;
	}
}

void CWeaponDefHandler::ParseWeaponSounds(const LuaTable& wdTable, WeaponDef& wd) {
	LoadSound(wdTable, "soundStart",  0, wd.fireSound.sounds);
	LoadSound(wdTable, "soundHitDry", 0, wd.hitSound.sounds);
	LoadSound(wdTable, "soundHitWet", 1, wd.hitSound.sounds);

	// FIXME: do we still want or need any of this?
	const bool forceSetVolume =
		(wd.fireSound.getVolume(0) == -1.0f) ||
		(wd.hitSound.getVolume(0) == -1.0f)  ||
		(wd.hitSound.getVolume(1) == -1.0f);

	if (forceSetVolume) {
		if (wd.damages[0] <= 50.0f) {
			wd.fireSound.setVolume(0, 5.0f);
			wd.hitSound.setVolume(0, 5.0f);
			wd.hitSound.setVolume(1, 5.0f);
		} else {
			float fireSoundVolume = math::sqrt(wd.damages[0] * 0.5f);

			if (wd.type == "LaserCannon") {
				fireSoundVolume *= 0.5f;
			}

			float hitSoundVolume = fireSoundVolume;

			if ((fireSoundVolume > 100.0f) &&
			    ((wd.type == "MissileLauncher") ||
			     (wd.type == "StarburstLauncher"))) {
				fireSoundVolume = 10.0f * math::sqrt(hitSoundVolume);
			}

			if (wd.damageAreaOfEffect > 8.0f) {
				hitSoundVolume *= 2.0f;
			}
			if (wd.type == "DGun") {
				hitSoundVolume *= 0.15f;
			}

			if (wd.fireSound.getVolume(0) == -1.0f) { wd.fireSound.setVolume(0, fireSoundVolume); }
			if (wd.hitSound.getVolume(0) == -1.0f) { wd.hitSound.setVolume(0, hitSoundVolume); }
			if (wd.hitSound.getVolume(1) == -1.0f) { wd.hitSound.setVolume(1, hitSoundVolume); }
		}
	}
}



void CWeaponDefHandler::LoadSound(
	const LuaTable& wdTable,
	const string& soundKey,
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

	if ((id = LoadSoundFile(name)) > 0) {
		soundData[soundIdx] = GuiSoundSet::Data(name, id, volume);
	}
}


const WeaponDef *CWeaponDefHandler::GetWeapon(std::string weaponname) const
{
	StringToLowerInPlace(weaponname);

	std::map<std::string,int>::const_iterator ii = weaponID.find(weaponname);
	if (ii == weaponID.end())
		return NULL;

	return &weaponDefs[ii->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponById(int weaponDefId) const
{
	if ((weaponDefId < 0) || (weaponDefId >= weaponDefs.size())) {
		return NULL;
	}
	return &weaponDefs[weaponDefId];
}



DamageArray CWeaponDefHandler::DynamicDamages(const DamageArray damages, const float3 startPos, const float3 curPos, const float range, const float exp, const float damageMin, const bool inverted)
{
	DamageArray dynDamages(damages);

	const float travDist  = std::min(range, curPos.distance2D(startPos));
	const float damageMod = 1.0f - math::pow(1.0f / range * travDist, exp);
	const float ddmod     = damageMin / damages[0]; // get damage mod from first damage type

	if (inverted) {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damages[i] - damageMod * damages[i];

			if (damageMin > 0)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// to prevent div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	}
	else {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damageMod * damages[i];

			if (damageMin > 0)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	}
	return dynDamages;
}
