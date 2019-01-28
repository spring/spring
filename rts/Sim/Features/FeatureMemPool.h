/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATURE_MEMPOOL_H
#define FEATURE_MEMPOOL_H

#include "Feature.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"

#if (defined(__x86_64) || defined(__x86_64__))
typedef StaticMemPool<MAX_FEATURES, sizeof(CFeature)> FeatureMemPool;
#else
typedef FixedDynMemPool<sizeof(CFeature), MAX_FEATURES / 1000, MAX_FEATURES / 32> FeatureMemPool;
#endif

extern FeatureMemPool featureMemPool;

#endif

