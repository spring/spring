/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_MEMPOOL_H
#define PROJECTILE_MEMPOOL_H

#include "Sim/Misc/SimObjectMemPool.h"
#include "Sim/Misc/GlobalConstants.h"

#if (defined(__x86_64) || defined(__x86_64__))
typedef StaticMemPool<MAX_PROJECTILES, 868> ProjMemPool;
#else
typedef DynMemPool<868> ProjMemPool;
#endif

extern ProjMemPool projMemPool;

#endif

