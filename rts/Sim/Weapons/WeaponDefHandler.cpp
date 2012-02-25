/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

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
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/ISound.h"

using std::min;
using std::max;

CR_BIND(WeaponDef, );


CWeaponDefHandler* weaponDefHandler = NULL;


CWeaponDefHandler::CWeaponDefHandler()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("WeaponDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading WeaponDefs");
	}

	vector<string> weaponNames;
	rootTable.GetKeys(weaponNames);

	numWeaponDefs = weaponNames.size();
	weaponDefs = new WeaponDef[numWeaponDefs];

	for (int wid = 0; wid < numWeaponDefs; wid++) {
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
	delete[] weaponDefs;
}


void CWeaponDefHandler::ParseWeapon(const LuaTable& wdTable, WeaponDef& wd)
{
	wd.tdfId = wdTable.GetInt("id", 0);

	wd.description = wdTable.GetString("name",     "Weapon");
	wd.cegTag      = wdTable.GetString("cegTag",   "");

	wd.avoidFriendly = wdTable.GetBool("avoidFriendly", true);
	wd.avoidFeature  = wdTable.GetBool("avoidFeature",  true);
	wd.avoidNeutral  = wdTable.GetBool("avoidNeutral",  false);

	//FIXME may be smarter to merge the collideXYZ tags with avoidXYZ and removing the collisionFlags tag (and move the code into CWeapon)?
	wd.collisionFlags = 0;

	if (!wdTable.GetBool("collideEnemy",    true)) { wd.collisionFlags |= Collision::NOENEMIES;    }
	if (!wdTable.GetBool("collideFriendly", true)) { wd.collisionFlags |= Collision::NOFRIENDLIES; }
	if (!wdTable.GetBool("collideFeature",  true)) { wd.collisionFlags |= Collision::NOFEATURES;   }
	if (!wdTable.GetBool("collideNeutral",  true)) { wd.collisionFlags |= Collision::NONEUTRALS;   }
	if (!wdTable.GetBool("collideGround",   true)) { wd.collisionFlags |= Collision::NOGROUND;     }

	wd.minIntensity = wdTable.GetFloat("minIntensity", 0.0f);
	wd.turret   = wdTable.GetBool("turret",      false);
	wd.highTrajectory = wdTable.GetInt("highTrajectory", 2);
	wd.noSelfDamage   = wdTable.GetBool("noSelfDamage", false);
	wd.impactOnly     = wdTable.GetBool("impactOnly",   false);

	wd.waterweapon   = wdTable.GetBool("waterWeapon",     false);
	wd.fireSubmersed = wdTable.GetBool("fireSubmersed",   wd.waterweapon);
	wd.submissile    = wdTable.GetBool("submissile",      false);
	wd.tracks        = wdTable.GetBool("tracks",          false);
	wd.fixedLauncher = wdTable.GetBool("fixedLauncher",   false);
	wd.noExplode     = wdTable.GetBool("noExplode",       false);
	wd.isShield      = wdTable.GetBool("isShield",        false);
	wd.beamtime      = wdTable.GetFloat("beamTime",       1.0f);
	wd.beamburst     = wdTable.GetBool("beamburst",       false);

	wd.waterBounce   = wdTable.GetBool("waterBounce",    false);
	wd.groundBounce  = wdTable.GetBool("groundBounce",   false);
	wd.bounceSlip    = wdTable.GetFloat("bounceSlip",    1.0f);
	wd.bounceRebound = wdTable.GetFloat("bounceRebound", 1.0f);
	wd.numBounce     = wdTable.GetInt("numBounce",       -1);

	wd.intensity      = wdTable.GetFloat("intensity",      0.9f);
	wd.duration       = wdTable.GetFloat("duration",       0.05f);
	wd.falloffRate    = wdTable.GetFloat("fallOffRate",    0.5f);

	wd.gravityAffected = wdTable.GetBool("gravityAffected", false);

	wd.type = wdTable.GetString("weaponType", "Cannon");

	wd.targetBorder = Clamp(wdTable.GetFloat("targetBorder", (wd.type == "Melee")? 1.0f : 0.0f), -1.0f, 1.0f);
	wd.cylinderTargetting = Clamp(wdTable.GetFloat("cylinderTargetting", (wd.type == "Melee")? 1.0f : 0.0f), 0.0f, 128.0f);

	wd.range = wdTable.GetFloat("range", 10.0f);

	const float accuracy       = wdTable.GetFloat("accuracy",   0.0f);
	const float sprayAngle     = wdTable.GetFloat("sprayAngle", 0.0f);
	const float movingAccuracy = wdTable.GetFloat("movingAccuracy", accuracy);

	// should really be tan but TA seem to cap it somehow
	// should also be 7fff or ffff theoretically but neither seems good
	wd.accuracy       = sin((accuracy)       * PI / 0xafff);
	wd.sprayAngle     = sin((sprayAngle)     * PI / 0xafff);
	wd.movingAccuracy = sin((movingAccuracy) * PI / 0xafff);

	wd.targetMoveError = wdTable.GetFloat("targetMoveError", 0.0f);
	wd.leadLimit = wdTable.GetFloat("leadLimit", -1.0f);
	wd.leadBonus = wdTable.GetFloat("leadBonus", 0.0f);

	// setup the default damages
	const LuaTable dmgTable = wdTable.SubTable("damage");
	float defDamage = dmgTable.GetFloat("default", 0.0f);
	if (defDamage == 0.0f) {
		defDamage = 1.0f; //avoid division by zeroes
	}
	for (int a = 0; a < damageArrayHandler->GetNumTypes(); ++a) {
		wd.damages[a] = defDamage;
	}

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

	wd.damages.impulseFactor = wdTable.GetFloat("impulseFactor", 1.0f);
	wd.damages.impulseBoost  = wdTable.GetFloat("impulseBoost",  0.0f);
	wd.damages.craterMult    = wdTable.GetFloat("craterMult",    wd.damages.impulseFactor);
	wd.damages.craterBoost   = wdTable.GetFloat("craterBoost",   0.0f);

	wd.damageAreaOfEffect = wdTable.GetFloat("areaOfEffect", 8.0f) * 0.5f;
	wd.craterAreaOfEffect = wdTable.GetFloat("craterAreaOfEffect", wd.damageAreaOfEffect);
	wd.edgeEffectiveness = wdTable.GetFloat("edgeEffectiveness", 0.0f);
	// prevent 0/0 division in CGameHelper::Explosion
	if (wd.edgeEffectiveness > 0.999f) {
		wd.edgeEffectiveness = 0.999f;
	}

	wd.projectilespeed = std::max(0.01f, wdTable.GetFloat("weaponVelocity", 0.0f) / GAME_SPEED);
	wd.startvelocity = max(0.01f, wdTable.GetFloat("startVelocity", 0.0f) / GAME_SPEED);
	wd.weaponacceleration = wdTable.GetFloat("weaponAcceleration", 0.0f) / GAME_SPEED / GAME_SPEED;
	wd.reload = wdTable.GetFloat("reloadTime", 1.0f);
	wd.salvodelay = wdTable.GetFloat("burstRate", 0.1f);
	wd.salvosize = wdTable.GetInt("burst", 1);
	wd.projectilespershot = wdTable.GetInt("projectiles", 1);
	wd.maxAngle = wdTable.GetFloat("tolerance", 3000.0f) * 180.0f / COBSCALEHALF;
	wd.restTime = 0.0f;
	wd.metalcost = wdTable.GetFloat("metalPerShot", 0.0f);
	wd.energycost = wdTable.GetFloat("energyPerShot", 0.0f);
	wd.selfExplode = wdTable.GetBool("burnblow", false);
	wd.predictBoost = wdTable.GetFloat("predictBoost", wd.selfExplode ? 0.5f : 0.0f);
	wd.sweepFire = wdTable.GetBool("sweepfire", false);
	wd.canAttackGround = wdTable.GetBool("canAttackGround", true);
	wd.myGravity = wdTable.GetFloat("myGravity", 0.0f);

	wd.fireStarter = wdTable.GetFloat("fireStarter", 0.0f) * 0.01f;
	wd.paralyzer = wdTable.GetBool("paralyzer", false);
	if (wd.paralyzer) {
		wd.damages.paralyzeDamageTime = max(0, wdTable.GetInt("paralyzeTime", 10));
	} else {
		wd.damages.paralyzeDamageTime = 0;
	}

	const float defShake = wd.paralyzer ? 0.0f : wd.damages.GetDefaultDamage();
	wd.cameraShake = wdTable.GetFloat("cameraShake", defShake);
	wd.cameraShake = max(0.0f, wd.cameraShake);

	// 0.78.2.1 backwards compatibility
	bool defaultSoundTrigger = (wd.type == "BeamLaser" && !wd.beamburst) || 
								wd.type == "Melee"  || wd.type == "Rifle";
	wd.soundTrigger = wdTable.GetBool("soundTrigger", defaultSoundTrigger);

	//sunparser->GetDef(wd.highTrajectory, "0", weaponname + "minbarrelangle");
	wd.stockpile     = wdTable.GetBool("stockpile", false);
	wd.stockpileTime = wdTable.GetFloat("stockpileTime", wd.reload);
	wd.interceptor   = wdTable.GetInt("interceptor", 0);
	wd.targetable    = wdTable.GetInt("targetable",  0);
	wd.manualfire    = wdTable.GetBool("commandfire", false);
	wd.coverageRange = wdTable.GetFloat("coverage", 0.0f);

	// FIXME -- remove the old style ?
	LuaTable shTable = wdTable.SubTable("shield");
	const float3 shieldBadColor (1.0f, 0.5f, 0.5f);
	const float3 shieldGoodColor(0.5f, 0.5f, 1.0f);
	if (shTable.IsValid()) {
		wd.shieldRepulser         = shTable.GetBool("repulser",       false);
		wd.smartShield            = shTable.GetBool("smart",          false);
		wd.exteriorShield         = shTable.GetBool("exterior",       false);
		wd.visibleShield          = shTable.GetBool("visible",        false);
		wd.visibleShieldRepulse   = shTable.GetBool("visibleRepulse", false);
		wd.visibleShieldHitFrames = shTable.GetInt("visibleHitFrames", 0);
		wd.shieldEnergyUse        = shTable.GetFloat("energyUse",        0.0f);
		wd.shieldForce            = shTable.GetFloat("force",            0.0f);
		wd.shieldRadius           = shTable.GetFloat("radius",           0.0f);
		wd.shieldMaxSpeed         = shTable.GetFloat("maxSpeed",         0.0f);
		wd.shieldPower            = shTable.GetFloat("power",            0.0f);
		wd.shieldPowerRegen       = shTable.GetFloat("powerRegen",       0.0f);
		wd.shieldPowerRegenEnergy = shTable.GetFloat("powerRegenEnergy", 0.0f);
		wd.shieldRechargeDelay    = (int)(shTable.GetFloat("rechargeDelay", 0) * GAME_SPEED);
		wd.shieldStartingPower    = shTable.GetFloat("startingPower",    0.0f);
		wd.shieldInterceptType    = shTable.GetInt("interceptType", 0);
		wd.shieldBadColor         = shTable.GetFloat3("badColor",  shieldBadColor);
		wd.shieldGoodColor        = shTable.GetFloat3("goodColor", shieldGoodColor);
		wd.shieldAlpha            = shTable.GetFloat("alpha", 0.2f);
	} else {
		wd.shieldRepulser         = wdTable.GetBool("shieldRepulser",       false);
		wd.smartShield            = wdTable.GetBool("smartShield",          false);
		wd.exteriorShield         = wdTable.GetBool("exteriorShield",       false);
		wd.visibleShield          = wdTable.GetBool("visibleShield",        false);
		wd.visibleShieldRepulse   = wdTable.GetBool("visibleShieldRepulse", false);
		wd.visibleShieldHitFrames = wdTable.GetInt("visibleShieldHitFrames", 0);
		wd.shieldEnergyUse        = wdTable.GetFloat("shieldEnergyUse",        0.0f);
		wd.shieldForce            = wdTable.GetFloat("shieldForce",            0.0f);
		wd.shieldRadius           = wdTable.GetFloat("shieldRadius",           0.0f);
		wd.shieldMaxSpeed         = wdTable.GetFloat("shieldMaxSpeed",         0.0f);
		wd.shieldPower            = wdTable.GetFloat("shieldPower",            0.0f);
		wd.shieldPowerRegen       = wdTable.GetFloat("shieldPowerRegen",       0.0f);
		wd.shieldPowerRegenEnergy = wdTable.GetFloat("shieldPowerRegenEnergy", 0.0f);
		wd.shieldRechargeDelay    = (int)(wdTable.GetFloat("shieldRechargeDelay", 0) * GAME_SPEED);
		wd.shieldStartingPower    = wdTable.GetFloat("shieldStartingPower",    0.0f);
		wd.shieldInterceptType    = wdTable.GetInt("shieldInterceptType", 0);
		wd.shieldBadColor         = wdTable.GetFloat3("shieldBadColor",  shieldBadColor);
		wd.shieldGoodColor        = wdTable.GetFloat3("shieldGoodColor", shieldGoodColor);
		wd.shieldAlpha            = wdTable.GetFloat("shieldAlpha", 0.2f);
	}


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

	wd.wobble = (wdTable.GetFloat("wobble", 0.0f) * TAANG2RAD) / GAME_SPEED;
	wd.dance = wdTable.GetFloat("dance", 0.0f) / GAME_SPEED;
	wd.trajectoryHeight = wdTable.GetFloat("trajectoryHeight", 0.0f);

	wd.noAutoTarget = (wd.manualfire || wd.interceptor || wd.isShield);
	wd.onlyTargetCategory = 0xffffffff;

	wd.largeBeamLaser = wdTable.GetBool("largeBeamLaser", false);

	if (wd.type == "Cannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.8f);
	} else if (wd.type == "BeamLaser" || wd.type == "LightningCannon") {
		wd.heightmod = wdTable.GetFloat("heightMod", 1.0f);
	} else {
		wd.heightmod = wdTable.GetFloat("heightMod", 0.2f);
	}

	wd.onlyForward = !wd.turret && (wd.type != "StarburstLauncher");
	wd.uptime = wdTable.GetFloat("weaponTimer", 0.0f);
	wd.flighttime = wdTable.GetFloat("flightTime", 0) * 32;
	wd.turnrate = (wdTable.GetFloat("turnRate", 0.0f) * TAANG2RAD) / GAME_SPEED;

	if ((wd.type == "AircraftBomb") && !wdTable.GetBool("manualBombSettings", false)) {
		// allow manually specifying burst and burstrate for AircraftBomb
		if (wd.reload < 0.5f) {
			wd.salvodelay = min(0.2f, wd.reload);
			wd.salvosize = (int)(1 / wd.salvodelay) + 1;
			wd.reload = 5;
		} else {
			wd.salvodelay = min(0.4f, wd.reload);
			wd.salvosize = 2;
		}
	}

	const float tempsize = 2.0f + std::min(wd.damages[0] * 0.0025f, wd.damageAreaOfEffect * 0.1f);
	wd.size = wdTable.GetFloat("size", tempsize);
	wd.sizeGrowth = wdTable.GetFloat("sizeGrowth", 0.2f);
	wd.collisionSize = wdTable.GetFloat("collisionSize", 0.05f);

	wd.heightBoostFactor = wdTable.GetFloat("heightBoostFactor", -1.0f);
	wd.proximityPriority = wdTable.GetFloat("proximityPriority", 1.0f);

	// get some weapon specific defaults
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
		wd.size              = wdTable.GetFloat("size",      tempsize);
		wd.sizeGrowth        = wdTable.GetFloat("sizeGrowth",    0.5f);
		wd.collisionSize     = wdTable.GetFloat("collisionSize", 0.5f);
		wd.duration          = wdTable.GetFloat("flameGfxTime",  1.2f);
	} else if (wd.type == "MissileLauncher") {
		// CMissileProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.5f);
	} else if (wd.type == "LaserCannon") {
		// CLaserProjectile
		wd.ownerExpAccWeight = wdTable.GetFloat("ownerExpAccWeight", 0.7f);
		wd.collisionSize = wdTable.GetFloat("collisionSize", 0.5f);
		wd.laserHardStop = wdTable.GetBool("hardstop", false);
	} else if (wd.type == "BeamLaser") {
		wd.beamLaserTTL      = wdTable.GetInt("beamTTL", 0);
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


	const float gd = max(30.0f, wd.damages[0] / 20.0f);
	const float defExpSpeed = (8.0f + (gd * 2.5f)) / (9.0f + (sqrt(gd) * 0.7f)) * 0.5f;
	wd.explosionSpeed = wdTable.GetFloat("explosionSpeed", defExpSpeed);

	// Dynamic Damage
	wd.dynDamageInverted = wdTable.GetBool("dynDamageInverted", false);
	wd.dynDamageExp      = wdTable.GetFloat("dynDamageExp",   0.0f);
	wd.dynDamageMin      = wdTable.GetFloat("dynDamageMin",   0.0f);
	wd.dynDamageRange    = wdTable.GetFloat("dynDamageRange", 0.0f);

	// custom parameters table
	wdTable.SubTable("customParams").GetMap(wd.customParams);
	ParseWeaponVisuals(wdTable, wd);
	ParseWeaponSounds(wdTable, wd);
}

