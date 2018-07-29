/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include <vector>

#include "IPath.h"
#include "IPathFinder.h"
#include "PathConstants.h"
#include "PathDataTypes.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Objects/SolidObject.h"

struct MoveDef;
class CPathFinderDef;

class CPathFinder: public IPathFinder {
public:
	static void InitStatic();

	CPathFinder() = default; // defer Init
	CPathFinder(bool threadSafe) { Init(threadSafe); }

	void Init(bool threadSafe);
	void Kill() { IPathFinder::Kill(); }

	typedef CMoveMath::BlockType (*BlockCheckFunc)(const MoveDef&, int, int, const CSolidObject*);

protected:
	/// Performs the actual search.
	IPath::SearchResult DoRawSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner);
	IPath::SearchResult DoSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner);

	/**
	 * Test the availability and value of a square,
	 * and possibly add it to the queue of open squares.
	 */
	bool TestBlock(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		const unsigned int pathOptDir,
		const unsigned int blockStatus,
		float speedMod
	);
	/**
	 * Recreates the path found by pathfinder.
	 * Starting at goalSquare and tracking backwards.
	 *
	 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
	 */
	void FinishSearch(const MoveDef&, const CPathFinderDef&, IPath::Path&) const;

	const CPathCache::CacheItem& GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const {
		// only cache in Estimator! (cause of flow & heatmapping etc.)
		return dummyCacheItem;
	}

	void AddCache(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) { }

private:
	void TestNeighborSquares(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner
	);

	/**
	 * Adjusts the found path to cut corners where possible.
	 */
	void AdjustFoundPath(
		const MoveDef&,
		IPath::Path&,
		const int2& p1,
		const int2& p2,
		const int2& p0
	) const;

	inline void SmoothMidWaypoint(
		const int2 testSqr,
		const int2 prevSqr,
		const MoveDef& moveDef,
		IPath::Path& foundPath
	) const;

	BlockCheckFunc blockCheckFunc;
	CPathCache::CacheItem dummyCacheItem;
};

#endif // PATH_FINDER_H
