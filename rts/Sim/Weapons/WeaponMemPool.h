/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_MEMPOOL_H
#define WEAPON_MEMPOOL_H

#include "Sim/Misc/SimObjectMemPool.h"
#include "Sim/Misc/GlobalConstants.h"

#if (defined(__x86_64) || defined(__x86_64__))
// NOTE: ~742MB, way too big for 32-bit builds
typedef StaticMemPool<MAX_UNITS * MAX_WEAPONS_PER_UNIT, 760> WeaponMemPool;
#else
typedef DynMemPool<760> WeaponMemPool;
#endif

extern WeaponMemPool weaponMemPool;

#endif

