/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Builder.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectMemPool.h"

#if (defined(__x86_64) || defined(__x86_64__))
// CBuilder is (currently) the largest derived unit-type
typedef StaticMemPool<MAX_UNITS, sizeof(CBuilder)> UnitMemPool;
#else
typedef DynMemPool<sizeof(CBuilder)> UnitMemPool;
#endif

extern UnitMemPool unitMemPool;

#endif

