/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "PathEstimator.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <boost/version.hpp>

#include "lib/minizip/zip.h"
#include "mmgr.h"

#include "PathAllocator.h"
#include "PathCache.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathFlowMap.hpp"
#include "Map/ReadMap.h"
#include "Game/LoadScreen.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/FileSystem/ArchiveZip.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"
#include "System/NetProtocol.h"

#define PATHDEBUG false

const std::string pathDir = "cache/paths/";

#if !defined(USE_MMGR)
void* CPathEstimator::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathEstimator::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }
#endif



CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, const std::string& cacheFileName, const std::string& mapFileName):
	BLOCK_SIZE(BSIZE),
	BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
	BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
	nbrOfBlocksX(gs->mapx / BLOCK_SIZE),
	nbrOfBlocksZ(gs->mapy / BLOCK_SIZE),
	pathChecksum(0),
	offsetBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	costBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	nextOffsetMessage(-1),
	nextCostMessage(-1),
	blockStates(int2(nbrOfBlocksX, nbrOfBlocksZ), int2(gs->mapx, gs->mapy))
{
 	pathFinder = pf;

	// these give the changes in (x, z) coors
	// when moving one step in given direction
	//
	// NOTE: the choices of +1 for LEFT and UP
	// are not arbitrary (they are related to
	// GetBlockVertexOffset) and also need to
	// be consistent with the PATHOPT_* flags
	// (because of PathDir2PathOpt)
	directionVectors[PATHDIR_LEFT      ] = int2(+1,  0);
	directionVectors[PATHDIR_RIGHT     ] = int2(-1,  0);
	directionVectors[PATHDIR_UP        ] = int2( 0, +1);
	directionVectors[PATHDIR_DOWN      ] = int2( 0, -1);
	directionVectors[PATHDIR_LEFT_UP   ] = int2(directionVectors[PATHDIR_LEFT ].x, directionVectors[PATHDIR_UP  ].y);
	directionVectors[PATHDIR_RIGHT_UP  ] = int2(directionVectors[PATHDIR_RIGHT].x, directionVectors[PATHDIR_UP  ].y);
	directionVectors[PATHDIR_RIGHT_DOWN] = int2(directionVectors[PATHDIR_RIGHT].x, directionVectors[PATHDIR_DOWN].y);
	directionVectors[PATHDIR_LEFT_DOWN ] = int2(directionVectors[PATHDIR_LEFT ].x, directionVectors[PATHDIR_DOWN].y);

	mGoalSqrOffset.x = BLOCK_SIZE >> 1;
	mGoalSqrOffset.y = BLOCK_SIZE >> 1;

	vertexCosts.resize(moveinfo->moveData.size() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, PATHCOST_INFINITY);

	// load precalculated data if it exists
	InitEstimator(cacheFileName, mapFileName);
}

CPathEstimator::~CPathEstimator()
{
	for (int i = 0; i < blockStates.GetSize(); i++)
		blockStates[i].nodeOffsets.clear();

	delete pathCache;

	blockStates.Clear();
	vertexCosts.clear();
}


void CPathEstimator::InitEstimator(const std::string& cacheFileName, const std::string& map)
{
	int numThreads_tmp = configHandler->Get("HardwareThreadCount", 0);
	size_t numThreads = ((numThreads_tmp < 0) ? 0 : numThreads_tmp);

	if (numThreads == 0) {
		#if (BOOST_VERSION >= 103500)
		numThreads = boost::thread::hardware_concurrency();
		#elif defined(USE_GML)
		numThreads = gmlCPUCount();
		#else
		numThreads = 1;
		#endif
	}

	if (threads.size() != numThreads) {
		threads.resize(numThreads);
		pathFinders.resize(numThreads);
	}
	pathFinders[0] = pathFinder;

	// Not much point in multithreading these...
	InitBlocks();

	char calcMsg[512];
	sprintf(calcMsg, "Reading Estimate PathCosts [%d]", BLOCK_SIZE);
	loadscreen->SetLoadMessage(calcMsg);

	if (!ReadFile(cacheFileName, map)) {
		pathBarrier = new boost::barrier(numThreads);

		// Start threads if applicable
		for (size_t i = 1; i < numThreads; ++i) {
			pathFinders[i] = new CPathFinder();
			threads[i] = new boost::thread(boost::bind(&CPathEstimator::CalcOffsetsAndPathCosts, this, i));
		}

		// Use the current thread as thread zero
		CalcOffsetsAndPathCosts(0);

		for (size_t i = 1; i < numThreads; ++i) {
			threads[i]->join();
			delete threads[i];
			delete pathFinders[i];
		}

		delete pathBarrier;

		loadscreen->SetLoadMessage("PathCosts: writing", true);
		WriteFile(cacheFileName, map);
		loadscreen->SetLoadMessage("PathCosts: written", true);
	}

	pathCache = new CPathCache(nbrOfBlocksX, nbrOfBlocksZ);
}



