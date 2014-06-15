/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"

#include "PathEstimator.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/thread.hpp>

#include "minizip/zip.h"

#include "PathAllocator.h"
#include "PathCache.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathFlowMap.hpp"
#include "PathLog.h"
#include "Map/ReadMap.h"
#include "Game/LoadScreen.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/ThreadPool.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"


CONFIG(int, MaxPathCostsMemoryFootPrint).defaultValue(512).minimumValue(64).description("Maximum memusage (in MByte) of mutlithreaded pathcache generator at loading time.");



// indexed by PATHDIR*
static int2 PE_DIRECTION_VECTORS[PATH_DIRECTIONS];



static const std::string GetPathCacheDir() {
	return (FileSystem::GetCacheDir() + "/paths/");
}

static size_t GetNumThreads() {
	const size_t numThreads = std::max(0, configHandler->GetInt("PathingThreadCount"));
	const size_t numCores = Threading::GetLogicalCpuCores();
	return ((numThreads == 0)? numCores: numThreads);
}



CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, const std::string& cacheFileName, const std::string& mapFileName):
	BLOCK_SIZE(BSIZE),
	BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
	BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
	nbrOfBlocksX(gs->mapx / BLOCK_SIZE),
	nbrOfBlocksZ(gs->mapy / BLOCK_SIZE),

	nextOffsetMessageIdx(0),
	nextCostMessageIdx(0),
	pathChecksum(0),
	offsetBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	costBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	blockStates(int2(nbrOfBlocksX, nbrOfBlocksZ), int2(gs->mapx, gs->mapy)),

	mStartBlockIdx(0),
	mGoalHeuristic(0.0f),
	blockUpdatePenalty(0)
{
 	pathFinder = pf;

	mGoalSqrOffset.x = BLOCK_SIZE >> 1;
	mGoalSqrOffset.y = BLOCK_SIZE >> 1;

	vertexCosts.resize(moveDefHandler->GetNumMoveDefs() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, PATHCOST_INFINITY);

	// load precalculated data if it exists
	InitEstimator(cacheFileName, mapFileName);
}

CPathEstimator::~CPathEstimator()
{
	delete pathCache[0]; pathCache[0] = NULL;
	delete pathCache[1]; pathCache[1] = NULL;
}

void* CPathEstimator::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathEstimator::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }

void CPathEstimator::InitDirectionVectorsTable() {
	// these give the changes in (x, z) coors
	// when moving one step in given direction
	//
	// NOTE: the choices of +1 for LEFT and UP are *not* arbitrary
	// (they are related to GetBlockVertexOffset) and also need to
	// be consistent with the PATHOPT_* flags (for PathDir2PathOpt)
	PE_DIRECTION_VECTORS[PATHDIR_LEFT      ] = int2(+1,  0);
	PE_DIRECTION_VECTORS[PATHDIR_RIGHT     ] = int2(-1,  0);
	PE_DIRECTION_VECTORS[PATHDIR_UP        ] = int2( 0, +1);
	PE_DIRECTION_VECTORS[PATHDIR_DOWN      ] = int2( 0, -1);
	PE_DIRECTION_VECTORS[PATHDIR_LEFT_UP   ] = int2(PE_DIRECTION_VECTORS[PATHDIR_LEFT ].x, PE_DIRECTION_VECTORS[PATHDIR_UP  ].y);
	PE_DIRECTION_VECTORS[PATHDIR_RIGHT_UP  ] = int2(PE_DIRECTION_VECTORS[PATHDIR_RIGHT].x, PE_DIRECTION_VECTORS[PATHDIR_UP  ].y);
	PE_DIRECTION_VECTORS[PATHDIR_RIGHT_DOWN] = int2(PE_DIRECTION_VECTORS[PATHDIR_RIGHT].x, PE_DIRECTION_VECTORS[PATHDIR_DOWN].y);
	PE_DIRECTION_VECTORS[PATHDIR_LEFT_DOWN ] = int2(PE_DIRECTION_VECTORS[PATHDIR_LEFT ].x, PE_DIRECTION_VECTORS[PATHDIR_DOWN].y);
}

