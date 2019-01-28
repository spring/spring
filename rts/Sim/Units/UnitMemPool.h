/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Builder.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__))
// CBuilder is (currently) the largest derived unit-type
typedef StaticMemPool<MAX_UNITS, sizeof(CBuilder)> UnitMemPool;
#else
typedef FixedDynMemPool<sizeof(CBuilder), MAX_UNITS / 1000, MAX_UNITS / 32> UnitMemPool;
#endif

extern UnitMemPool unitMemPool;

#endif