void CPathEstimator::InitBlocks() {
	for (unsigned int idx = 0; idx < blockStates.GetSize(); idx++) {
		const unsigned int x = idx % nbrOfBlocksX;
		const unsigned int z = idx / nbrOfBlocksX;
		const unsigned int blockNr = z * nbrOfBlocksX + x;

		blockStates[blockNr].nodeOffsets.resize(moveinfo->moveData.size());
	}
}


void CPathEstimator::CalcOffsetsAndPathCosts(unsigned int threadNum) {
	//! reset FPU state for synced computations
	streflop_init<streflop::Simple>();

	// NOTE: EstimatePathCosts() [B] is temporally dependent on CalculateBlockOffsets() [A],
	// A must be completely finished before B_i can be safely called. This means we cannot
	// let thread i execute (A_i, B_i), but instead have to split the work such that every
	// thread finishes its part of A before any starts B_i.
	const unsigned int maxBlockIdx = blockStates.GetSize() - 1;
	int i;

	while ((i = --offsetBlockNum) >= 0)
		CalculateBlockOffsets(maxBlockIdx - i, threadNum);

	pathBarrier->wait();

	while ((i = --costBlockNum) >= 0)
		EstimatePathCosts(maxBlockIdx - i, threadNum);
}


void CPathEstimator::CalculateBlockOffsets(unsigned int blockIdx, unsigned int threadNum)
{
	const unsigned int x = blockIdx % nbrOfBlocksX;
	const unsigned int z = blockIdx / nbrOfBlocksX;

	if (threadNum == 0 && blockIdx >= nextOffsetMessageIdx) {
		nextOffsetMessageIdx = blockIdx + blockStates.GetSize() / 16;
		net->Send(CBaseNetProtocol::Get().SendCPUUsage(BLOCK_SIZE | (blockIdx << 8)));
	}

	for (vector<MoveData*>::iterator mi = moveinfo->moveData.begin(); mi != moveinfo->moveData.end(); mi++)
		FindOffset(**mi, x, z);
}

void CPathEstimator::EstimatePathCosts(unsigned int blockIdx, unsigned int threadNum) {
	const unsigned int x = blockIdx % nbrOfBlocksX;
	const unsigned int z = blockIdx / nbrOfBlocksX;

	if (threadNum == 0 && blockIdx >= nextCostMessageIdx) {
		nextCostMessageIdx = blockIdx + blockStates.GetSize() / 16;

		char calcMsg[128];
		sprintf(calcMsg, "PathCosts: precached %d of %d blocks", blockIdx, blockStates.GetSize());

		net->Send(CBaseNetProtocol::Get().SendCPUUsage(0x1 | BLOCK_SIZE | (blockIdx << 8)));
		loadscreen->SetLoadMessage(calcMsg, (blockIdx != 0));
	}

	for (vector<MoveData*>::iterator mi = moveinfo->moveData.begin(); mi != moveinfo->moveData.end(); mi++)
		CalculateVertices(**mi, x, z, threadNum);
}






/**
 * Finds a square accessable by the given movedata within the given block
 */
