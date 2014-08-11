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
	virtual ~IPathFinder() {}

	PathNodeStateBuffer& GetNodeStateBuffer() { return blockStates; }

public:
	static int2 PE_DIRECTION_VECTORS[PATH_DIRECTIONS];
	static int2 PF_DIRECTION_VECTORS_2D[PATH_DIRECTIONS << 1];

	const unsigned int BLOCK_SIZE;
	const bool isEstimator;

	int2 mStartBlock;
	unsigned int mStartBlockIdx;
	unsigned int mGoalBlockIdx;   ///< set during each search as the square closest to the goal
	float mGoalHeuristic;         ///< heuristic value of goalSquareIdx

	unsigned int maxBlocksToBeSearched;
	unsigned int testedBlocks;

	PathNodeBuffer openBlockBuffer;
	PathNodeStateBuffer blockStates;
	PathPriorityQueue openBlocks;

	std::vector<unsigned int> dirtyBlocks; /// List of blocks changed in last search.
};

#endif // IPATH_FINDER_H
