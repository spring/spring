/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_MEMPOOL_H
#define WEAPON_MEMPOOL_H

#include "Weapon.h"
#include "Sim/Misc/SimObjectMemPool.h"

extern SimObjectMemPool<sizeof(CWeapon) + (sizeof(CWeapon) >> 1)> weaponMemPool;

#endif