void CPathEstimator::FindOffset(const MoveData& moveData, unsigned int blockX, unsigned int blockZ) {
	// lower corner position of block
	const unsigned int lowerX = blockX * BLOCK_SIZE;
	const unsigned int lowerZ = blockZ * BLOCK_SIZE;

	const unsigned int blockArea = (BLOCK_SIZE * BLOCK_SIZE) / SQUARE_SIZE;

	unsigned int bestPosX = BLOCK_SIZE >> 1;
	unsigned int bestPosZ = BLOCK_SIZE >> 1;

	float bestCost = std::numeric_limits<float>::max();

	// search for an accessible position
	for (unsigned int z = 1; z < BLOCK_SIZE; z += 2) {
		for (unsigned int x = 1; x < BLOCK_SIZE; x += 2) {
			const int dx = x - (BLOCK_SIZE >> 1);
			const int dz = z - (BLOCK_SIZE >> 1);
			const float speedMod = moveData.moveMath->SpeedMod(moveData, int(lowerX + x), int(lowerZ + z));

			float cost = (dx * dx + dz * dz) + (blockArea / (0.001f + speedMod));

			if (moveData.moveMath->IsBlocked2(moveData, lowerX + x, lowerZ + z) & CMoveMath::BLOCK_STRUCTURE) {
				cost = std::numeric_limits<float>::infinity();
			}

			if (cost < bestCost) {
				bestCost = cost;
				bestPosX = x;
				bestPosZ = z;
			}
		}
	}

	// store the offset found
	blockStates[blockZ * nbrOfBlocksX + blockX].nodeOffsets[moveData.pathType].x = blockX * BLOCK_SIZE + bestPosX;
	blockStates[blockZ * nbrOfBlocksX + blockX].nodeOffsets[moveData.pathType].y = blockZ * BLOCK_SIZE + bestPosZ;
}


/**
 * Calculate all vertices connected from the given block
 * (always 4 out of 8 vertices connected to the block)
 */
void CPathEstimator::CalculateVertices(const MoveData& moveData, unsigned int blockX, unsigned int blockZ, unsigned int thread) {
	for (unsigned int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++)
		CalculateVertex(moveData, blockX, blockZ, dir, thread);
}


/**
 * Calculate requested vertex
 */
void CPathEstimator::CalculateVertex(
	const MoveData& moveData,
	unsigned int parentBlockX,
	unsigned int parentBlockZ,
	unsigned int direction,
	unsigned int threadNum)
{
	const int childBlockX = parentBlockX + directionVectors[direction].x;
	const int childBlockZ = parentBlockZ + directionVectors[direction].y;

	const unsigned int parentBlockNbr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	const unsigned int vertexNbr =
		moveData.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
		parentBlockNbr * PATH_DIRECTION_VERTICES +
		direction;

	// outside map?
	if (childBlockX < 0 || childBlockZ < 0 || childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertexCosts[vertexNbr] = PATHCOST_INFINITY;
		return;
	}

	// start position
	const unsigned int parentXSquare = blockStates[parentBlockNbr].nodeOffsets[moveData.pathType].x;
	const unsigned int parentZSquare = blockStates[parentBlockNbr].nodeOffsets[moveData.pathType].y;

	// goal position
	const unsigned int childBlockNbr = childBlockZ * nbrOfBlocksX + childBlockX;
	const unsigned int childXSquare = blockStates[childBlockNbr].nodeOffsets[moveData.pathType].x;
	const unsigned int childZSquare = blockStates[childBlockNbr].nodeOffsets[moveData.pathType].y;

	const float3& startPos = SquareToFloat3(parentXSquare, parentZSquare);
	const float3& goalPos = SquareToFloat3(childXSquare, childZSquare);

	// PathFinder definition
	CRangedGoalWithCircularConstraint pfDef(startPos, goalPos, 0, 1.1f, 2);

	// the path to find
	IPath::Path path;
	IPath::SearchResult result;

	// since CPathFinder::GetPath() is not thread-safe,
	// use this thread's "private" CPathFinder instance
	// (rather than locking pathFinder->GetPath()) if we
	// are in one
	result = pathFinders[threadNum]->GetPath(moveData, startPos, pfDef, path, false, true, MAX_SEARCHED_NODES_PE, false, 0, true);

	// store the result
	if (result == IPath::Ok)
		vertexCosts[vertexNbr] = path.pathCost;
	else
		vertexCosts[vertexNbr] = PATHCOST_INFINITY;
}


