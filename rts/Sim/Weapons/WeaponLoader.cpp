/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponLoader.h"
#include "WeaponDef.h"
#include "BeamLaser.h"
#include "BombDropper.h"
#include "Cannon.h"
#include "DGunWeapon.h"
#include "EmgCannon.h"
#include "FlameThrower.h"
#include "LaserCannon.h"
#include "LightningCannon.h"
#include "MeleeWeapon.h"
#include "MissileLauncher.h"
#include "NoWeapon.h"
#include "PlasmaRepulser.h"
#include "Rifle.h"
#include "StarburstLauncher.h"
#include "TorpedoLauncher.h"

#include "Game/TraceRay.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#define DEG2RAD(a) ((a) * (3.141592653f / 180.0f))

CWeaponLoader* CWeaponLoader::GetInstance()
{
	static CWeaponLoader instance;
	return &instance;
}



void CWeaponLoader::LoadWeapons(CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;

	      std::vector<CWeapon*>& weapons = unit->weapons;
	const std::vector<UnitDefWeapon>& defWeapons = unitDef->weapons;

	weapons.reserve(defWeapons.size());

	for (unsigned int i = 0; i < defWeapons.size(); i++) {
		const UnitDefWeapon* defWeapon = &defWeapons[i];
		CWeapon* weapon = LoadWeapon(unit, defWeapon);

		weapons.push_back(InitWeapon(unit, weapon, defWeapon));
	}
}



CWeapon* CWeaponLoader::LoadWeapon(CUnit* owner, const UnitDefWeapon* defWeapon)
{
	CWeapon* weapon = NULL;

	const WeaponDef* weaponDef = defWeapon->def;
	const std::string& weaponType = weaponDef->type;

	if (weaponType == "Cannon") {
		CCannon* cannon = new CCannon(owner);
		cannon->selfExplode = weaponDef->selfExplode;
		weapon = cannon;
	} else if (weaponType == "Rifle") {
		weapon = new CRifle(owner);
	} else if (weaponType == "Melee") {
		weapon = new CMeleeWeapon(owner);
	} else if (weaponType == "AircraftBomb") {
		weapon = new CBombDropper(owner, false);
	} else if (weaponType == "Shield") {
		weapon = new CPlasmaRepulser(owner);
	} else if (weaponType == "Flame") {
		weapon = new CFlameThrower(owner);
	} else if (weaponType == "MissileLauncher") {
		weapon = new CMissileLauncher(owner);
	} else if (weaponType == "TorpedoLauncher") {
		if (owner->unitDef->canfly && !weaponDef->submissile) {
			CBombDropper* bombDropper = new CBombDropper(owner, true);

			if (weaponDef->tracks)
				bombDropper->tracking = weaponDef->turnrate;

			bombDropper->bombMoveRange = weaponDef->range;
			weapon = bombDropper;
		} else {
			CTorpedoLauncher* torpLauncher = new CTorpedoLauncher(owner);

			if (weaponDef->tracks)
				torpLauncher->tracking = weaponDef->turnrate;

			weapon = torpLauncher;
		}
	} else if (weaponType == "LaserCannon") {
		CLaserCannon* laserCannon = new CLaserCannon(owner);
		laserCannon->color = weaponDef->visuals.color;
		weapon = laserCannon;
	} else if (weaponType == "BeamLaser") {
		CBeamLaser* beamLaser = new CBeamLaser(owner);
		beamLaser->color = weaponDef->visuals.color;
		weapon = beamLaser;
	} else if (weaponType == "LightningCannon") {
		CLightningCannon* lightningCannon = new CLightningCannon(owner);
		lightningCannon->color = weaponDef->visuals.color;
		weapon = lightningCannon;
	} else if (weaponType == "EmgCannon") {
		weapon = new CEmgCannon(owner);
	} else if (weaponType == "DGun") {
		// NOTE: no special connection to UnitDef::canManualFire
		// (any type of weapon may be slaved to the button which
		// controls manual firing) or the CMD_MANUALFIRE command
		weapon = new CDGunWeapon(owner);
	} else if (weaponType == "StarburstLauncher") {
		CStarburstLauncher* vLauncher = new CStarburstLauncher(owner);
		vLauncher->tracking = weaponDef->tracks? weaponDef->turnrate: 0;
		vLauncher->uptime = weaponDef->uptime * GAME_SPEED;
		weapon = vLauncher;
	} else {
		weapon = new CNoWeapon(owner);
		LOG_L(L_ERROR, "weapon-type %s unknown or NOWEAPON", weaponType.c_str());
	}

	weapon->weaponDef = weaponDef;
	return weapon;
}