const int2* CPathEstimator::GetDirectionVectorsTable() {
	return (&PE_DIRECTION_VECTORS[0]);
}

void CPathEstimator::InitEstimator(const std::string& cacheFileName, const std::string& map)
{
	const unsigned int numThreads = GetNumThreads();

	if (threads.size() != numThreads) {
		threads.resize(numThreads);
		pathFinders.resize(numThreads);
	}

	pathFinders[0] = pathFinder;

	// Not much point in multithreading these...
	InitBlocks();

	if (!ReadFile(cacheFileName, map)) {
		// start extra threads if applicable, but always keep the total
		// memory-footprint made by CPathFinder instances within bounds
		const unsigned int minMemFootPrint = sizeof(CPathFinder) + pathFinder->GetMemFootPrint();
		const unsigned int maxMemFootPrint = configHandler->GetInt("MaxPathCostsMemoryFootPrint") * 1024 * 1024;
		const unsigned int numExtraThreads = std::min(int(numThreads - 1), std::max(0, int(maxMemFootPrint / minMemFootPrint) - 1));
		const unsigned int reqMemFootPrint = minMemFootPrint * (numExtraThreads + 1);

		{
			char calcMsg[512];
			const char* fmtString = (numExtraThreads > 0)?
				"PathCosts: creating PE%u cache with %u PF threads (%u MB)":
				"PathCosts: creating PE%u cache with %u PF thread (%u MB)";

			sprintf(calcMsg, fmtString, BLOCK_SIZE, numExtraThreads + 1, reqMemFootPrint / (1024 * 1024));
			loadscreen->SetLoadMessage(calcMsg);
		}

		// note: only really needed if numExtraThreads > 0
		pathBarrier = new boost::barrier(numExtraThreads + 1);

		for (unsigned int i = 1; i <= numExtraThreads; i++) {
			pathFinders[i] = new CPathFinder();
			threads[i] = new boost::thread(boost::bind(&CPathEstimator::CalcOffsetsAndPathCosts, this, i));
		}

		// Use the current thread as thread zero
		CalcOffsetsAndPathCosts(0);

		for (unsigned int i = 1; i <= numExtraThreads; i++) {
			threads[i]->join();
			delete threads[i];
			delete pathFinders[i];
		}

		delete pathBarrier;

		loadscreen->SetLoadMessage("PathCosts: writing", true);
		WriteFile(cacheFileName, map);
		loadscreen->SetLoadMessage("PathCosts: written", true);
	}

	pathCache[0] = new CPathCache(nbrOfBlocksX, nbrOfBlocksZ);
	pathCache[1] = new CPathCache(nbrOfBlocksX, nbrOfBlocksZ);
}



void CPathEstimator::InitBlocks() {
	for (unsigned int idx = 0; idx < blockStates.GetSize(); idx++) {
		const unsigned int blockX = idx % nbrOfBlocksX;
		const unsigned int blockZ = idx / nbrOfBlocksX;
		const unsigned int blockNr = blockZ * nbrOfBlocksX + blockX;

		blockStates.peNodeOffsets[blockNr].resize(moveDefHandler->GetNumMoveDefs());
	}
}


__FORCE_ALIGN_STACK__
void CPathEstimator::CalcOffsetsAndPathCosts(unsigned int threadNum) {
	// reset FPU state for synced computations
	streflop::streflop_init<streflop::Simple>();

	if (threadNum > 0) {
		// FIXME: not running any thread on core 0 is a big perf-hit
		// Threading::SetAffinity(1 << threadNum);
		Threading::SetAffinity(~0);
		Threading::SetThreadName(IntToString(threadNum, "pathhelper%i"));
	}

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

	for (unsigned int i = 0; i < moveDefHandler->GetNumMoveDefs(); i++) {
		const MoveDef* md = moveDefHandler->GetMoveDefByPathType(i);

		if (md->udRefCount > 0) {
			blockStates.peNodeOffsets[blockIdx][md->pathType] = FindOffset(*md, x, z);
		}
	}
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

	for (unsigned int i = 0; i < moveDefHandler->GetNumMoveDefs(); i++) {
		const MoveDef* md = moveDefHandler->GetMoveDefByPathType(i);

		if (md->udRefCount > 0) {
			CalculateVertices(*md, x, z, threadNum);
		}
	}
}