/**
 * Mark affected blocks as obsolete
 */
void CPathEstimator::MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned z2) {
	// find the upper and lower corner of the rectangular area
	int lowerX, upperX;
	int lowerZ, upperZ;

	if (x1 < x2) {
		lowerX = (x1 / BLOCK_SIZE) - 1;
		upperX = (x2 / BLOCK_SIZE);
	} else {
		lowerX = (x2 / BLOCK_SIZE) - 1;
		upperX = (x1 / BLOCK_SIZE);
	}
	if (z1 < z2) {
		lowerZ = (z1 / BLOCK_SIZE) - 1;
		upperZ = (z2 / BLOCK_SIZE);
	} else {
		lowerZ = (z2 / BLOCK_SIZE) - 1;
		upperZ = (z1 / BLOCK_SIZE);
	}

	// error-check
	upperX = std::min(upperX, int(nbrOfBlocksX - 1));
	upperZ = std::min(upperZ, int(nbrOfBlocksZ - 1));

	if (lowerX < 0) lowerX = 0;
	if (lowerZ < 0) lowerZ = 0;

	// mark the blocks inside the rectangle, enqueue them
	// from upper to lower because of the placement of the
	// bi-directional vertices
	for (int z = upperZ; z >= lowerZ; z--) {
		for (int x = upperX; x >= lowerX; x--) {
			if (!(blockStates[z * nbrOfBlocksX + x].nodeMask & PATHOPT_OBSOLETE)) {
				std::vector<MoveData*>::iterator mi;

				for (mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
					SingleBlock sb;
						sb.blockPos.x = x;
						sb.blockPos.y = z;
						sb.moveData = *mi;
					changedBlocks.push_back(sb);
					blockStates[z * nbrOfBlocksX + x].nodeMask |= PATHOPT_OBSOLETE;
				}
			}
		}
	}
}


/**
 * Update some obsolete blocks using the FIFO-principle
 */
void CPathEstimator::Update() {
	pathCache->Update();

	unsigned int counter = 0;

	while (!changedBlocks.empty() && counter < BLOCKS_TO_UPDATE) {
		// next block in line
		SingleBlock sb = changedBlocks.front();
		changedBlocks.pop_front();

		const unsigned int blockIdx = sb.blockPos.y * nbrOfBlocksX + sb.blockPos.x;

		// check if it's already updated
		if (!(blockStates[blockIdx].nodeMask & PATHOPT_OBSOLETE))
			continue;

		// no, update the block
		FindOffset(*sb.moveData, sb.blockPos.x, sb.blockPos.y);
		CalculateVertices(*sb.moveData, sb.blockPos.x, sb.blockPos.y);

		// mark it as updated
		if (sb.moveData == moveinfo->moveData.back()) {
			blockStates[blockIdx].nodeMask &= ~PATHOPT_OBSOLETE;
		}

		// one block updated
		counter++;
	}
}


/**
 * Stores data and does some top-administration
 */
