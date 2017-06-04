/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_MEMPOOL_H
#define WEAPON_MEMPOOL_H

#include "Sim/Misc/SimObjectMemPool.h"
#include "Sim/Misc/GlobalConstants.h"

extern SimObjectStaticMemPool<MAX_UNITS * MAX_WEAPONS_PER_UNIT, 760> weaponMemPool;

#endif