/**
 * Finds a square accessable by the given MoveDef within the given block
 */
int2 CPathEstimator::FindOffset(const MoveDef& moveDef, unsigned int blockX, unsigned int blockZ) {
	// lower corner position of block
	const unsigned int lowerX = blockX * BLOCK_SIZE;
	const unsigned int lowerZ = blockZ * BLOCK_SIZE;
	const unsigned int blockArea = (BLOCK_SIZE * BLOCK_SIZE) / SQUARE_SIZE;

	unsigned int bestPosX = BLOCK_SIZE >> 1;
	unsigned int bestPosZ = BLOCK_SIZE >> 1;

	float bestCost = std::numeric_limits<float>::max();
	float speedMod = CMoveMath::GetPosSpeedMod(moveDef, lowerX, lowerZ);

	bool curblock = (speedMod == 0.0f) || CMoveMath::IsBlockedStructure(moveDef, lowerX, lowerZ, NULL);

	// search for an accessible position within this block
	unsigned int x = 0;
	unsigned int z = 0;

	while (z < BLOCK_SIZE) {
		bool zcurblock = curblock;

		while (x < BLOCK_SIZE) {
			if (!curblock) {
				const float dx = x - (float)(BLOCK_SIZE - 1) / 2.0f;
				const float dz = z - (float)(BLOCK_SIZE - 1) / 2.0f;

				const float cost = (dx * dx + dz * dz) + (blockArea / (0.001f + speedMod));

				if (cost < bestCost) {
					bestCost = cost;
					bestPosX = x;
					bestPosZ = z;
				}
			}

			// if last position was not blocked, then we do not need to check the entire square
			speedMod = CMoveMath::GetPosSpeedMod(moveDef, lowerX + x, lowerZ + z);
			curblock = (speedMod == 0.0f) || (curblock ?
				CMoveMath::IsBlockedStructure(moveDef, lowerX + x, lowerZ + z, NULL) :
				CMoveMath::IsBlockedStructureXmax(moveDef, lowerX + x, lowerZ + z, NULL));

			x += 1;
		}

		speedMod = CMoveMath::GetPosSpeedMod(moveDef, lowerX, lowerZ + z);
		curblock = (speedMod == 0.0f) || (zcurblock ?
			CMoveMath::IsBlockedStructure(moveDef, lowerX, lowerZ + z, NULL) :
			CMoveMath::IsBlockedStructureZmax(moveDef, lowerX, lowerZ + z, NULL));

		x  = 0;
		z += 1;
	}

	// return the offset found
	return int2(blockX * BLOCK_SIZE + bestPosX, blockZ * BLOCK_SIZE + bestPosZ);
}


/**
 * Calculate all vertices connected from the given block
 */
void CPathEstimator::CalculateVertices(const MoveDef& moveDef, unsigned int blockX, unsigned int blockZ, unsigned int thread) {
	CalculateVertex(moveDef, blockX, blockZ, PATHDIR_LEFT,     thread);
	CalculateVertex(moveDef, blockX, blockZ, PATHDIR_LEFT_UP,  thread);
	CalculateVertex(moveDef, blockX, blockZ, PATHDIR_UP,       thread);
	CalculateVertex(moveDef, blockX, blockZ, PATHDIR_RIGHT_UP, thread);
}


/**
 * Calculate requested vertex
 */
