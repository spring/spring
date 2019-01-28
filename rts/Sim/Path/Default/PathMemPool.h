/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_MEMPOOL_H
#define PATH_MEMPOOL_H

#include "PathCache.h"
#include "PathEstimator.h"
#include "PathFinder.h"
#include "System/MemPoolTypes.h"

typedef DynMemPool<sizeof(CPathCache    )> PCMemPool;
typedef DynMemPool<sizeof(CPathEstimator)> PEMemPool;
typedef DynMemPool<sizeof(CPathFinder   )> PFMemPool;

extern PCMemPool pcMemPool;
extern PEMemPool peMemPool;
extern PFMemPool pfMemPool;

#endif

