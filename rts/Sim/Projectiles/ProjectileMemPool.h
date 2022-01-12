/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_MEMPOOL_H
#define PROJECTILE_MEMPOOL_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__) || defined(__e2k__) || defined(_M_X64))
typedef StaticMemPool<MAX_PROJECTILES, 888> ProjMemPool;
#else
typedef FixedDynMemPool<888, MAX_PROJECTILES / 2000, MAX_PROJECTILES / 64> ProjMemPool;
#endif

extern ProjMemPool projMemPool;

#endif