void CPathEstimator::CalculateVertex(
	const MoveDef& moveDef,
	unsigned int parentBlockX,
	unsigned int parentBlockZ,
	unsigned int direction,
	unsigned int threadNum)
{
	const unsigned int childBlockX = parentBlockX + PE_DIRECTION_VECTORS[direction].x;
	const unsigned int childBlockZ = parentBlockZ + PE_DIRECTION_VECTORS[direction].y;

	const unsigned int parentBlockNbr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	const unsigned int childBlockNbr = childBlockZ * nbrOfBlocksX + childBlockX;
	const unsigned int vertexNbr =
		moveDef.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
		parentBlockNbr * PATH_DIRECTION_VERTICES +
		direction;

	// outside map?
	if (childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertexCosts[vertexNbr] = PATHCOST_INFINITY;
		return;
	}


	// start position within parent block
	const int2 parentSquare = blockStates.peNodeOffsets[parentBlockNbr][moveDef.pathType];

	// goal position within child block
	const int2 childSquare = blockStates.peNodeOffsets[childBlockNbr][moveDef.pathType];

	const float3& startPos = SquareToFloat3(parentSquare.x, parentSquare.y);
	const float3& goalPos = SquareToFloat3(childSquare.x, childSquare.y);

	// keep search exactly contained within the two blocks
	CRectangularSearchConstraint pfDef(startPos, goalPos, BLOCK_SIZE);
	// CCircularSearchConstraint pfDef(startPos, goalPos, 0, 1.1f, 2);

	IPath::Path path;
	IPath::SearchResult result;

	// find path from parent to child block
	//
	// since CPathFinder::GetPath() is not thread-safe, use
	// this thread's "private" CPathFinder instance (rather
	// than locking pathFinder->GetPath()) if we are in one
	result = pathFinders[threadNum]->GetPath(moveDef, pfDef, NULL, startPos, path, MAX_SEARCHED_NODES_PF >> 2, false, true, false, true, true);

	// store the result
	if (result == IPath::Ok) {
		vertexCosts[vertexNbr] = path.pathCost;
	} else {
		vertexCosts[vertexNbr] = PATHCOST_INFINITY;
	}
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
	lowerX = std::max(lowerX,                    0 );
	lowerZ = std::max(lowerZ,                    0 );

	// mark the blocks inside the rectangle, enqueue them
	// from upper to lower because of the placement of the
	// bi-directional vertices
	for (int z = upperZ; z >= lowerZ; z--) {
		for (int x = upperX; x >= lowerX; x--) {
			if ((blockStates.nodeMask[z * nbrOfBlocksX + x] & PATHOPT_OBSOLETE) != 0)
				continue;

			for (unsigned int i = 0; i < moveDefHandler->GetNumMoveDefs(); i++) {
				const MoveDef* md = moveDefHandler->GetMoveDefByPathType(i);

				if (md->udRefCount == 0)
					continue;

				SingleBlock sb;
					sb.blockPos.x = x;
					sb.blockPos.y = z;
					sb.moveDef = md;

				updatedBlocks.push_back(sb);
				blockStates.nodeMask[z * nbrOfBlocksX + x] |= PATHOPT_OBSOLETE;
			}
		}
	}
}


/**
 * Update some obsolete blocks using the FIFO-principle
 */
