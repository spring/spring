/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include <list>
#include <vector>
#include <deque>

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
	CPathFinder(bool threadSafe = true);

	static void InitDirectionVectorsTable();
	static void InitDirectionCostsTable();

	static const   int2* GetDirectionVectorsTable2D();
	static const float3* GetDirectionVectorsTable3D();

protected: // IPathFinder impl
	/// Performs the actual search.
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
	IPath::SearchResult FinishSearch(const MoveDef&, const CPathFinderDef&, IPath::Path&) const;

	const CPathCache::CacheItem* GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const {
		// only cache in Estimator! (cause of flow & heatmapping etc.)
		return nullptr;
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
		const float3 nextPoint,
		std::deque<int2>& previous,
		int2 square
	) const;

	inline void SmoothMidWaypoint(
		const int2 testsqr,
		const int2 prvsqr,
		const MoveDef& moveDef,
		IPath::Path& foundPath,
		const float3 nextPoint
	) const;

	typedef CMoveMath::BlockType (*BlockCheckFunc)(const MoveDef&, int, int, const CSolidObject*);

	BlockCheckFunc blockCheckFunc;
};

#endif // PATH_FINDER_H
