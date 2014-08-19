/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IPATH_FINDER_H
#define IPATH_FINDER_H

#include <list>
#include <queue>
#include <cstdlib>

#include "IPath.h"
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
	int2 BlockIdxToPos(const int idx)  const { return int2(idx % nbrOfBlocks.x, idx / nbrOfBlocks.x); }
	int  BlockPosToIdx(const int2 pos) const { return pos.y * nbrOfBlocks.x + pos.x; }

protected:
	/// Clear things up from last search.
	void ResetSearch();

public:
	static int2 PE_DIRECTION_VECTORS[PATH_DIRECTIONS];
	static int2 PF_DIRECTION_VECTORS_2D[PATH_DIRECTIONS << 1];

	const unsigned int BLOCK_SIZE;
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