void CPathEstimator::Update() {
	pathCache[0]->Update();
	pathCache[1]->Update();

	static const unsigned int MIN_BLOCKS_TO_UPDATE = std::max(BLOCKS_TO_UPDATE >> 1, 4U);
	static const unsigned int MAX_BLOCKS_TO_UPDATE = std::min(BLOCKS_TO_UPDATE << 1, MIN_BLOCKS_TO_UPDATE);
	const unsigned int progressiveUpdates = updatedBlocks.size() * 0.007f * ((BLOCK_SIZE >= 16)? 1.0f : 0.6f);
	const unsigned int blocksToUpdate = Clamp(progressiveUpdates, MIN_BLOCKS_TO_UPDATE, MAX_BLOCKS_TO_UPDATE);

	blockUpdatePenalty = std::max(0, blockUpdatePenalty - int(blocksToUpdate));

	if (blockUpdatePenalty >= blocksToUpdate)
		return;

	if (updatedBlocks.empty())
		return;

	std::vector<SingleBlock> consumedBlocks;
	consumedBlocks.reserve(blocksToUpdate);

	int2 curBatchBlockPos = (updatedBlocks.front()).blockPos;
	int2 nxtBatchBlockPos = curBatchBlockPos;

	while (!updatedBlocks.empty()) {
		nxtBatchBlockPos = (updatedBlocks.front()).blockPos;

		if ((blockStates.nodeMask[nxtBatchBlockPos.y * nbrOfBlocksX + nxtBatchBlockPos.x] & PATHOPT_OBSOLETE) == 0) {
			updatedBlocks.pop_front();
			continue;
		}

		// MapChanged ensures format of updatedBlocks is {
		//   (x1,y1,pt1), (x1,y1,pt2), ..., (x1,y1,ptN), // 1st batch
		//   (x2,y2,pt1), (x2,y2,pt2), ..., (x2,y2,ptN), // 2nd batch
		//   ...
		// }
		//
		// always process all MoveDefs of a block in one batch (even
		// if we need to exceed blocksToUpdate to complete the batch)
		//
		// needed because blockStates.nodeMask saves PATHOPT_OBSOLETE
		// for the block as a whole, not for every individual MoveDef
		// (and terrain changes might affect only a subset of MoveDefs)
		//
		// if we didn't use batches, block changes might be missed for
		// MoveDefs lower in the (path-type) ordering so we only allow
		// exiting the loop at batch boundaries
		if (nxtBatchBlockPos != curBatchBlockPos) {
			curBatchBlockPos = nxtBatchBlockPos;

			if (consumedBlocks.size() >= blocksToUpdate) {
				break;
			}
		}

		// no need to check for duplicates, because FindOffset is deterministic
		// so even when we compute it multiple times the result will be the same
		consumedBlocks.push_back(updatedBlocks.front());
		updatedBlocks.pop_front();
	}

	blockUpdatePenalty += std::max(0, int(consumedBlocks.size()) - int(blocksToUpdate));

	// FindOffset (threadsafe)
	{
		SCOPED_TIMER("CPathEstimator::FindOffset");
		for_mt(0, consumedBlocks.size(), [&](const int n) {
			// copy the next block in line
			const SingleBlock sb = consumedBlocks[n];

			const unsigned int blockX = sb.blockPos.x;
			const unsigned int blockZ = sb.blockPos.y;
			const unsigned int blockN = blockZ * nbrOfBlocksX + blockX;

			const MoveDef* currBlockMD = sb.moveDef;

			blockStates.peNodeOffsets[blockN][currBlockMD->pathType] = FindOffset(*currBlockMD, blockX, blockZ);
		});
	}

	// CalculateVertices (not threadsafe)
	{
		SCOPED_TIMER("CPathEstimator::CalculateVertices");
		for (unsigned int n = 0; n < consumedBlocks.size(); ++n) {
			// copy the next block in line
			const SingleBlock sb = consumedBlocks[n];

			const unsigned int blockX = sb.blockPos.x;
			const unsigned int blockZ = sb.blockPos.y;
			const unsigned int blockN = blockZ * nbrOfBlocksX + blockX;

			// check for batch boundary
			const MoveDef* currBlockMD = sb.moveDef;
			const MoveDef* nextBlockMD = ((n + 1) < consumedBlocks.size())? consumedBlocks[n + 1].moveDef: NULL;

			CalculateVertices(*currBlockMD, blockX, blockZ);

			// each MapChanged() call adds AT MOST <moveDefs.size()> SingleBlock's
			// in ascending pathType order per (x, z) PE-block, therefore when the
			// next SingleBlock's pathType is less or equal to the current we know
			// that all have been processed (for one PE-block)
			if (nextBlockMD == NULL || nextBlockMD->pathType <= currBlockMD->pathType) {
				blockStates.nodeMask[blockN] &= ~PATHOPT_OBSOLETE;
			}
		}
	}
}


