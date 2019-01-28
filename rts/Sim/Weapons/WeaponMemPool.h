/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_MEMPOOL_H
#define WEAPON_MEMPOOL_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__))
// NOTE: ~742MB, way too big for 32-bit builds
typedef StaticMemPool<MAX_UNITS * MAX_WEAPONS_PER_UNIT, 760> WeaponMemPool;
#else
typedef FixedDynMemPool<760, (MAX_UNITS * MAX_WEAPONS_PER_UNIT) / 4000, (MAX_UNITS * MAX_WEAPONS_PER_UNIT) / 256> WeaponMemPool;
#endif

extern WeaponMemPool weaponMemPool;

#endif