IPath::SearchResult CPathEstimator::GetPath(
	const MoveData& moveData,
	float3 start,
	const CPathFinderDef& peDef,
	IPath::Path& path,
	unsigned int maxSearchedBlocks,
	bool synced
) {
	start.CheckInBounds();

	// clear the path
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	// initial calculations
	maxBlocksToBeSearched = std::min(maxSearchedBlocks, MAX_SEARCHED_NODES_PE - 8U);

	int2 startBlock;
		startBlock.x = start.x / BLOCK_PIXEL_SIZE;
		startBlock.y = start.z / BLOCK_PIXEL_SIZE;
	int2 goalBlock;
		goalBlock.x = peDef.goalSquareX / BLOCK_SIZE;
		goalBlock.y = peDef.goalSquareZ / BLOCK_SIZE;

	mStartBlock = startBlock;
	mStartBlockIdx = startBlock.y * nbrOfBlocksX + startBlock.x;

	if (synced) {
		CPathCache::CacheItem* ci = pathCache->GetCachedPath(startBlock, goalBlock, peDef.sqGoalRadius, moveData.pathType);
		if (ci) {
			// use a cached path if we have one (NOTE: only when in synced context)
			path = ci->path;
			return ci->result;
		}
	}

	// oterhwise search
	IPath::SearchResult result = InitSearch(moveData, peDef, synced);

	// if search successful, generate new path
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveData, path);

		if (synced) {
			// add succesful paths to the cache (NOTE: only when in synced context)
			pathCache->AddPath(&path, result, startBlock, goalBlock, peDef.sqGoalRadius, moveData.pathType);
		}

		if (PATHDEBUG) {
			LogObject() << "PE: Search completed.\n";
			LogObject() << "Tested blocks: " << testedBlocks << "\n";
			LogObject() << "Open blocks: " << openBlockBuffer.GetSize() << "\n";
			LogObject() << "Path length: " << path.path.size() << "\n";
			LogObject() << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if (PATHDEBUG) {
			LogObject() << "PE: Search failed!\n";
			LogObject() << "Tested blocks: " << testedBlocks << "\n";
			LogObject() << "Open blocks: " << openBlockBuffer.GetSize() << "\n";
		}
	}

	return result;
}


// set up the starting point of the search
IPath::SearchResult CPathEstimator::InitSearch(const MoveData& moveData, const CPathFinderDef& peDef, bool synced) {
	const unsigned int xSquare = blockStates[mStartBlockIdx].nodeOffsets[moveData.pathType].x;
	const unsigned int zSquare = blockStates[mStartBlockIdx].nodeOffsets[moveData.pathType].y;

	if (peDef.IsGoal(xSquare, zSquare)) {
		// is starting square inside goal area?
		return IPath::CantGetCloser;
	}

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockStates[mStartBlockIdx].nodeMask |= PATHOPT_OPEN;
	blockStates[mStartBlockIdx].fCost = 0.0f;
	blockStates[mStartBlockIdx].gCost = 0.0f;
	blockStates.SetMaxFCost(0.0f);
	blockStates.SetMaxGCost(0.0f);

	dirtyBlocks.push_back(mStartBlockIdx);

	openBlockBuffer.SetSize(0);
	// add the starting block to the open-blocks-queue
	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = 0.0f;
		ob->gCost   = 0.0f;
		ob->nodePos = mStartBlock;
		ob->nodeNum = mStartBlockIdx;
	openBlocks.push(ob);

	// mark starting point as best found position
	mGoalBlock = mStartBlock;
	mGoalHeuristic = peDef.Heuristic(xSquare, zSquare);

	// get the goal square offset
	mGoalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	// perform the search
	IPath::SearchResult result = DoSearch(moveData, peDef, synced);

	// if no improvements are found, then return CantGetCloser instead
	if (mGoalBlock.x == mStartBlock.x && mGoalBlock.y == mStartBlock.y) {
		return IPath::CantGetCloser;
	}

	return result;
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathEstimator::DoSearch(const MoveData& moveData, const CPathFinderDef& peDef, bool synced) {
	bool foundGoal = false;

	while (!openBlocks.empty() && (openBlockBuffer.GetSize() < maxBlocksToBeSearched)) {
		// get the open block with lowest cost
		PathNode* ob = const_cast<PathNode*>(openBlocks.top());
		openBlocks.pop();

		// check if the block has been marked as unaccessible during its time in the queue
		if (blockStates[ob->nodeNum].nodeMask & (PATHOPT_BLOCKED | PATHOPT_CLOSED | PATHOPT_FORBIDDEN))
			continue;

		// no, check if the goal is already reached
		const unsigned int xBSquare = blockStates[ob->nodeNum].nodeOffsets[moveData.pathType].x;
		const unsigned int zBSquare = blockStates[ob->nodeNum].nodeOffsets[moveData.pathType].y;
		const unsigned int xGSquare = ob->nodePos.x * BLOCK_SIZE + mGoalSqrOffset.x;
		const unsigned int zGSquare = ob->nodePos.y * BLOCK_SIZE + mGoalSqrOffset.y;

		if (peDef.IsGoal(xBSquare, zBSquare) || peDef.IsGoal(xGSquare, zGSquare)) {
			mGoalBlock = ob->nodePos;
			mGoalHeuristic = 0.0f;
			foundGoal = true;
			break;
		}

		// no, test the 8 surrounding blocks
		// NOTE: each of these calls increments openBlockBuffer.idx by 1, so
		// maxBlocksToBeSearched is always less than <MAX_SEARCHED_NODES_PE - 8>
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT,       synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_UP,    synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_UP,         synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_UP,   synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT,      synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_DOWN, synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_DOWN,       synced);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_DOWN,  synced);

		// mark this block as closed
		blockStates[ob->nodeNum].nodeMask |= PATHOPT_CLOSED;
	}

	// we found our goal
	if (foundGoal)
		return IPath::Ok;

	// we could not reach the goal
	if (openBlockBuffer.GetSize() >= maxBlocksToBeSearched)
		return IPath::GoalOutOfRange;

	// search could not reach the goal due to the unit being locked in
	if (openBlocks.empty())
		return IPath::GoalOutOfRange;

	// should never happen
	LogObject() << "ERROR: CPathEstimator::DoSearch() - Unhandled end of search!\n";
	return IPath::Error;
}