void CWeaponDefHandler::ParseWeaponVisuals(const LuaTable& wdTable, WeaponDef& wd) {
	const float color1Hue = wdTable.GetInt("color",  0) / 255.0f;
	const float color1Sat = wdTable.GetInt("color2", 0) / 255.0f;

	const std::string& colormap = wdTable.GetString("colormap", "");
	const LuaTable& texTable = wdTable.SubTable("textures");

	const float3 defColors[4] = {
		hs2rgb(color1Hue, color1Sat), // default rgbColor1 for all weapon-types except cannons
		float3(1.0f, 1.0f, 1.0f),     // default rgbColor2 for all weapon-types
		float3(1.0f, 0.5f, 0.0f),     // default rgbColor1 for Cannons
		float3(0.9f, 0.9f, 0.2f),     // default rgbColor1 for EMGCannons
	};

	WeaponDef::Visuals& visuals = wd.visuals;

	visuals.modelName      = wdTable.GetString("model",       "");
	visuals.explosionScar  = wdTable.GetBool("explosionScar", true);
	visuals.smokeTrail     = wdTable.GetBool("smokeTrail",    false);
	visuals.alwaysVisible  = wdTable.GetBool("alwaysVisible", false);
	visuals.sizeDecay      = wdTable.GetFloat("sizeDecay",    0.0f);
	visuals.alphaDecay     = wdTable.GetFloat("alphaDecay",   1.0f);
	visuals.separation     = wdTable.GetFloat("separation",   1.0f);
	visuals.noGap          = wdTable.GetBool("noGap",         true);
	visuals.stages         = wdTable.GetInt("stages",         5);
	visuals.lodDistance    = wdTable.GetInt("lodDistance",      1000);
	visuals.thickness      = wdTable.GetFloat("thickness",      2.0f);
	visuals.corethickness  = wdTable.GetFloat("coreThickness",  0.25f);
	visuals.laserflaresize = wdTable.GetFloat("laserFlareSize", 15.0f);

	visuals.tilelength     = wdTable.GetFloat("tileLength", 200.0f);
	visuals.scrollspeed    = wdTable.GetFloat("scrollSpeed",  5.0f);
	visuals.pulseSpeed     = wdTable.GetFloat("pulseSpeed",   1.0f);
	visuals.beamdecay      = wdTable.GetFloat("beamDecay",    1.0f);
	visuals.color          = wdTable.GetFloat3("rgbColor",  defColors[0]);
	visuals.color2         = wdTable.GetFloat3("rgbColor2", defColors[1]);

	if (wd.type ==    "Cannon") { visuals.color = wdTable.GetFloat3("rgbColor", defColors[2]); }
	if (wd.type == "EmgCannon") { visuals.color = wdTable.GetFloat3("rgbColor", defColors[3]); }

	visuals.texNames[0]     = texTable.GetString(1, wdTable.GetString("texture1", ""));
	visuals.texNames[1]     = texTable.GetString(2, wdTable.GetString("texture2", ""));
	visuals.texNames[2]     = texTable.GetString(3, wdTable.GetString("texture3", ""));
	visuals.texNames[3]     = texTable.GetString(4, wdTable.GetString("texture4", ""));
	visuals.expGenTag       = wdTable.GetString("explosionGenerator", "");
	visuals.bounceExpGenTag = wdTable.GetString("bounceExplosionGenerator", "");

	if (!colormap.empty()) {
		visuals.colorMap = CColorMap::LoadFromDefString(colormap);
	} else {
		visuals.colorMap = NULL;
	}
}

