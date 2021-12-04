/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_MEMPOOL_H
#define FEATURE_MEMPOOL_H

#include "Feature.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectMemPool.h"

#if (defined(__x86_64) || defined(__x86_64__))
typedef StaticMemPool<MAX_FEATURES, sizeof(CFeature)> FeatureMemPool;
#else
typedef DynMemPool<sizeof(CFeature)> FeatureMemPool;
#endif

extern FeatureMemPool featureMemPool;

#endif

