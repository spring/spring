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
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"

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

	for (const UnitDefWeapon& defWeapon: defWeapons) {
		CWeapon* weapon = LoadWeapon(unit, &defWeapon);
		weapons.push_back(InitWeapon(unit, weapon, &defWeapon));
		unit->maxRange = std::max(weapon->range, unit->maxRange);
	}
}



CWeapon* CWeaponLoader::LoadWeapon(CUnit* owner, const UnitDefWeapon* defWeapon)
{
	CWeapon* weapon = NULL;

	const WeaponDef* weaponDef = defWeapon->def;
	const std::string& weaponType = weaponDef->type;

	if (StringToLower(weaponDef->name) == "noweapon") {
		weapon = new CNoWeapon(owner, weaponDef);
	} else if (weaponType == "Cannon") {
		weapon = new CCannon(owner, weaponDef);
	} else if (weaponType == "Rifle") {
		weapon = new CRifle(owner, weaponDef);
	} else if (weaponType == "Melee") {
		weapon = new CMeleeWeapon(owner, weaponDef);
	} else if (weaponType == "Shield") {
		weapon = new CPlasmaRepulser(owner, weaponDef);
	} else if (weaponType == "Flame") {
		weapon = new CFlameThrower(owner, weaponDef);
	} else if (weaponType == "MissileLauncher") {
		weapon = new CMissileLauncher(owner, weaponDef);
	} else if (weaponType == "AircraftBomb") {
		weapon = new CBombDropper(owner, weaponDef, false);
	} else if (weaponType == "TorpedoLauncher") {
		if (owner->unitDef->canfly && !weaponDef->submissile) {
			weapon = new CBombDropper(owner, weaponDef, true);
		} else {
			weapon = new CTorpedoLauncher(owner, weaponDef);
		}
	} else if (weaponType == "LaserCannon") {
		weapon = new CLaserCannon(owner, weaponDef);
	} else if (weaponType == "BeamLaser") {
		weapon = new CBeamLaser(owner, weaponDef);
	} else if (weaponType == "LightningCannon") {
		weapon = new CLightningCannon(owner, weaponDef);
	} else if (weaponType == "EmgCannon") {
		weapon = new CEmgCannon(owner, weaponDef);
	} else if (weaponType == "DGun") {
		// NOTE: no special connection to UnitDef::canManualFire
		// (any type of weapon may be slaved to the button which
		// controls manual firing) or the CMD_MANUALFIRE command
		weapon = new CDGunWeapon(owner, weaponDef);
	} else if (weaponType == "StarburstLauncher") {
		weapon = new CStarburstLauncher(owner, weaponDef);
	} else {
		weapon = new CNoWeapon(owner, weaponDef);
		LOG_L(L_ERROR, "weapon-type %s unknown, using noweapon", weaponType.c_str());
	}

	return weapon;
}

CWeapon* CWeaponLoader::InitWeapon(CUnit* owner, CWeapon* weapon, const UnitDefWeapon* defWeapon)
{
	const WeaponDef* weaponDef = defWeapon->def;

	weapon->reloadTime = std::max(1, int(weaponDef->reload * GAME_SPEED));
	weapon->projectileSpeed = weaponDef->projectilespeed;

	weapon->accuracyError = weaponDef->accuracy;
	weapon->sprayAngle = weaponDef->sprayAngle;

	weapon->salvoSize = weaponDef->salvosize;
	weapon->salvoDelay = int(weaponDef->salvodelay * GAME_SPEED);
	weapon->projectilesPerShot = weaponDef->projectilespershot;

	weapon->fireSoundId = weaponDef->fireSound.getID(0);
	weapon->fireSoundVolume = weaponDef->fireSound.getVolume(0);

	weapon->onlyForward = weaponDef->onlyForward;
	weapon->maxForwardAngleDif = math::cos(weaponDef->maxAngle);
	weapon->maxMainDirAngleDif = defWeapon->maxMainDirAngleDif;
	weapon->mainDir = defWeapon->mainDir;

	weapon->badTargetCategory = defWeapon->badTargetCat;
	weapon->onlyTargetCategory = defWeapon->onlyTargetCat;

	// can only slave to an already-loaded weapon
	if (defWeapon->slavedTo > 0 && defWeapon->slavedTo <= owner->weapons.size()) {
		weapon->slavedTo = owner->weapons[defWeapon->slavedTo - 1];
	}

	weapon->heightBoostFactor = weaponDef->heightBoostFactor;
	weapon->collisionFlags = weaponDef->collisionFlags;

	if (!weaponDef->avoidNeutral)  weapon->avoidFlags |= Collision::NONEUTRALS;
	if (!weaponDef->avoidFriendly) weapon->avoidFlags |= Collision::NOFRIENDLIES;
	if (!weaponDef->avoidFeature)  weapon->avoidFlags |= Collision::NOFEATURES;
	if (!weaponDef->avoidGround)   weapon->avoidFlags |= Collision::NOGROUND;

	weapon->damages = DynDamageArray::IncRef(&weaponDef->damages);

	weapon->SetWeaponNum(owner->weapons.size());
	weapon->Init();
	weapon->UpdateRange(weaponDef->range);
	return weapon;
}

