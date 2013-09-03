/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "PathCache.h"

#include "Sim/Misc/GlobalSynced.h"
#include "System/Log/ILog.h"

#define MAX_CACHE_QUEUE_SIZE 100

CPathCache::CPathCache(int blocksX, int blocksZ):
blocksX(blocksX),
blocksZ(blocksZ),

numCacheHits(0),
numCacheMisses(0),
numHashCollisions(0)
{
}

CPathCache::~CPathCache()
{
	LOG("[%s(%ix%i)] cache-hits=%i hit-percentage=%.0f%% collisions=%i",
		__FUNCTION__, blocksX, blocksZ, numCacheHits, GetCacheHitPercentage(), numHashCollisions);

	for (CachedPathConstIter iter = cachedPaths.begin(); iter != cachedPaths.end(); ++iter)
		delete (iter->second);
}

bool CPathCache::AddPath(
	const IPath::Path* path,
	const IPath::SearchResult result,
	const int2 strtBlock,
	const int2 goalBlock,
	float goalRadius,
	int pathType
) {
	if (cacheQue.size() > MAX_CACHE_QUEUE_SIZE)
		RemoveFrontQueItem();

	const unsigned int hash = GetHash(strtBlock, goalBlock, goalRadius, pathType);
	const unsigned int cols = numHashCollisions;
	const CachedPathConstIter iter = cachedPaths.find(hash);

	// register any hash collisions
	if (iter != cachedPaths.end()) {
		return ((numHashCollisions += HashCollision(iter->second, strtBlock, goalBlock, goalRadius, pathType)) != cols);
	}

	CacheItem* ci = new CacheItem();
	ci->path       = *path; // copy
	ci->result     = result;
	ci->strtBlock  = strtBlock;
	ci->goalBlock  = goalBlock;
	ci->goalRadius = goalRadius;
	ci->pathType   = pathType;

	cachedPaths[hash] = ci;

	CacheQue cq;
	cq.hash = hash;
	cq.timeout = gs->frameNum + GAME_SPEED * 7;

	cacheQue.push_back(cq);
	return false;
}

const CPathCache::CacheItem* CPathCache::GetCachedPath(
	const int2 strtBlock,
	const int2 goalBlock,
	float goalRadius,
	int pathType
) {
	const unsigned int hash = GetHash(strtBlock, goalBlock, goalRadius, pathType);
	const CachedPathConstIter iter = cachedPaths.find(hash);

	if (iter == cachedPaths.end()) {
		++numCacheMisses; return NULL;
	}
	if (iter->second->strtBlock != strtBlock) {
		++numCacheMisses; return NULL;
	}
	if (iter->second->goalBlock != goalBlock) {
		++numCacheMisses; return NULL;
	}
	if (iter->second->pathType != pathType) {
		++numCacheMisses; return NULL;
	}

	++numCacheHits;
	return (iter->second);
}

void CPathCache::Update()
{
	while (!cacheQue.empty() && (cacheQue.front().timeout) < gs->frameNum)
		RemoveFrontQueItem();
}

void CPathCache::RemoveFrontQueItem()
{
	const CachedPathConstIter it = cachedPaths.find((cacheQue.front()).hash);

	assert(it != cachedPaths.end());
	delete (it->second);

	cachedPaths.erase(it);
	cacheQue.pop_front();
}

bool CPathCache::HashCollision(
	const CacheItem* ci,
	const int2 strtBlk,
	const int2 goalBlk,
	float goalRadius,
	int pathType
) const {
	bool hashColl = false;

	hashColl |= (ci->strtBlock != strtBlk || ci->goalBlock != goalBlk);
	hashColl |= (ci->pathType != pathType || ci->goalRadius != goalRadius);

	if (hashColl) {
		LOG_L(L_DEBUG,
			"[%s][f=%d][hash=%u] Hash(sb=<%d,%d> gb=<%d,%d> gr=%.2f pt=%d)==Hash(sb=<%d,%d> gb=<%d,%d> gr=%.2f pt=%d)",
			__FUNCTION__, gs->frameNum,
			GetHash(strtBlock, goalBlock, goalRadius, pathType);
			ci->strtBlock.x, ci->strtBlock.y,
			ci->goalBlock.x, ci->goalBlock.y,
			ci->goalRadius, ci->pathType,
			strtBlck.x, strtBlck.y,
			goalBlck.x, goalBlck.y,
			goalRadius, pathType
		);
	}

	return hashColl;
}
