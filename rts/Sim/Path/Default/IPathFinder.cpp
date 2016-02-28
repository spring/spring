/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathFinder.h"
#include "PathFinderDef.h"
#include "PathLog.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Log/ILog.h"


// these give the changes in (x, z) coors
// when moving one step in given direction
//
// NOTE: the choices of +1 for LEFT and UP are *not* arbitrary
// (they are related to GetBlockVertexOffset) and also need to
// be consistent with the PATHOPT_* flags (for PathDir2PathOpt)
int2 IPathFinder::PE_DIRECTION_VECTORS[PATH_DIRECTIONS] = {
	int2(+1,  0), // PATHDIR_LEFT
	int2(+1, +1), // PATHDIR_LEFT_UP
	int2( 0, +1), // PATHDIR_UP
	int2(-1, +1), // PATHDIR_RIGHT_UP
	int2(-1,  0), // PATHDIR_RIGHT
	int2(-1, -1), // PATHDIR_RIGHT_DOWN
	int2( 0, -1), // PATHDIR_DOWN
	int2(+1, -1), // PATHDIR_LEFT_DOWN
};

//FIXME why not use PATHDIR_* consts and merge code with top one
int2 IPathFinder::PF_DIRECTION_VECTORS_2D[PATH_DIRECTIONS << 1] = {
	int2(0, 0),
	int2(+1 * PATH_NODE_SPACING,  0 * PATH_NODE_SPACING), // PATHOPT_LEFT
	int2(-1 * PATH_NODE_SPACING,  0 * PATH_NODE_SPACING), // PATHOPT_RIGHT
	int2(0, 0),                                           // PATHOPT_LEFT | PATHOPT_RIGHT
	int2( 0 * PATH_NODE_SPACING, +1 * PATH_NODE_SPACING), // PATHOPT_UP
	int2(+1 * PATH_NODE_SPACING, +1 * PATH_NODE_SPACING), // PATHOPT_LEFT | PATHOPT_UP
	int2(-1 * PATH_NODE_SPACING, +1 * PATH_NODE_SPACING), // PATHOPT_RIGHT | PATHOPT_UP
	int2(0, 0),                                           // PATHOPT_LEFT | PATHOPT_RIGHT | PATHOPT_UP
	int2( 0 * PATH_NODE_SPACING, -1 * PATH_NODE_SPACING), // PATHOPT_DOWN
	int2(+1 * PATH_NODE_SPACING, -1 * PATH_NODE_SPACING), // PATHOPT_LEFT | PATHOPT_DOWN
	int2(-1 * PATH_NODE_SPACING, -1 * PATH_NODE_SPACING), // PATHOPT_RIGHT | PATHOPT_DOWN
	int2(0, 0),
	int2(0, 0),
	int2(0, 0),
	int2(0, 0),
	int2(0, 0),
};



IPathFinder::IPathFinder(unsigned int _BLOCK_SIZE)
	: BLOCK_SIZE(_BLOCK_SIZE)
	, BLOCK_PIXEL_SIZE(BLOCK_SIZE * SQUARE_SIZE)
	, isEstimator(BLOCK_SIZE != 1)
	, mStartBlockIdx(0)
	, mGoalBlockIdx(0)
	, mGoalHeuristic(0.0f)
	, maxBlocksToBeSearched(0)
	, testedBlocks(0)
	, nbrOfBlocks(mapDims.mapx / BLOCK_SIZE, mapDims.mapy / BLOCK_SIZE)
	, blockStates(nbrOfBlocks, int2(mapDims.mapx, mapDims.mapy))
{
}


IPathFinder::~IPathFinder()
{
	//ResetSearch();
}


void IPathFinder::ResetSearch()
{
	openBlocks.Clear();

	while (!dirtyBlocks.empty()) {
		blockStates.ClearSquare(dirtyBlocks.back());
		dirtyBlocks.pop_back();
	}

	testedBlocks = 0;
}