CWeapon* CWeaponLoader::InitWeapon(CUnit* owner, CWeapon* weapon, const UnitDefWeapon* defWeapon)
{
	const WeaponDef* weaponDef = defWeapon->def;

	weapon->reloadTime = std::max(1, int(weaponDef->reload * GAME_SPEED));
	weapon->range = weaponDef->range;
	weapon->heightMod = weaponDef->heightmod;
	weapon->projectileSpeed = weaponDef->projectilespeed;

	weapon->damageAreaOfEffect = weaponDef->damageAreaOfEffect;
	weapon->craterAreaOfEffect = weaponDef->craterAreaOfEffect;
	weapon->accuracy = weaponDef->accuracy;
	weapon->sprayAngle = weaponDef->sprayAngle;

	weapon->stockpileTime = int(weaponDef->stockpileTime * GAME_SPEED);

	weapon->salvoSize = weaponDef->salvosize;
	weapon->salvoDelay = int(weaponDef->salvodelay * GAME_SPEED);
	weapon->projectilesPerShot = weaponDef->projectilespershot;

	weapon->metalFireCost = weaponDef->metalcost;
	weapon->energyFireCost = weaponDef->energycost;

	weapon->fireSoundId = weaponDef->fireSound.getID(0);
	weapon->fireSoundVolume = weaponDef->fireSound.getVolume(0);

	weapon->onlyForward = weaponDef->onlyForward;
	weapon->maxForwardAngleDif = math::cos(DEG2RAD(weaponDef->maxAngle));
	weapon->maxMainDirAngleDif = defWeapon->maxMainDirAngleDif;
	weapon->mainDir = defWeapon->mainDir;

	weapon->badTargetCategory = defWeapon->badTargetCat;
	weapon->onlyTargetCategory = defWeapon->onlyTargetCat;

	if (defWeapon->slavedTo) {
		const int index = (defWeapon->slavedTo - 1);

		// can only slave to an already-loaded weapon
		if ((index < 0) || (static_cast<size_t>(index) >= owner->weapons.size())) {
			throw content_error("Bad weapon slave in " + owner->unitDef->name);
		}

		weapon->slavedTo = owner->weapons[index];
	}

	weapon->fuelUsage = defWeapon->fuelUsage;
	weapon->targetBorder = weaponDef->targetBorder;
	weapon->cylinderTargeting = weaponDef->cylinderTargeting;
	weapon->minIntensity = weaponDef->minIntensity;
	weapon->heightBoostFactor = weaponDef->heightBoostFactor;
	weapon->collisionFlags = weaponDef->collisionFlags;

	if (!weaponDef->avoidNeutral)  weapon->avoidFlags |= Collision::NONEUTRALS;
	if (!weaponDef->avoidFriendly) weapon->avoidFlags |= Collision::NOFRIENDLIES;
	if (!weaponDef->avoidFeature)  weapon->avoidFlags |= Collision::NOFEATURES;
	if (!weaponDef->avoidGround)   weapon->avoidFlags |= Collision::NOGROUND;

	weapon->SetWeaponNum(owner->weapons.size());
	weapon->Init();

	return weapon;
}