void CWeaponDefHandler::ParseWeaponSounds(const LuaTable& wdTable, WeaponDef& wd) {
	LoadSound(wdTable, wd.firesound, "start");
	LoadSound(wdTable, wd.soundhit,  "hit");

	if ((wd.firesound.getVolume(0) == -1.0f) ||
	    (wd.soundhit.getVolume(0)  == -1.0f)) {
		// no volume (-1.0f) read from weapon definition, set it dynamically here
		if (wd.damages[0] <= 50.0f) {
			wd.soundhit.setVolume(0, 5.0f);
			wd.firesound.setVolume(0, 5.0f);
		}
		else {
			float soundVolume = sqrt(wd.damages[0] * 0.5f);

			if (wd.type == "LaserCannon") {
				soundVolume *= 0.5f;
			}

			float hitSoundVolume = soundVolume;

			if ((soundVolume > 100.0f) &&
			    ((wd.type == "MissileLauncher") ||
			     (wd.type == "StarburstLauncher"))) {
				soundVolume = 10.0f * sqrt(soundVolume);
			}

			if (wd.firesound.getVolume(0) == -1.0f) {
				wd.firesound.setVolume(0, soundVolume);
			}

			soundVolume = hitSoundVolume;

			if (wd.damageAreaOfEffect > 8.0f) {
				soundVolume *= 2.0f;
			}
			if (wd.type == "DGun") {
				soundVolume *= 0.15f;
			}
			if (wd.soundhit.getVolume(0) == -1.0f) {
				wd.soundhit.setVolume(0, soundVolume);
			}
		}
	}
}



