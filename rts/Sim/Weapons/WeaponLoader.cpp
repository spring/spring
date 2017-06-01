/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponLoader.h"
#include "WeaponMemPool.h"
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

#include "Game/TraceRay.h" // Collision::*
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"

SimObjectMemPool<760> weaponMemPool;

void CWeaponLoader::InitStatic() { weaponMemPool.reserve(128); }
void CWeaponLoader::KillStatic() { weaponMemPool.clear(); }



void CWeaponLoader::LoadWeapons(CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;

	unit->weapons.reserve(unitDef->weapons.size());

	for (const UnitDefWeapon& defWeapon: unitDef->weapons) {
		CWeapon* weapon = LoadWeapon(unit, defWeapon.def);

		weapon->SetWeaponNum(unit->weapons.size());
		unit->weapons.push_back(weapon);
	}
}

void CWeaponLoader::InitWeapons(CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;

	for (size_t n = 0; n < unit->weapons.size(); n++) {
		InitWeapon(unit, unit->weapons[n], &unitDef->weapons[n]);
	}
}

void CWeaponLoader::FreeWeapons(CUnit* unit)
{
	for (CWeapon*& w: unit->weapons) {
		weaponMemPool.free(w);
	}

	unit->weapons.clear();
}



CWeapon* CWeaponLoader::LoadWeapon(CUnit* owner, const WeaponDef* weaponDef)
{
	const std::string& weaponType = weaponDef->type;

	if (StringToLower(weaponDef->name) == "noweapon")
		return (weaponMemPool.alloc<CNoWeapon>(owner, weaponDef));

	if (weaponType == "Cannon")
		return (weaponMemPool.alloc<CCannon>(owner, weaponDef));

	if (weaponType == "Rifle")
		return (weaponMemPool.alloc<CRifle>(owner, weaponDef));

	if (weaponType == "Melee")
		return (weaponMemPool.alloc<CMeleeWeapon>(owner, weaponDef));

	if (weaponType == "Shield")
		return (weaponMemPool.alloc<CPlasmaRepulser>(owner, weaponDef));

	if (weaponType == "Flame")
		return (weaponMemPool.alloc<CFlameThrower>(owner, weaponDef));

	if (weaponType == "MissileLauncher")
		return (weaponMemPool.alloc<CMissileLauncher>(owner, weaponDef));

	if (weaponType == "AircraftBomb")
		return (weaponMemPool.alloc<CBombDropper>(owner, weaponDef, false));

	if (weaponType == "TorpedoLauncher") {
		if (owner->unitDef->canfly && !weaponDef->submissile)
			return (weaponMemPool.alloc<CBombDropper>(owner, weaponDef, true));

		return (weaponMemPool.alloc<CTorpedoLauncher>(owner, weaponDef));
	}

	if (weaponType == "LaserCannon")
		return (weaponMemPool.alloc<CLaserCannon>(owner, weaponDef));

	if (weaponType == "BeamLaser")
		return (weaponMemPool.alloc<CBeamLaser>(owner, weaponDef));

	if (weaponType == "LightningCannon")
		return (weaponMemPool.alloc<CLightningCannon>(owner, weaponDef));

	if (weaponType == "EmgCannon")
		return (weaponMemPool.alloc<CEmgCannon>(owner, weaponDef));

	// NOTE: no special connection to UnitDef::canManualFire
	// (any type of weapon may be slaved to the button which
	// controls manual firing) or the CMD_MANUALFIRE command
	if (weaponType == "DGun")
		return (weaponMemPool.alloc<CDGunWeapon>(owner, weaponDef));

	if (weaponType == "StarburstLauncher")
		return (weaponMemPool.alloc<CStarburstLauncher>(owner, weaponDef));

	LOG_L(L_ERROR, "[%s] weapon-type \"%s\" unknown, using noweapon", __func__, weaponType.c_str());
	return (weaponMemPool.alloc<CNoWeapon>(owner, weaponDef));
}

void CWeaponLoader::InitWeapon(CUnit* owner, CWeapon* weapon, const UnitDefWeapon* defWeapon)
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
	if (defWeapon->slavedTo > 0 && defWeapon->slavedTo <= owner->weapons.size())
		weapon->slavedTo = owner->weapons[defWeapon->slavedTo - 1];

	weapon->heightBoostFactor = weaponDef->heightBoostFactor;
	weapon->collisionFlags = weaponDef->collisionFlags;

	weapon->avoidFlags |= (Collision::NONEUTRALS   * (!weaponDef->avoidNeutral));
	weapon->avoidFlags |= (Collision::NOFRIENDLIES * (!weaponDef->avoidFriendly));
	weapon->avoidFlags |= (Collision::NOFEATURES   * (!weaponDef->avoidFeature));
	weapon->avoidFlags |= (Collision::NOGROUND     * (!weaponDef->avoidGround));

	weapon->damages = DynDamageArray::IncRef(&weaponDef->damages);

	// store availability of {Query,AimFrom}Weapon script functions, etc
	weapon->Init();
	weapon->UpdateRange(weaponDef->range);

	owner->maxRange = std::max(weapon->range, owner->maxRange);
}

