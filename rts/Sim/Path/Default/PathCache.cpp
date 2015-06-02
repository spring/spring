/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "PathCache.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Log/ILog.h"

#define MAX_CACHE_QUEUE_SIZE   200
#define MAX_PATH_LIFETIME_SECS   6
#define USE_NONCOLLIDABLE_HASH   1

CPathCache::CPathCache(int blocksX, int blocksZ)
	: numBlocksX(blocksX)
	, numBlocksZ(blocksZ)
	, numBlocks(numBlocksX * numBlocksZ)

	, maxCacheSize(0)
	, numCacheHits(0)
	, numCacheMisses(0)
	, numHashCollisions(0)
{}

CPathCache::~CPathCache()
{
	LOG("[%s(%ux%u)] cacheHits=%u hitPercentage=%.0f%% numHashColls=%u maxCacheSize=%lu",
		__FUNCTION__, numBlocksX, numBlocksZ, numCacheHits, GetCacheHitPercentage(), numHashCollisions, maxCacheSize);

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

	const boost::uint64_t hash = GetHash(strtBlock, goalBlock, goalRadius, pathType);
	const boost::uint32_t cols = numHashCollisions;
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

	const int lifeTime = (result == IPath::Ok) ? GAME_SPEED * MAX_PATH_LIFETIME_SECS : GAME_SPEED * (MAX_PATH_LIFETIME_SECS / 2);

	CacheQue cq;
	cq.hash = hash;
	cq.timeout = gs->frameNum + lifeTime;

	cacheQue.push_back(cq);
	maxCacheSize = std::max<boost::uint64_t>(maxCacheSize, cacheQue.size());
	return false;
}

const CPathCache::CacheItem* CPathCache::GetCachedPath(
	const int2 strtBlock,
	const int2 goalBlock,
	float goalRadius,
	int pathType
) {
	const boost::uint64_t hash = GetHash(strtBlock, goalBlock, goalRadius, pathType);
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

boost::uint64_t CPathCache::GetHash(
	const int2 strtBlk,
	const int2 goalBlk,
	boost::uint32_t goalRadius,
	boost::int32_t pathType
) const {
	#define N numBlocks
	#define NX numBlocksX
	#define NZ numBlocksZ

	#ifndef USE_NONCOLLIDABLE_HASH
	// susceptible to collisions for given pathType and goalRadius:
	//   Hash(sb=< 8,18> gb=<17, 2> ...)==Hash(sb=< 9,18> gb=<15, 2> ...)
	//   Hash(sb=<11,10> gb=<17, 1> ...)==Hash(sb=<12,10> gb=<15, 1> ...)
	//   Hash(sb=<12,10> gb=<17, 2> ...)==Hash(sb=<13,10> gb=<15, 2> ...)
	//   Hash(sb=<13,10> gb=<15, 1> ...)==Hash(sb=<12,10> gb=<17, 1> ...)
	//   Hash(sb=<13,10> gb=<15, 3> ...)==Hash(sb=<12,10> gb=<17, 3> ...)
	//   Hash(sb=<12,18> gb=< 6,28> ...)==Hash(sb=<11,18> gb=< 8,28> ...)
	//
	const boost::uint32_t index = ((goalBlk.y * NX + goalBlk.x) * NZ + strtBlk.y) * NX;
	const boost::uint32_t offset = strtBlk.x * (pathType + 1) * std::max(1.0f, goalRadius);
	return (index + offset);
	#else
	// map into linear space, cannot collide unless given non-integer radii
	const boost::uint64_t index =
		(strtBlk.y * NX + strtBlk.x) +
		(goalBlk.y * NX + goalBlk.x) * N;
	const boost::uint64_t offset =
		pathType * N*N +
		goalRadius * N*N*N;
	return (index + offset);
	#endif

	#undef NZ
	#undef NX
	#undef N
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
			"[%s][f=%d][hash=%lu] Hash(sb=<%d,%d> gb=<%d,%d> gr=%.2f pt=%d)==Hash(sb=<%d,%d> gb=<%d,%d> gr=%.2f pt=%d)",
			__FUNCTION__, gs->frameNum,
			GetHash(strtBlk, goalBlk, goalRadius, pathType),
			ci->strtBlock.x, ci->strtBlock.y,
			ci->goalBlock.x, ci->goalBlock.y,
			ci->goalRadius, ci->pathType,
			strtBlk.x, strtBlk.y,
			goalBlk.x, goalBlk.y,
			goalRadius, pathType
		);
	}

	return hashColl;
}