void CWeaponDefHandler::LoadSound(const LuaTable& wdTable,
                                  GuiSoundSet& gsound, const string& soundCat)
{
	string name = "";
	float volume = -1.0f;

	if (soundCat == "start") {
		name   = wdTable.GetString("soundStart", "");
		volume = wdTable.GetFloat("soundStartVolume", -1.0f);
	}
	else if (soundCat == "hit") {
		name   = wdTable.GetString("soundHit", "");
		volume = wdTable.GetFloat("soundHitVolume", -1.0f);
	}

	if (name != "") {
		const int id = LoadSoundFile(name);
		if (id > 0)
		{
			GuiSoundSet::Data soundData(name, 0, volume);
			gsound.sounds.push_back(soundData);
			gsound.setID(0, id);
		}
	}
}


const WeaponDef *CWeaponDefHandler::GetWeapon(const string& weaponname2)
{
	string weaponname(StringToLower(weaponname2));

	map<string,int>::iterator ii=weaponID.find(weaponname);
	if(ii == weaponID.end())
		return NULL;

	return &weaponDefs[ii->second];
}


const WeaponDef* CWeaponDefHandler::GetWeaponById(int weaponDefId)
{
	if ((weaponDefId < 0) || (weaponDefId >= numWeaponDefs)) {
		return NULL;
	}
	return &weaponDefs[weaponDefId];
}