/**
 * Stores data and does some top-administration
 */
IPath::SearchResult CPathEstimator::GetPath(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	float3 start,
	IPath::Path& path,
	unsigned int maxSearchedBlocks,
	bool synced
) {
	start.ClampInBounds();

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

	const CPathCache::CacheItem* ci = pathCache[synced]->GetCachedPath(startBlock, goalBlock, peDef.sqGoalRadius, moveDef.pathType);

	if (ci != NULL) {
		// use a cached path if we have one
		path = ci->path;
		return ci->result;
	}

	// oterhwise search
	const IPath::SearchResult result = InitSearch(moveDef, peDef, synced);

	// if search successful, generate new path
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, path);

		if (result == IPath::Ok) {
			// add succesful paths to the cache
			pathCache[synced]->AddPath(&path, result, startBlock, goalBlock, peDef.sqGoalRadius, moveDef.pathType);
		}

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "PE: Search completed.");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "Path length: " _STPF_, path.path.size());
			LOG_L(L_DEBUG, "Path cost: %f", path.pathCost);
		}
	} else {
		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "PE: Search failed!");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
		}
	}

	return result;
}


// set up the starting point of the search
IPath::SearchResult CPathEstimator::InitSearch(const MoveDef& moveDef, const CPathFinderDef& peDef, bool synced) {
	const int2 square = blockStates.peNodeOffsets[mStartBlockIdx][moveDef.pathType];
	const bool isStartGoal = peDef.IsGoal(square.x, square.y);

	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && peDef.startInGoalRadius)
		return IPath::CantGetCloser;

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockStates.nodeMask[mStartBlockIdx] |= PATHOPT_OPEN;
	blockStates.fCost[mStartBlockIdx] = 0.0f;
	blockStates.gCost[mStartBlockIdx] = 0.0f;
	blockStates.SetMaxCost(NODE_COST_F, 0.0f);
	blockStates.SetMaxCost(NODE_COST_G, 0.0f);

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
	mGoalHeuristic = peDef.Heuristic(square.x, square.y);

	// get the goal square offset
	mGoalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, peDef, synced);

	// if no improvements are found, then return CantGetCloser instead
	if (mGoalBlock.x == mStartBlock.x && mGoalBlock.y == mStartBlock.y && (!isStartGoal || peDef.startInGoalRadius)) {
		return IPath::CantGetCloser;
	}

	return result;
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathEstimator::DoSearch(const MoveDef& moveDef, const CPathFinderDef& peDef, bool synced) {
	bool foundGoal = false;

	while (!openBlocks.empty() && (openBlockBuffer.GetSize() < maxBlocksToBeSearched)) {
		// get the open block with lowest cost
		PathNode* ob = const_cast<PathNode*>(openBlocks.top());
		openBlocks.pop();

		// check if the block has been marked as unaccessible during its time in the queue
		if (blockStates.nodeMask[ob->nodeNum] & (PATHOPT_BLOCKED | PATHOPT_CLOSED | PATHOPT_FORBIDDEN))
			continue;

		// no, check if the goal is already reached
		const unsigned int xBSquare = blockStates.peNodeOffsets[ob->nodeNum][moveDef.pathType].x;
		const unsigned int zBSquare = blockStates.peNodeOffsets[ob->nodeNum][moveDef.pathType].y;
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
		TestBlock(moveDef, peDef, *ob, PATHDIR_LEFT,       synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_LEFT_UP,    synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_UP,         synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_RIGHT_UP,   synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_RIGHT,      synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_RIGHT_DOWN, synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_DOWN,       synced);
		TestBlock(moveDef, peDef, *ob, PATHDIR_LEFT_DOWN,  synced);

		// mark this block as closed
		blockStates.nodeMask[ob->nodeNum] |= PATHOPT_CLOSED;
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
	LOG_L(L_ERROR, "%s - Unhandled end of search!", __FUNCTION__);
	return IPath::Error;
}


/**
 * Test the accessability of a block and its value,
 * possibly also add it to the open-blocks pqueue.
 */
void CPathEstimator::TestBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	PathNode& parentOpenBlock,
	unsigned int pathDir,
	bool synced
) {
	testedBlocks++;

	// initial calculations of the new block
	const int2 block = parentOpenBlock.nodePos + PE_DIRECTION_VECTORS[pathDir];

	const int vertexIdx =
		moveDef.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
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
	if (blockStates.nodeMask[blockIdx] & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	const int2 square = blockStates.peNodeOffsets[blockIdx][moveDef.pathType];

	// check if the block is blocked or out of constraints
	if (!peDef.WithinConstraints(square.x, square.y)) {
		blockStates.nodeMask[blockIdx] |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blockIdx);
		return;
	}

	// evaluate this node (NOTE the max-resolution indexing for {flow,extra}Cost)
	const float flowCost = (PathFlowMap::GetInstance())->GetFlowCost(square.x, square.y, moveDef, PathDir2PathOpt(pathDir));
	const float extraCost = blockStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = vertexCosts[vertexIdx] + flowCost + extraCost;

	const float gCost = parentOpenBlock.gCost + nodeCost;
	const float hCost = peDef.Heuristic(square.x, square.y);
	const float fCost = gCost + hCost;


	if (blockStates.nodeMask[blockIdx] & PATHOPT_OPEN) {
		// already in the open set
		if (blockStates.fCost[blockIdx] <= fCost)
			return;

		blockStates.nodeMask[blockIdx] &= ~PATHOPT_CARDINALS;
	}

	// look for improvements
	if (hCost < mGoalHeuristic) {
		mGoalBlock = block;
		mGoalHeuristic = hCost;
	}

	// store this block as open
	openBlockBuffer.SetSize(openBlockBuffer.GetSize() + 1);
	assert(openBlockBuffer.GetSize() < MAX_SEARCHED_NODES_PE);

	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = fCost;
		ob->gCost   = gCost;
		ob->nodePos = block;
		ob->nodeNum = blockIdx;
	openBlocks.push(ob);

	blockStates.SetMaxCost(NODE_COST_F, std::max(blockStates.GetMaxCost(NODE_COST_F), fCost));
	blockStates.SetMaxCost(NODE_COST_G, std::max(blockStates.GetMaxCost(NODE_COST_G), gCost));

	// mark this block as open
	blockStates.fCost[blockIdx] = fCost;
	blockStates.gCost[blockIdx] = gCost;
	blockStates.nodeMask[blockIdx] |= (pathDir | PATHOPT_OPEN);
	blockStates.peParentNodePos[blockIdx] = parentOpenBlock.nodePos;

	dirtyBlocks.push_back(blockIdx);
}


