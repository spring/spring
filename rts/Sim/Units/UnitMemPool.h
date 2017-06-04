/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_MEMPOOL_H
#define UNIT_MEMPOOL_H

#include "UnitTypes/Builder.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectMemPool.h"

// CBuilder is (currently) the largest derived unit-type
extern SimObjectStaticMemPool<MAX_UNITS, sizeof(CBuilder)> unitMemPool;

#endif

