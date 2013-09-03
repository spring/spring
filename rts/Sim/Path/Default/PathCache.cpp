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
numCacheMisses(0)
{
}

CPathCache::~CPathCache()
{
	LOG("[%s] cache-hits=%i hit-percentage=%.0f%%", __FUNCTION__, numCacheHits, GetCacheHitPercentage());

	for (CachedPathConstIter ci = cachedPaths.begin(); ci != cachedPaths.end(); ++ci)
		delete (ci->second);
}

void CPathCache::AddPath(
	const IPath::Path* path,
	const IPath::SearchResult result,
	const int2 startBlock,
	const int2 goalBlock,
	float goalRadius,
	int pathType
) {
	if (cacheQue.size() > MAX_CACHE_QUEUE_SIZE)
		RemoveFrontQueItem();

	const unsigned int hash = GetHash(startBlock, goalBlock, goalRadius, pathType);

	if (cachedPaths.find(hash) != cachedPaths.end())
		return;

	CacheItem* ci = new CacheItem();
	ci->path       = *path; // copy
	ci->result     = result;
	ci->startBlock = startBlock;
	ci->goalBlock  = goalBlock;
	ci->goalRadius = goalRadius;
	ci->pathType   = pathType;

	cachedPaths[hash] = ci;

	CacheQue cq;
	cq.hash = hash;
	cq.timeout = gs->frameNum + GAME_SPEED * 7;

	cacheQue.push_back(cq);
}

const CPathCache::CacheItem* CPathCache::GetCachedPath(
	const int2 startBlock,
	const int2 goalBlock,
	float goalRadius,
	int pathType
) {
	const unsigned int hash = GetHash(startBlock, goalBlock, goalRadius, pathType);
	const CachedPathConstIter ci = cachedPaths.find(hash);

	if (ci == cachedPaths.end()) {
		++numCacheMisses; return NULL;
	}
	if (ci->second->startBlock.x != startBlock.x) {
		++numCacheMisses; return NULL;
	}
	if (ci->second->startBlock.y != startBlock.y) {
		++numCacheMisses; return NULL;
	}
	if (ci->second->goalBlock.x != goalBlock.x) {
		++numCacheMisses; return NULL;
	}
	#if 0
	if (ci->second->goalBlock.y != goalBlock.y) {
		++numCacheMisses; return NULL;
	}
	#endif
	if (ci->second->pathType != pathType) {
		++numCacheMisses; return NULL;
	}

	++numCacheHits;
	return (ci->second);
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

