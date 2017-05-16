/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_MEMPOOL_H
#define FEATURE_MEMPOOL_H

#include "Feature.h"
#include "Sim/Misc/SimObjectMemPool.h"

extern SimObjectMemPool<sizeof(CFeature)> featureMemPool;

#endif

