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
#include "System/Log/ILog.h"

static std::array<uint8_t, 2048> udWeaponCounts;

WeaponMemPool weaponMemPool;

static_assert((sizeof(UnitDef::weapons) / sizeof(UnitDef::weapons[0])) == MAX_WEAPONS_PER_UNIT, "");
static_assert(MAX_WEAPONS_PER_UNIT < std::numeric_limits<decltype(udWeaponCounts)::value_type>::max(), "");

void CWeaponLoader::InitStatic() { udWeaponCounts.fill(MAX_WEAPONS_PER_UNIT + 1); weaponMemPool.reserve(128); }
void CWeaponLoader::KillStatic() { udWeaponCounts.fill(MAX_WEAPONS_PER_UNIT + 1); weaponMemPool.clear(); }



void CWeaponLoader::LoadWeapons(CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;
	const UnitDefWeapon* udWeapons = &unitDef->GetWeapon(0);

	unsigned int i = 0;
	unsigned int n = 0;

	if ((n = udWeaponCounts.at(unitDef->id)) > MAX_WEAPONS_PER_UNIT)
		n = (udWeaponCounts.at(unitDef->id) = unitDef->NumWeapons());

	for (unit->weapons.reserve(n); i < n; i++) {
		CWeapon* weapon = LoadWeapon(unit, udWeapons[i].def);

		weapon->SetWeaponNum(unit->weapons.size());
		unit->weapons.push_back(weapon);
	}
}

void CWeaponLoader::InitWeapons(CUnit* unit)
{
	const UnitDef* unitDef = unit->unitDef;

	for (size_t n = 0; n < unit->weapons.size(); n++) {
		InitWeapon(unit, unit->weapons[n], &unitDef->GetWeapon(n));
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
	if (weaponDef->isNulled)
		return (weaponMemPool.alloc<CNoWeapon>(owner, weaponDef));

	switch (weaponDef->type[0]) {
		case 'C': { return (weaponMemPool.alloc<CCannon>(owner, weaponDef)); } break; // "Cannon"
		case 'R': { return (weaponMemPool.alloc<CRifle >(owner, weaponDef)); } break; // "Rifle"

		case 'M': {
			// "Melee" or "MissileLauncher"
			switch (weaponDef->type[1]) {
				case 'e': { return (weaponMemPool.alloc<CMeleeWeapon    >(owner, weaponDef)); } break;
				case 'i': { return (weaponMemPool.alloc<CMissileLauncher>(owner, weaponDef)); } break;
			}
		} break;
		case 'S': {
			// "Shield" or "StarburstLauncher"
			switch (weaponDef->type[1]) {
				case 'h': { return (weaponMemPool.alloc<CPlasmaRepulser   >(owner, weaponDef)); } break;
				case 't': { return (weaponMemPool.alloc<CStarburstLauncher>(owner, weaponDef)); } break;
			}
		} break;

		case 'F': { return (weaponMemPool.alloc<CFlameThrower>(owner, weaponDef       )); } break; // "Flame"
		case 'A': { return (weaponMemPool.alloc<CBombDropper >(owner, weaponDef, false)); } break; // "AircraftBomb"

		case 'T': {
			// "TorpedoLauncher"
			if (owner->unitDef->canfly && !weaponDef->submissile)
				return (weaponMemPool.alloc<CBombDropper>(owner, weaponDef, true));

			return (weaponMemPool.alloc<CTorpedoLauncher>(owner, weaponDef));
		} break;
		case 'L': {
			// "LaserCannon" or "LightningCannon"
			switch (weaponDef->type[1]) {
				case 'a': { return (weaponMemPool.alloc<CLaserCannon    >(owner, weaponDef)); } break;
				case 'i': { return (weaponMemPool.alloc<CLightningCannon>(owner, weaponDef)); } break;
			}
		} break;

		case 'B': { return (weaponMemPool.alloc<CBeamLaser>(owner, weaponDef)); } break; // "BeamLaser"
		case 'E': { return (weaponMemPool.alloc<CEmgCannon>(owner, weaponDef)); } break; // "EmgCannon"
		case 'D': {
			// "DGun"
			// NOTE: no special connection to UnitDef::canManualFire
			// (any type of weapon may be slaved to the button which
			// controls manual firing) or the CMD_MANUALFIRE command
			return (weaponMemPool.alloc<CDGunWeapon>(owner, weaponDef));
		} break;
		default: {} break;
	}

	LOG_L(L_ERROR, "[%s] unit \"%s\" has unknown weapon-type \"%s\", using NoWeapon", __func__, owner->unitDef->name.c_str(), weaponDef->type.c_str());
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
	weapon->avoidFlags |= (Collision::NOCLOAKED    * (!weaponDef->avoidCloaked));

	weapon->damages = DynDamageArray::IncRef(&weaponDef->damages);

	// store availability of {Query,AimFrom}Weapon script functions, etc
	weapon->Init();
	weapon->UpdateRange(weaponDef->range);

	owner->maxRange = std::max(weapon->range, owner->maxRange);
}

