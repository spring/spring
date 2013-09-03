/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHCACHE_H
#define PATHCACHE_H

#include <map>
#include <list>

#include "IPath.h"
#include "System/Vec2.h"

class CPathCache
{
public:
	CPathCache(int blocksX, int blocksZ);
	~CPathCache();

	struct CacheItem {
		IPath::SearchResult result;
		IPath::Path path;
		int2 startBlock;
		int2 goalBlock;
		float goalRadius;
		int pathType;
	};

	void Update();
	void AddPath(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 startBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType
	);

	const CacheItem* GetCachedPath(
		const int2 startBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType
	);

private:
	void RemoveFrontQueItem();

	unsigned int GetHash(const int2 sb, const int2 gb, float goalRadius, int pathType) const {
		const unsigned int index = ((gb.y * blocksX + gb.x) * blocksZ + sb.y) * blocksX;
		const unsigned int offset = sb.x * (pathType + 1) * std::max(1.0f, goalRadius);
		return (index + offset);
	}

	float GetCacheHitPercentage() const {
		if ((numCacheHits + numCacheMisses) == 0)
			return 0.0f;

		return ((numCacheHits / float(numCacheHits + numCacheMisses)) * 100.0f);
	}

private:
	struct CacheQue {
		int timeout;
		unsigned int hash;
	};

	std::list<CacheQue> cacheQue;
	std::map<unsigned int, CacheItem*> cachedPaths;

	typedef std::map<unsigned int, CacheItem*>::const_iterator CachedPathConstIter;

	int blocksX;
	int blocksZ;

	int numCacheHits;
	int numCacheMisses;
};

#endif
