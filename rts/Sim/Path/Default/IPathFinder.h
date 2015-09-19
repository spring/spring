/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPATH_FINDER_H
#define IPATH_FINDER_H

#include <list>
#include <queue>
#include <cstdlib>

#include "IPath.h"
#include "PathCache.h"
#include "PathConstants.h"
#include "PathDataTypes.h"

struct MoveDef;
class CPathFinderDef;
class CSolidObject;


class IPathFinder {
public:
	IPathFinder(unsigned int BLOCK_SIZE);
	virtual ~IPathFinder();

	// size of the memory-region we hold allocated (excluding sizeof(*this))
	// (PathManager stores HeatMap and FlowMap, so we do not need to add them)
	size_t GetMemFootPrint() const { return (blockStates.GetMemFootPrint()); }

	PathNodeStateBuffer& GetNodeStateBuffer() { return blockStates; }

	unsigned int GetBlockSize() const { return BLOCK_SIZE; }
	int2 GetNumBlocks() const { return nbrOfBlocks; }
	int2 BlockIdxToPos(const unsigned idx)  const { return int2(idx % nbrOfBlocks.x, idx / nbrOfBlocks.x); }
	int  BlockPosToIdx(const int2 pos) const { return pos.y * nbrOfBlocks.x + pos.x; }


	/**
	 * Gives a path from given starting location to target defined in
	 * CPathFinderDef, whenever any such are available.
	 * If no complete path was found, any path leading as "close" to target as
	 * possible will be created, and SearchResult::OutOfRange will be returned.
	 * Only when no "closer" position than the given starting location could be
	 * found no path is created, and SearchResult::CantGetCloser is returned.
	 *
	 * @param moveDef defining the footprint of the unit requesting the path.
	 * @param startPos The starting location of the path. (Projected onto (x,z))
	 * @param pfDef Object defining the target/goal of the search.
	 *   Could also be used to put constraints on the searchspace used.
	 * @param path If any path could be found, it will be generated and put into
	 *   this structure.
	 * @param exactPath Overrides the return of the "closest" path.
	 *   If this option is true, a path is returned only if it's completed all
	 *   the way to the goal defined in pfDef. All SearchResult::OutOfRange are
	 *   then turned into SearchResult::CantGetCloser.
	 * @param maxNodes The maximum number of nodes / squares the search is
	 *   allowed to analyze. This restriction could be used in cases where
	 *   CPU-consumption is critical.
	 */
	IPath::SearchResult GetPath(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const CSolidObject* owner,
		float3 startPos,
		IPath::Path& path,
		const unsigned int maxNodes
	);

protected:
	///
	IPath::SearchResult InitSearch(const MoveDef&, const CPathFinderDef&, const CSolidObject* owner);

	/// Clear things up from last search.
	void ResetSearch();

protected: // pure virtuals
	virtual IPath::SearchResult DoSearch(const MoveDef&, const CPathFinderDef&, const CSolidObject* owner) = 0;

	/**
	 * Test the availability and value of a block,
	 * and possibly add it to the queue of open blocks.
	 */
	virtual bool TestBlock(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		const unsigned int pathOptDir,
		const unsigned int blockStatus,
		float speedMod
	) = 0;

	/**
	 * Recreates the path found by pathfinder.
	 * Starting at goalSquare and tracking backwards.
	 *
	 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
	 */
	virtual IPath::SearchResult FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& path) const = 0;


	virtual const CPathCache::CacheItem* GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const = 0;

	virtual void AddCache(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) = 0;

public:
	static int2 PE_DIRECTION_VECTORS[PATH_DIRECTIONS];
	static int2 PF_DIRECTION_VECTORS_2D[PATH_DIRECTIONS << 1];

	const unsigned int BLOCK_SIZE;
	const unsigned int BLOCK_PIXEL_SIZE;
	const bool isEstimator;

	int2 mStartBlock;
	unsigned int mStartBlockIdx;
	unsigned int mGoalBlockIdx;   //< set during each search as the square closest to the goal
	float mGoalHeuristic;         //< heuristic value of goalSquareIdx

	unsigned int maxBlocksToBeSearched;
	unsigned int testedBlocks;

	int2 nbrOfBlocks;             //< Number of blocks on the axes

	PathNodeBuffer openBlockBuffer;
	PathNodeStateBuffer blockStates;
	PathPriorityQueue openBlocks;

	std::vector<unsigned int> dirtyBlocks; //< List of blocks changed in last search.
};

#endif // IPATH_FINDER_H