/**
 * Test the accessability of a block and its value,
 * possibly also add it to the open-blocks pqueue.
 */
void CPathEstimator::TestBlock(
	const MoveData& moveData,
	const CPathFinderDef& peDef,
	PathNode& parentOpenBlock,
	unsigned int pathDir,
	bool synced
) {
	testedBlocks++;

	// initial calculations of the new block
	int2 block;
		block.x = parentOpenBlock.nodePos.x + directionVectors[pathDir].x;
		block.y = parentOpenBlock.nodePos.y + directionVectors[pathDir].y;

	const int vertexIdx =
		moveData.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
		parentOpenBlock.nodeNum * PATH_DIRECTION_VERTICES +
		GetBlockVertexOffset(pathDir, nbrOfBlocksX);
	const unsigned int blockIdx = block.y * nbrOfBlocksX + block.x;

	if (block.x < 0 || block.x >= nbrOfBlocksX || block.y < 0 || block.y >= nbrOfBlocksZ) {
		// blocks should never be able to lie outside map
		// (due to the infinite vertex-costs at the edges)
		return;
	}

	if (vertexIdx < 0 || vertexIdx >= vertexCosts.size())
		return;

	if (vertexCosts[vertexIdx] >= PATHCOST_INFINITY)
		return;

	// check if the block is unavailable
	if (blockStates[blockIdx].nodeMask & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	const unsigned int xSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].x;
	const unsigned int zSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].y;

	// check if the block is blocked or out of constraints
	if (!peDef.WithinConstraints(xSquare, zSquare)) {
		blockStates[blockIdx].nodeMask |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blockIdx);
		return;
	}


	// evaluate this node (NOTE the max-resolution indexing for {flow,extra}Cost)
	const float flowCost = (PathFlowMap::GetInstance())->GetFlowCost(xSquare, zSquare, moveData, PathDir2PathOpt(pathDir));
	const float extraCost = blockStates.GetNodeExtraCost(xSquare, zSquare, synced);
	const float nodeCost = vertexCosts[vertexIdx] + flowCost + extraCost;

	const float gCost = parentOpenBlock.gCost + nodeCost;  // g
	const float hCost = peDef.Heuristic(xSquare, zSquare); // h
	const float fCost = gCost + hCost;                     // f


	if (blockStates[blockIdx].nodeMask & PATHOPT_OPEN) {
		// already in the open set
		if (blockStates[blockIdx].fCost <= fCost)
			return;

		blockStates[blockIdx].nodeMask &= ~PATHOPT_AXIS_DIRS;
	}

	// look for improvements
	if (hCost < mGoalHeuristic) {
		mGoalBlock = block;
		mGoalHeuristic = hCost;
	}

	// store this block as open.
	openBlockBuffer.SetSize(openBlockBuffer.GetSize() + 1);
	assert(openBlockBuffer.GetSize() < MAX_SEARCHED_NODES_PE);

	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = fCost;
		ob->gCost   = gCost;
		ob->nodePos = block;
		ob->nodeNum = blockIdx;
	openBlocks.push(ob);

	blockStates.SetMaxFCost(std::max(blockStates.GetMaxFCost(), fCost));
	blockStates.SetMaxGCost(std::max(blockStates.GetMaxGCost(), gCost));

	// mark this block as open
	blockStates[blockIdx].fCost = fCost;
	blockStates[blockIdx].gCost = gCost;
	blockStates[blockIdx].nodeMask |= (pathDir | PATHOPT_OPEN);
	blockStates[blockIdx].parentNodePos = parentOpenBlock.nodePos;

	dirtyBlocks.push_back(blockIdx);
}


