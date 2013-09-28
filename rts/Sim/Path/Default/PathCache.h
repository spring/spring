/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHCACHE_H
#define PATHCACHE_H

#include <map>
#include <list>

#include "IPath.h"
#include "System/type2.h"

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

	const CacheItem* GetCachedPath(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType
	);

private:
	void RemoveFrontQueItem();

	boost::uint64_t GetHash(
		const int2 strtBlk,
		const int2 goalBlk,
		boost::uint32_t goalRadius,
		boost::int32_t pathType
	) const;

	bool HashCollision(
		const CacheItem* ci,
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
	struct CacheQue {
		boost::int32_t timeout;
		boost::uint64_t hash;
	};

	std::list<CacheQue> cacheQue;
	std::map<boost::uint64_t, CacheItem*> cachedPaths;

	typedef std::map<boost::uint64_t, CacheItem*>::const_iterator CachedPathConstIter;

	boost::uint32_t numBlocksX;
	boost::uint32_t numBlocksZ;
	boost::uint64_t numBlocks;

	boost::uint64_t maxCacheSize;
	boost::uint32_t numCacheHits;
	boost::uint32_t numCacheMisses;
	boost::uint32_t numHashCollisions;
};

#endif
