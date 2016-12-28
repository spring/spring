/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHCACHE_H
#define PATHCACHE_H

#include <deque>

#include "IPath.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"

class CPathCache
{
public:
	CPathCache(int blocksX, int blocksZ);
	~CPathCache();

	struct CacheItem {
		IPath::SearchResult result;
		IPath::Path path;
		int2 strtBlock;
		int2 goalBlock;
		float goalRadius;
		int pathType;
	};

	void Update();
	bool AddPath(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType
	);

	const CacheItem& GetCachedPath(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType
	);

private:
	void RemoveFrontQueItem();

	std::uint64_t GetHash(
		const int2 strtBlk,
		const int2 goalBlk,
		std::uint32_t goalRadius,
		std::int32_t pathType
	) const;

	bool HashCollision(
		const CacheItem& ci,
		const int2 strtBlk,
		const int2 goalBlk,
		float goalRadius,
		int pathType
	) const;

	float GetCacheHitPercentage() const {
		if ((numCacheHits + numCacheMisses) == 0)
			return 0.0f;

		return ((numCacheHits / float(numCacheHits + numCacheMisses)) * 100.0f);
	}

private:
	struct CacheQueItem {
		std::int32_t timeout;
		std::uint64_t hash;
	};

	// returned on any cache-miss
	CacheItem dummyCacheItem;

	std::deque<CacheQueItem> cacheQue;
	spring::unordered_map<std::uint64_t, CacheItem> cachedPaths; // ints are sync-safe keys

	std::uint32_t numBlocksX;
	std::uint32_t numBlocksZ;
	std::uint64_t numBlocks;

	std::uint64_t maxCacheSize;
	std::uint32_t numCacheHits;
	std::uint32_t numCacheMisses;
	std::uint32_t numHashCollisions;
};

#endif