/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveData& moveData, IPath::Path& foundPath) {
	int2 block = mGoalBlock;

	while (block.x != mStartBlock.x || block.y != mStartBlock.y) {
		const unsigned int blockIdx = block.y * nbrOfBlocksX + block.x;

		{
			// use offset defined by the block
			const unsigned int xBSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].x;
			const unsigned int zBSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].y;
			const float3& pos = SquareToFloat3(xBSquare, zBSquare);

			foundPath.path.push_back(pos);
		}

		// next step backwards
		block = blockStates[blockIdx].parentNodePos;
	}

	if (!foundPath.path.empty()) {
		foundPath.pathGoal = foundPath.path.front();
	}

	// set some additional information
	foundPath.pathCost = blockStates[mGoalBlock.y * nbrOfBlocksX + mGoalBlock.x].fCost - mGoalHeuristic;
}


/**
 * Clean lists from last search
 */
void CPathEstimator::ResetSearch() {
	openBlocks.Clear();

	while (!dirtyBlocks.empty()) {
		PathNodeState& ns = blockStates[dirtyBlocks.back()];
			ns.fCost = PATHCOST_INFINITY;
			ns.gCost = PATHCOST_INFINITY;
			ns.nodeMask &= PATHOPT_OBSOLETE;
			ns.parentNodePos.x = -1;
			ns.parentNodePos.y = -1;
		dirtyBlocks.pop_back();
	}

	testedBlocks = 0;
}


/**
 * Try to read offset and vertices data from file, return false on failure
 * TODO: Read-error-check.
 */
bool CPathEstimator::ReadFile(const std::string& cacheFileName, const std::string& map)
{
	const unsigned int hash = Hash();
	char hashString[50];
	sprintf(hashString, "%u", hash);

	std::string filename = std::string(pathDir) + map + hashString + "." + cacheFileName + ".zip";

	// open file for reading from a suitable location (where the file exists)
	CArchiveZip* pfile = new CArchiveZip(filesystem.LocateFile(filename));

	if (!pfile || !pfile->IsOpen()) {
		delete pfile;
		return false;
	}

	std::auto_ptr<CArchiveZip> auto_pfile(pfile);
	CArchiveZip& file(*pfile);

	const unsigned fid = file.FindFile("pathinfo");

	if (fid < file.NumFiles()) {
		pathChecksum = file.GetCrc32(fid);

		std::vector<boost::uint8_t> buffer;
		file.GetFile(fid, buffer);

		if (buffer.size() < 4)
			return false;

		unsigned filehash = *((unsigned*)&buffer[0]);
		if (filehash != hash)
			return false;

		unsigned pos = sizeof(unsigned);

		// Read block-center-offset data.
		const unsigned blockSize = moveinfo->moveData.size() * sizeof(int2);
		if (buffer.size() < pos + blockSize * blockStates.GetSize())
			return false;

		for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++) {
			std::memcpy(&blockStates[blocknr].nodeOffsets[0], &buffer[pos], blockSize);
			pos += blockSize;
		}

		// Read vertices data.
		if (buffer.size() < pos + vertexCosts.size() * sizeof(float))
			return false;
		std::memcpy(&vertexCosts[0], &buffer[pos], vertexCosts.size() * sizeof(float));

		// File read successful.
		return true;
	} else {
		return false;
	}
}