/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveDef& moveDef, IPath::Path& foundPath) {
	int2 block = mGoalBlock;

	while (block.x != mStartBlock.x || block.y != mStartBlock.y) {
		const unsigned int blockIdx = block.y * nbrOfBlocksX + block.x;

		// use offset defined by the block
		const int2 bsquare = blockStates.peNodeOffsets[blockIdx][moveDef.pathType];
		const float3& pos = SquareToFloat3(bsquare.x, bsquare.y);

		foundPath.path.push_back(pos);

		// next step backwards
		block = blockStates.peParentNodePos[blockIdx];
	}

	if (!foundPath.path.empty()) {
		foundPath.pathGoal = foundPath.path.front();
	}

	// set some additional information
	foundPath.pathCost = blockStates.fCost[mGoalBlock.y * nbrOfBlocksX + mGoalBlock.x] - mGoalHeuristic;
}


/**
 * Clean lists from last search
 */
void CPathEstimator::ResetSearch() {
	openBlocks.Clear();

	while (!dirtyBlocks.empty()) {
		blockStates.ClearSquare(dirtyBlocks.back());
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
	char hashString[64] = {0};

	sprintf(hashString, "%u", hash);
	LOG("[PathEstimator::%s] hash=%s\n", __FUNCTION__, hashString);

	std::string filename = GetPathCacheDir() + map + hashString + "." + cacheFileName + ".zip";
	if (!FileSystem::FileExists(filename))
		return false;
	// open file for reading from a suitable location (where the file exists)
	IArchive* pfile = archiveLoader.OpenArchive(dataDirsAccess.LocateFile(filename), "sdz");

	if (!pfile || !pfile->IsOpen()) {
		delete pfile;
		return false;
	}

	char calcMsg[512];
	sprintf(calcMsg, "Reading Estimate PathCosts [%d]", BLOCK_SIZE);
	loadscreen->SetLoadMessage(calcMsg);

	std::auto_ptr<IArchive> auto_pfile(pfile);
	IArchive& file(*pfile);

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
		const unsigned blockSize = moveDefHandler->GetNumMoveDefs() * sizeof(int2);
		if (buffer.size() < pos + blockSize * blockStates.GetSize())
			return false;

		for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++) {
			std::memcpy(&blockStates.peNodeOffsets[blocknr][0], &buffer[pos], blockSize);
			pos += blockSize;
		}

		// Read vertices data.
		if (buffer.size() < pos + vertexCosts.size() * sizeof(float))
			return false;
		std::memcpy(&vertexCosts[0], &buffer[pos], vertexCosts.size() * sizeof(float));

		// File read successful.
		return true;
	}

	return false;
}


