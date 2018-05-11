/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IPathFinder.h"
#include "PathFinderDef.h"
#include "PathLog.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/Log/ILog.h"


static std::vector<PathNodeStateBuffer> nodeStateBuffers;
static std::vector<IPathFinder*> pathFinderInstances;


void IPathFinder::InitStatic() { pathFinderInstances.reserve(8); }
void IPathFinder::KillStatic() { pathFinderInstances.clear  ( ); }


void IPathFinder::Init(unsigned int _BLOCK_SIZE)
{
	{
		BLOCK_SIZE = _BLOCK_SIZE;
		BLOCK_PIXEL_SIZE = BLOCK_SIZE * SQUARE_SIZE;

		nbrOfBlocks.x = mapDims.mapx / BLOCK_SIZE;
		nbrOfBlocks.y = mapDims.mapy / BLOCK_SIZE;

		mStartBlockIdx = 0;
		mGoalBlockIdx = 0;

		mGoalHeuristic = 0.0f;

		maxBlocksToBeSearched = 0;
		testedBlocks = 0;

		instanceIndex = pathFinderInstances.size();
	}
	{
		openBlockBuffer.Clear();
		// handled via AllocStateBuffer
		// blockStates.Clear();
		// done in ResetSearch
		// openBlocks.Clear();
		dirtyBlocks.clear();
	}
	{
		pathFinderInstances.push_back(this);
	}

	AllocStateBuffer();
	ResetSearch();
}

void IPathFinder::Kill()
{
	// allow our PNSB to be reused across reloads
	nodeStateBuffers[instanceIndex] = std::move(blockStates);
}


void IPathFinder::AllocStateBuffer()
{
	if (instanceIndex >= nodeStateBuffers.size())
		nodeStateBuffers.emplace_back();

	nodeStateBuffers[instanceIndex].Clear();
	nodeStateBuffers[instanceIndex].Resize(nbrOfBlocks, int2(mapDims.mapx, mapDims.mapy));

	// steal memory, returned in dtor
	blockStates = std::move(nodeStateBuffers[instanceIndex]);
}

void IPathFinder::ResetSearch()
{
	while (!dirtyBlocks.empty()) {
		blockStates.ClearSquare(dirtyBlocks.back());
		dirtyBlocks.pop_back();
	}

	// reserve a batch of dirty blocks
	dirtyBlocks.reserve(4096);
	openBlocks.Clear();

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

	// clear the path
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// initial calculations
	if (BLOCK_SIZE != 1) {
		maxBlocksToBeSearched = std::min(MAX_SEARCHED_NODES_PE - 8U, maxNodes);
	} else {
		maxBlocksToBeSearched = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);
	}

	mStartBlock.x  = startPos.x / BLOCK_PIXEL_SIZE;
	mStartBlock.y  = startPos.z / BLOCK_PIXEL_SIZE;
	mStartBlockIdx = BlockPosToIdx(mStartBlock);
	mGoalBlockIdx  = mStartBlockIdx;

	assert(static_cast<unsigned int>(mStartBlock.x) < nbrOfBlocks.x);
	assert(static_cast<unsigned int>(mStartBlock.y) < nbrOfBlocks.y);

	// check cache (when there is one)
	const int2 goalBlock = {int(pfDef.goalSquareX / BLOCK_SIZE), int(pfDef.goalSquareZ / BLOCK_SIZE)};
	const CPathCache::CacheItem& ci = GetCache(mStartBlock, goalBlock, pfDef.sqGoalRadius, moveDef.pathType, pfDef.synced);

	if (ci.pathType != -1) {
		path = ci.path;
		return ci.result;
	}

	// start up a new search
	const IPath::SearchResult result = InitSearch(moveDef, pfDef, owner);

	// if search was successful, generate new path and cache it
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, pfDef, path);
		AddCache(&path, result, mStartBlock, goalBlock, pfDef.sqGoalRadius, moveDef.pathType, pfDef.synced);

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "==== %s: Search completed ====", (BLOCK_SIZE != 1) ? "PE" : "PF");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "Path length: " _STPF_, path.path.size());
			LOG_L(L_DEBUG, "Path cost: %f", path.pathCost);
			LOG_L(L_DEBUG, "==============================");
		}
	} else {
		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "==== %s: Search failed! ====", (BLOCK_SIZE != 1) ? "PE" : "PF");
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

	if (BLOCK_SIZE != 1)
		square = blockStates.peNodeOffsets[moveDef.pathType][mStartBlockIdx];

	const bool isStartGoal = pfDef.IsGoal(square.x, square.y);
	const bool startInGoal = pfDef.startInGoalRadius;

	const bool allowRawPath = pfDef.allowRawPath;
	const bool allowDefPath = pfDef.allowDefPath;

	assert(allowRawPath || allowDefPath);

	// cleanup after the last search
	ResetSearch();

	IPath::SearchResult results[] = {IPath::CantGetCloser, IPath::Ok, IPath::CantGetCloser};

	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && startInGoal)
		return results[allowRawPath];

	// mark and store the start-block; clear all bits except PATHOPT_OBSOLETE
	blockStates.nodeMask[mStartBlockIdx] &= PATHOPT_OBSOLETE;
	blockStates.nodeMask[mStartBlockIdx] |= PATHOPT_OPEN;
	blockStates.fCost[mStartBlockIdx] = 0.0f;
	blockStates.gCost[mStartBlockIdx] = 0.0f;
	blockStates.SetMaxCost(NODE_COST_F, 0.0f);
	blockStates.SetMaxCost(NODE_COST_G, 0.0f);

	dirtyBlocks.push_back(mStartBlockIdx);

	// start a new search and add the starting block to the open-blocks-queue
	openBlockBuffer.SetSize(0);
	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = 0.0f;
		ob->gCost   = 0.0f;
		ob->nodePos = mStartBlock;
		ob->nodeNum = mStartBlockIdx;
	openBlocks.push(ob);

	// mark starting point as best found position
	mGoalHeuristic = pfDef.Heuristic(square.x, square.y, BLOCK_SIZE);

	enum {
		RAW = 0,
		IPF = 1,
	};

	// perform the search
	results[RAW] = (allowRawPath                                )? DoRawSearch(moveDef, pfDef, owner): IPath::Error;
	results[IPF] = (allowDefPath && results[RAW] == IPath::Error)? DoSearch(moveDef, pfDef, owner): results[RAW];

	if (results[IPF] == IPath::Ok)
		return IPath::Ok;
	if (mGoalBlockIdx != mStartBlockIdx)
		return results[IPF];

	// if start and goal are within the same block but distinct squares (or
	// considered a single point for search purposes), then we probably can
	// not get closer and should return CGC *unless* the caller requested a
	// raw search only
	return results[IPF + ((!allowRawPath || allowDefPath) && (!isStartGoal || startInGoal))];
}

