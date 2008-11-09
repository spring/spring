#ifndef PATHCACHE_H
#define PATHCACHE_H

#include <map>
#include <list>

#include "IPath.h"
#include "Vec2.h"

class CPathCache
{
public:
	CPathCache(int blocksX,int blocksZ);
	~CPathCache(void);

	struct CacheItem {
		IPath::SearchResult result;
		IPath::Path path;
		int2 startBlock;
		int2 goalBlock;
		float goalRadius;
		int pathType;
	};

	void AddPath(IPath::Path* path, IPath::SearchResult result, int2 startBlock,int2 goalBlock,float goalRadius,int pathType);
	CacheItem* GetCachedPath(int2 startBlock,int2 goalBlock,float goalRadius,int pathType);
	void Update(void);

	std::map<unsigned int,CacheItem*> cachedPaths;

	struct CacheQue {
		int timeout;
		unsigned int hash;
	};
	std::list<CacheQue> cacheQue;
	void RemoveFrontQueItem(void);

	int blocksX;
	int blocksZ;

	int numCacheHits;
	int numCacheMisses;
};

#endif