/**
 * Try to write offset and vertex data to file.
 */
void CPathEstimator::WriteFile(const std::string& cacheFileName, const std::string& map)
{
	// We need this directory to exist
	if (!FileSystem::CreateDirectory(GetPathCacheDir()))
		return;

	const unsigned int hash = Hash();
	char hashString[64] = {0};

	sprintf(hashString, "%u", hash);
	LOG("[PathEstimator::%s] hash=%s\n", __FUNCTION__, hashString);

	const std::string filename = GetPathCacheDir() + map + hashString + "." + cacheFileName + ".zip";

	// open file for writing in a suitable location
	zipFile file = zipOpen(dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE).c_str(), APPEND_STATUS_CREATE);

	if (file == NULL)
		return;

	zipOpenNewFileInZip(file, "pathinfo", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);

	// Write hash. (NOTE: this also affects the CRC!)
	zipWriteInFileInZip(file, (void*) &hash, 4);

	// Write block-center-offsets.
	for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++)
		zipWriteInFileInZip(file, (void*) &blockStates.peNodeOffsets[blocknr][0], moveDefHandler->GetNumMoveDefs() * sizeof(int2));

	// Write vertices.
	zipWriteInFileInZip(file, &vertexCosts[0], vertexCosts.size() * sizeof(float));

	zipCloseFileInZip(file);
	zipClose(file, NULL);


	// get the CRC over the written path data
	IArchive* pfile = archiveLoader.OpenArchive(dataDirsAccess.LocateFile(filename), "sdz");

	if (pfile == NULL)
		return;

	if (pfile->IsOpen()) {
		assert(pfile->FindFile("pathinfo") < pfile->NumFiles());
		pathChecksum = pfile->GetCrc32(pfile->FindFile("pathinfo"));
	}

	delete pfile;
}


/**
 * Returns a hash-code identifying the dataset of this estimator.
 * FIXME: uses checksum of raw heightmap (before Lua has seen it)
 */
unsigned int CPathEstimator::Hash() const
{
	return (readMap->GetMapChecksum() + moveDefHandler->GetCheckSum() + BLOCK_SIZE + PATHESTIMATOR_VERSION);
}