DamageArray CWeaponDefHandler::DynamicDamages(DamageArray damages, float3 startPos, float3 curPos, float range, float exp, float damageMin, bool inverted)
{
	DamageArray dynDamages(damages);

	curPos.y = 0;
	startPos.y = 0;

	float travDist = (curPos-startPos).Length()>range?range:(curPos-startPos).Length();
	float ddmod = 0;

	if (damageMin > 0)
		ddmod = damageMin/damages[0]; // get damage mod from first damage type

	if (inverted == true) {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damages[i] - (1 - pow(1 / range * travDist, exp)) * damages[i];

			if (damageMin > 0)
				dynDamages[i] = max(damages[i] * ddmod, dynDamages[i]);

			// to prevent div by 0
			dynDamages[i] = max(0.0001f, dynDamages[i]);
//			LOG_L(L_DEBUG, "D%i: %f (%f) - mod %f", i, dynDamages[i], damages[i], ddmod);
//			LOG_L(L_DEBUG, "F%i: %f - (1 - (1/%f * %f) ^ %f) * %f", i, damages[i], range, travDist, exp, damages[i]);
		}
	}
	else {
		for(int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = (1 - pow(1 / range * travDist, exp)) * damages[i];

			if (damageMin > 0)
				dynDamages[i] = max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = max(0.0001f, dynDamages[i]);
//			LOG_L(L_DEBUG, "D%i: %f (%f) - mod %f", i, dynDamages[i], damages[i], ddmod);
//			LOG_L(L_DEBUG, "F%i: (1 - (1/%f * %f) ^ %f) * %f", i, range, travDist, exp, damages[i]);
		}
	}
	return dynDamages;
}