IPath::SearchResult IPathFinder::GetPath(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	float3 startPos,
	IPath::Path& path,
	const unsigned int maxNodes
) {
	startPos.ClampInBounds();

	// Clear the path
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// initial calculations
	if (isEstimator) {
		maxBlocksToBeSearched = std::min(MAX_SEARCHED_NODES_PE - 8U, maxNodes);
	} else {
		maxBlocksToBeSearched = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);
	}
	mStartBlock.x  = startPos.x / BLOCK_PIXEL_SIZE;
	mStartBlock.y  = startPos.z / BLOCK_PIXEL_SIZE;
	mStartBlockIdx = BlockPosToIdx(mStartBlock);
	assert((unsigned)mStartBlock.x < nbrOfBlocks.x && (unsigned)mStartBlock.y < nbrOfBlocks.y);

	// Check cache (when there is one)
	int2 goalBlock;
	goalBlock.x = pfDef.goalSquareX / BLOCK_SIZE;
	goalBlock.y = pfDef.goalSquareZ / BLOCK_SIZE;
	const CPathCache::CacheItem* ci = GetCache(mStartBlock, goalBlock, pfDef.sqGoalRadius, moveDef.pathType, pfDef.synced);
	if (ci != nullptr) {
		path = ci->path;
		return ci->result;
	}

	// Start up a new search
	IPath::SearchResult result = InitSearch(moveDef, pfDef, owner);

	// If search was successful, generate new path
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, pfDef, path);

		// Save to cache
		AddCache(&path, result, mStartBlock, goalBlock, pfDef.sqGoalRadius, moveDef.pathType, pfDef.synced);

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "==== %s: Search completed ====", (isEstimator) ? "PE" : "PF");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "Path length: " _STPF_, path.path.size());
			LOG_L(L_DEBUG, "Path cost: %f", path.pathCost);
			LOG_L(L_DEBUG, "==============================");
		}
	} else {
		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "==== %s: Search failed! ====", (isEstimator) ? "PE" : "PF");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "============================");
		}
	}

	return result;
}


// set up the starting point of the search
IPath::SearchResult IPathFinder::InitSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner)
{
	int2 square = mStartBlock;

	if (isEstimator)
		square = blockStates.peNodeOffsets[moveDef.pathType][mStartBlockIdx];

	const bool isStartGoal = pfDef.IsGoal(square.x, square.y);
	const bool startInGoal = pfDef.startInGoalRadius;

	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && startInGoal)
		return IPath::CantGetCloser;

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockStates.nodeMask[mStartBlockIdx] &= PATHOPT_OBSOLETE; // clear all except PATHOPT_OBSOLETE
	blockStates.nodeMask[mStartBlockIdx] |= PATHOPT_OPEN;
	blockStates.fCost[mStartBlockIdx] = 0.0f;
	blockStates.gCost[mStartBlockIdx] = 0.0f;
	blockStates.SetMaxCost(NODE_COST_F, 0.0f);
	blockStates.SetMaxCost(NODE_COST_G, 0.0f);

	dirtyBlocks.push_back(mStartBlockIdx);

	// start a new search and
	// add the starting block to the open-blocks-queue
	openBlockBuffer.SetSize(0);
	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = 0.0f;
		ob->gCost   = 0.0f;
		ob->nodePos = mStartBlock;
		ob->nodeNum = mStartBlockIdx;
	openBlocks.push(ob);

	// mark starting point as best found position
	mGoalBlockIdx  = mStartBlockIdx;
	mGoalHeuristic = pfDef.Heuristic(square.x, square.y);

	// perform the search
	const IPath::SearchResult result = DoSearch(moveDef, pfDef, owner);

	if (result == IPath::Ok)
		return result;
	if (mGoalBlockIdx != mStartBlockIdx)
		return result;

	// if start and goal are within the same block, but distinct squares
	// or considered a single point for search purposes, then we probably
	// can not get closer
	return (!isStartGoal || startInGoal)? IPath::CantGetCloser: result;
}