/**
 * Try to write offset and vertex data to file.
 */
void CPathEstimator::WriteFile(const std::string& cacheFileName, const std::string& map)
{
	// We need this directory to exist
	if (!filesystem.CreateDirectory(pathDir))
		return;

	const unsigned int hash = Hash();
	char hashString[64] = {0};

	sprintf(hashString, "%u", hash);

	const std::string filename = std::string(pathDir) + map + hashString + "." + cacheFileName + ".zip";
	zipFile file;

	// open file for writing in a suitable location
	file = zipOpen(filesystem.LocateFile(filename, FileSystem::WRITE).c_str(), APPEND_STATUS_CREATE);

	if (file) {
		zipOpenNewFileInZip(file, "pathinfo", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);

		// Write hash.
		zipWriteInFileInZip(file, (void*) &hash, 4);

		// Write block-center-offsets.
		for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++)
			zipWriteInFileInZip(file, (void*) &blockStates[blocknr].nodeOffsets[0], moveinfo->moveData.size() * sizeof(int2));

		// Write vertices.
		zipWriteInFileInZip(file, &vertexCosts[0], vertexCosts.size() * sizeof(float));

		zipCloseFileInZip(file);
		zipClose(file, NULL);


		// get the CRC over the written path data
		CArchiveZip* pfile = new CArchiveZip(filesystem.LocateFile(filename));

		if (!pfile || !pfile->IsOpen()) {
			delete pfile;
			return;
		}

		std::auto_ptr<CArchiveZip> auto_pfile(pfile);
		CArchiveZip& file(*pfile);
		
		const unsigned fid = file.FindFile("pathinfo");
		assert(fid < file.NumFiles());
		pathChecksum = file.GetCrc32(fid);
	}
}


/**
 * Returns a hash-code identifying the dataset of this estimator.
 */
unsigned int CPathEstimator::Hash() const
{
	return (readmap->mapChecksum + moveinfo->moveInfoChecksum + BLOCK_SIZE + PATHESTIMATOR_VERSION);
}



float3 CPathEstimator::FindBestBlockCenter(const MoveData* moveData, const float3& pos, bool synced)
{
	CRangedGoalWithCircularConstraint rangedGoal(pos, pos, 0, 0, BLOCK_PIXEL_SIZE * BLOCK_PIXEL_SIZE * 4);
	IPath::Path path;

	std::vector<float3> startPositions;

	// <pos> has been CheckInBounds()'ed already (so
	// .x and .z are in [1, mapxz * SQUARE_SIZE - 1])
	const unsigned int pathType = moveData->pathType;
	const unsigned int blockX = pos.x / BLOCK_PIXEL_SIZE;
	const unsigned int blockZ = pos.z / BLOCK_PIXEL_SIZE;

	const unsigned int zmin = (blockZ | 1) - 1, zmax = std::min(nbrOfBlocksZ - 1, blockZ + 1);
	const unsigned int xmin = (blockX | 1) - 1, xmax = std::min(nbrOfBlocksX - 1, blockX + 1);

	for (unsigned int z = zmin; z <= zmax; ++z) {
		for (unsigned int x = xmin; x <= xmax; ++x) {
			float3 spos;
				spos.y = 0.0f;
				spos.x = blockStates[z * nbrOfBlocksX + x].nodeOffsets[pathType].x * SQUARE_SIZE;
				spos.z = blockStates[z * nbrOfBlocksX + x].nodeOffsets[pathType].y * SQUARE_SIZE;

			startPositions.push_back(spos);
		}
	}

	IPath::SearchResult result = pathFinder->GetPath(*moveData, startPositions, rangedGoal, path, 0, synced);

	if (result == IPath::Ok && !path.path.empty()) {
		return path.path.back();
	}

	return ZeroVector;
}
