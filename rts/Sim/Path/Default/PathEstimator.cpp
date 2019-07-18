/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"

#include "minizip/zip.h"

#include "PathEstimator.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
// #include "PathFlowMap.hpp"
#include "PathLog.h"
#include "PathMemPool.h"
#include "Game/GlobalUnsynced.h"
#include "Game/LoadScreen.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Threading/ThreadPool.h" // for_mt
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/Archives/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Platform/Threading.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/Sync/SHA512.hpp"

#define ENABLE_NETLOG_CHECKSUM 1

CONFIG(int, PathingThreadCount).defaultValue(0).safemodeValue(1).minimumValue(0);
CONFIG(int, MaxPathCostsMemoryFootPrint).defaultValue(512).minimumValue(64).description("Maximum memusage (in MByte) of multithreaded pathcache generator at loading time.");

PCMemPool pcMemPool;
PEMemPool peMemPool;


static const std::string GetPathCacheDir() {
	return (FileSystem::GetCacheDir() + "/paths/");
}

static const std::string GetCacheFileName(const std::string& fileHashCode, const std::string& peFileName, const std::string& mapFileName) {
	return (GetPathCacheDir() + mapFileName + "." + peFileName + "-" + fileHashCode + ".zip");
}


static size_t GetNumThreads() {
	const size_t numThreads = std::max(0, configHandler->GetInt("PathingThreadCount"));
	const size_t numCores = Threading::GetLogicalCpuCores();
	return ((numThreads == 0)? numCores: numThreads);
}



void CPathEstimator::Init(IPathFinder* pf, unsigned int BLOCK_SIZE, const std::string& peFileName, const std::string& mapFileName)
{
	IPathFinder::Init(BLOCK_SIZE);

	{
		BLOCKS_TO_UPDATE = SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1;

		blockUpdatePenalty = 0;
		nextOffsetMessageIdx = 0;
		nextCostMessageIdx = 0;

		pathChecksum = 0;
		fileHashCode = CalcHash(__func__);

		offsetBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};
		costBlockNum = {nbrOfBlocks.x * nbrOfBlocks.y};

		parentPathFinder = pf;
		nextPathEstimator = nullptr;
	}
	{
		vertexCosts.clear();
		vertexCosts.resize(moveDefHandler.GetNumMoveDefs() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, PATHCOST_INFINITY);
		maxSpeedMods.clear();
		maxSpeedMods.resize(moveDefHandler.GetNumMoveDefs(), 0.001f);

		updatedBlocks.clear();
		consumedBlocks.clear();
		offsetBlocksSortedByCost.clear();
	}

	CPathEstimator*  childPE = this;
	CPathEstimator* parentPE = dynamic_cast<CPathEstimator*>(pf);

	if (parentPE != nullptr)
		parentPE->nextPathEstimator = childPE;

	// precalc for FindBlockPosOffset()
	{
		offsetBlocksSortedByCost.reserve(BLOCK_SIZE * BLOCK_SIZE);
		for (unsigned int z = 0; z < BLOCK_SIZE; ++z) {
			for (unsigned int x = 0; x < BLOCK_SIZE; ++x) {
				const float dx = x - (float)(BLOCK_SIZE - 1) * 0.5f;
				const float dz = z - (float)(BLOCK_SIZE - 1) * 0.5f;
				const float cost = (dx * dx + dz * dz);

				offsetBlocksSortedByCost.emplace_back(cost, x, z);
			}
		}
		std::stable_sort(offsetBlocksSortedByCost.begin(), offsetBlocksSortedByCost.end(), [](const SOffsetBlock& a, const SOffsetBlock& b) {
			return (a.cost < b.cost);
		});
	}

	if (BLOCK_SIZE == LOWRES_PE_BLOCKSIZE) {
		assert(parentPE != nullptr);

		// calculate map-wide maximum positional speedmod for each MoveDef
		for_mt(0, moveDefHandler.GetNumMoveDefs(), [&](unsigned int i) {
			const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

			for (int y = 0; y < mapDims.mapy; y++) {
				for (int x = 0; x < mapDims.mapx; x++) {
					childPE->maxSpeedMods[i] = std::max(childPE->maxSpeedMods[i], CMoveMath::GetPosSpeedMod(*md, x, y));
				}
			}
		});

		// calculate reciprocals, avoids divisions in TestBlock
		for (unsigned int i = 0; i < maxSpeedMods.size(); i++) {
			 childPE->maxSpeedMods[i] = 1.0f / childPE->maxSpeedMods[i];
			parentPE->maxSpeedMods[i] = childPE->maxSpeedMods[i];
		}
	}

	// load precalculated data if it exists
	InitEstimator(peFileName, mapFileName);
}


void CPathEstimator::Kill()
{
	pcMemPool.free(pathCache[0]);
	pcMemPool.free(pathCache[1]);
}


void CPathEstimator::InitEstimator(const std::string& peFileName, const std::string& mapFileName)
{
	const unsigned int numThreads = GetNumThreads();

	if (threads.size() != numThreads) {
		threads.clear();
		threads.resize(numThreads);
		pathFinders.clear();
		pathFinders.resize(numThreads);
	}

	// always use PF for initialization, later PE maybe used
	// TODO: pooling these will not help much, need to reuse
	pathFinders[0] = pfMemPool.alloc<CPathFinder>(true);

	// Not much point in multithreading these...
	InitBlocks();

	if (!ReadFile(peFileName, mapFileName)) {
		// start extra threads if applicable, but always keep the total
		// memory-footprint made by CPathFinder instances within bounds
		const unsigned int minMemFootPrint = sizeof(CPathFinder) + parentPathFinder->GetMemFootPrint();
		const unsigned int maxMemFootPrint = configHandler->GetInt("MaxPathCostsMemoryFootPrint") * 1024 * 1024;
		const unsigned int numExtraThreads = Clamp(int(maxMemFootPrint / minMemFootPrint) - 1, 0, int(numThreads) - 1);
		const unsigned int reqMemFootPrint = minMemFootPrint * (numExtraThreads + 1);

		char calcMsg[512];
		const char* fmtStrs[4] = {
			"[%s] creating PE%u cache with %u PF threads (%u MB)",
			"[%s] creating PE%u cache with %u PF thread (%u MB)",
			"[%s] writing PE%u cache-file %s-%x",
			"[%s] written PE%u cache-file %s-%x",
		};

		{
			sprintf(calcMsg, fmtStrs[numExtraThreads == 0], __func__, BLOCK_SIZE, numExtraThreads + 1, reqMemFootPrint / (1024 * 1024));
			loadscreen->SetLoadMessage(calcMsg);
		}


		// note: only really needed if numExtraThreads > 0
		spring::barrier pathBarrier(numExtraThreads + 1);

		for (unsigned int i = 1; i <= numExtraThreads; i++) {
			pathFinders[i] = pfMemPool.alloc<CPathFinder>(true);
			threads[i] = std::move(spring::thread(&CPathEstimator::CalcOffsetsAndPathCosts, this, i, &pathBarrier));
		}

		// Use the current thread as thread zero
		CalcOffsetsAndPathCosts(0, &pathBarrier);

		for (unsigned int i = 1; i <= numExtraThreads; i++) {
			threads[i].join();
			pfMemPool.free(pathFinders[i]);
		}


		sprintf(calcMsg, fmtStrs[2], __func__, BLOCK_SIZE, peFileName.c_str(), fileHashCode);
		loadscreen->SetLoadMessage(calcMsg, true);

		WriteFile(peFileName, mapFileName);

		sprintf(calcMsg, fmtStrs[3], __func__, BLOCK_SIZE, peFileName.c_str(), fileHashCode);
		loadscreen->SetLoadMessage(calcMsg, true);
	}

	// calculate checksum over block-offsets and vertex-costs
	pathChecksum = CalcChecksum();

	// switch to runtime wanted IPathFinder (maybe PF or PE)
	pfMemPool.free(pathFinders[0]);
	pathFinders[0] = parentPathFinder;

	pathCache[0] = pcMemPool.alloc<CPathCache>(nbrOfBlocks.x, nbrOfBlocks.y);
	pathCache[1] = pcMemPool.alloc<CPathCache>(nbrOfBlocks.x, nbrOfBlocks.y);
}


void CPathEstimator::InitBlocks()
{
	blockStates.peNodeOffsets.resize(moveDefHandler.GetNumMoveDefs());
	for (unsigned int idx = 0; idx < moveDefHandler.GetNumMoveDefs(); idx++) {
		blockStates.peNodeOffsets[idx].resize(nbrOfBlocks.x * nbrOfBlocks.y);
	}
}


__FORCE_ALIGN_STACK__
void CPathEstimator::CalcOffsetsAndPathCosts(unsigned int threadNum, spring::barrier* pathBarrier)
{
	// reset FPU state for synced computations
	streflop::streflop_init<streflop::Simple>();

	if (threadNum > 0) {
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
	const int2 blockPos = BlockIdxToPos(blockIdx);

	if (threadNum == 0 && blockIdx >= nextOffsetMessageIdx) {
		nextOffsetMessageIdx = blockIdx + blockStates.GetSize() / 16;
		clientNet->Send(CBaseNetProtocol::Get().SendCPUUsage(BLOCK_SIZE | (blockIdx << 8)));
	}

	for (unsigned int i = 0; i < moveDefHandler.GetNumMoveDefs(); i++) {
		const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

		blockStates.peNodeOffsets[md->pathType][blockIdx] = FindBlockPosOffset(*md, blockPos.x, blockPos.y);
	}
}


void CPathEstimator::EstimatePathCosts(unsigned int blockIdx, unsigned int threadNum)
{
	const int2 blockPos = BlockIdxToPos(blockIdx);

	if (threadNum == 0 && blockIdx >= nextCostMessageIdx) {
		nextCostMessageIdx = blockIdx + blockStates.GetSize() / 16;

		char calcMsg[128];
		sprintf(calcMsg, "[%s] precached %d of %d blocks", __func__, blockIdx, blockStates.GetSize());

		clientNet->Send(CBaseNetProtocol::Get().SendCPUUsage(0x1 | BLOCK_SIZE | (blockIdx << 8)));
		loadscreen->SetLoadMessage(calcMsg, (blockIdx != 0));
	}

	for (unsigned int i = 0; i < moveDefHandler.GetNumMoveDefs(); i++) {
		const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

		CalcVertexPathCosts(*md, blockPos, threadNum);
	}
}


/**
 * Move around the blockPos a bit, so we `surround` unpassable blocks.
 */
int2 CPathEstimator::FindBlockPosOffset(const MoveDef& moveDef, unsigned int blockX, unsigned int blockZ) const
{
	// lower corner position of block
	const unsigned int lowerX = blockX * BLOCK_SIZE;
	const unsigned int lowerZ = blockZ * BLOCK_SIZE;
	const unsigned int blockArea = (BLOCK_SIZE * BLOCK_SIZE) / SQUARE_SIZE;

	int2 bestPos(lowerX + (BLOCK_SIZE >> 1), lowerZ + (BLOCK_SIZE >> 1));
	float bestCost = std::numeric_limits<float>::max();

	// search for an accessible position within this block
	/*for (unsigned int z = 0; z < BLOCK_SIZE; ++z) {
		for (unsigned int x = 0; x < BLOCK_SIZE; ++x) {
			const float speedMod = CMoveMath::GetPosSpeedMod(moveDef, lowerX + x, lowerZ + z);
			const bool curblock = (speedMod == 0.0f) || CMoveMath::IsBlockedStructure(moveDef, lowerX + x, lowerZ + z, nullptr);

			if (!curblock) {
				const float dx = x - (float)(BLOCK_SIZE - 1) * 0.5f;
				const float dz = z - (float)(BLOCK_SIZE - 1) * 0.5f;
				const float cost = (dx * dx + dz * dz) + (blockArea / (0.001f + speedMod));

				if (cost < bestCost) {
					bestCost = cost;
					bestPos.x = lowerX + x;
					bestPos.y = lowerZ + z;
				}
			}
		}
	}*/

	// same as above, but with squares sorted by their baseCost
	// s.t. we can exit early when a square exceeds our current
	// best (from testing, on avg. 40% of blocks can be skipped)
	for (const SOffsetBlock& ob: offsetBlocksSortedByCost) {
		if (ob.cost >= bestCost)
			break;

		const int2 blockPos(lowerX + ob.offset.x, lowerZ + ob.offset.y);
		const float speedMod = CMoveMath::GetPosSpeedMod(moveDef, blockPos.x, blockPos.y);

		//assert((blockArea / (0.001f + speedMod) >= 0.0f);
		const float cost = ob.cost + (blockArea / (0.001f + speedMod));

		if (cost >= bestCost)
			continue;

		if (!CMoveMath::IsBlockedStructure(moveDef, blockPos.x, blockPos.y, nullptr)) {
			bestCost = cost;
			bestPos  = blockPos;
		}
	}

	// return the offset found
	return bestPos;
}


/**
 * Calculate costs of paths to all vertices connected from the given block
 */
void CPathEstimator::CalcVertexPathCosts(const MoveDef& moveDef, int2 block, unsigned int threadNum)
{
	// see GetBlockVertexOffset(); costs are bi-directional and only
	// calculated for *half* the outgoing edges (while costs for the
	// other four directions are stored at the adjacent vertices)
	CalcVertexPathCost(moveDef, block, PATHDIR_LEFT,     threadNum);
	CalcVertexPathCost(moveDef, block, PATHDIR_LEFT_UP,  threadNum);
	CalcVertexPathCost(moveDef, block, PATHDIR_UP,       threadNum);
	CalcVertexPathCost(moveDef, block, PATHDIR_RIGHT_UP, threadNum);
}

void CPathEstimator::CalcVertexPathCost(
	const MoveDef& moveDef,
	int2 parentBlockPos,
	unsigned int pathDir,
	unsigned int threadNum
) {
	const int2 childBlockPos = parentBlockPos + PE_DIRECTION_VECTORS[pathDir];

	const unsigned int parentBlockIdx = BlockPosToIdx(parentBlockPos);
	const unsigned int  childBlockIdx = BlockPosToIdx( childBlockPos);
	const unsigned int  vertexCostIdx =
		moveDef.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
		parentBlockIdx * PATH_DIRECTION_VERTICES +
		pathDir;

	// outside map?
	if ((unsigned)childBlockPos.x >= nbrOfBlocks.x || (unsigned)childBlockPos.y >= nbrOfBlocks.y) {
		vertexCosts[vertexCostIdx] = PATHCOST_INFINITY;
		return;
	}


	// start position within parent block, goal position within child block
	const int2 parentSquare = blockStates.peNodeOffsets[moveDef.pathType][parentBlockIdx];
	const int2  childSquare = blockStates.peNodeOffsets[moveDef.pathType][ childBlockIdx];

	const float3 startPos = SquareToFloat3(parentSquare.x, parentSquare.y);
	const float3  goalPos = SquareToFloat3( childSquare.x,  childSquare.y);

	// keep search exactly contained within the two blocks
	CRectangularSearchConstraint pfDef(startPos, goalPos, 0.0f, BLOCK_SIZE);

	// we never want to allow searches from any blocked starting positions
	// (otherwise PE and PF can disagree), but are more lenient for normal
	// searches so players can "unstuck" units
	// note: PE itself should ensure this never happens to begin with?
	//
	// blocked goal positions are always early-outs (no searching needed)
	const bool strtBlocked = ((CMoveMath::IsBlocked(moveDef, startPos, nullptr) & CMoveMath::BLOCK_STRUCTURE) != 0);
	const bool goalBlocked = pfDef.IsGoalBlocked(moveDef, CMoveMath::BLOCK_STRUCTURE, nullptr);

	if (strtBlocked || goalBlocked) {
		vertexCosts[vertexCostIdx] = PATHCOST_INFINITY;
		return;
	}

	// find path from parent to child block
	//
	// since CPathFinder::GetPath() is not thread-safe, use
	// this thread's "private" CPathFinder instance (rather
	// than locking parentPathFinder->GetPath()) if we are
	// invoked in one
	pfDef.skipSubSearches = true;
	pfDef.testMobile      = false;
	pfDef.needPath        = false;
	pfDef.exactPath       = true;
	pfDef.dirIndependent  = true;

	IPath::Path path;
	IPath::SearchResult result = pathFinders[threadNum]->GetPath(moveDef, pfDef, nullptr, startPos, path, MAX_SEARCHED_NODES_PF >> 2);

	// store the result
	if (result == IPath::Ok) {
		vertexCosts[vertexCostIdx] = path.pathCost;
	} else {
		vertexCosts[vertexCostIdx] = PATHCOST_INFINITY;
	}
}


/**
 * Mark affected blocks as obsolete
 */
void CPathEstimator::MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned z2)
{
	assert(x2 >= x1);
	assert(z2 >= z1);

	// find the upper and lower corner of the rectangular area
	const int lowerX = Clamp(int(x1 / BLOCK_SIZE) - 1, 0, int(nbrOfBlocks.x - 1));
	const int upperX = Clamp(int(x2 / BLOCK_SIZE) + 1, 0, int(nbrOfBlocks.x - 1));
	const int lowerZ = Clamp(int(z1 / BLOCK_SIZE) - 1, 0, int(nbrOfBlocks.y - 1));
	const int upperZ = Clamp(int(z2 / BLOCK_SIZE) + 1, 0, int(nbrOfBlocks.y - 1));

	// mark the blocks inside the rectangle, enqueue them
	// from upper to lower because of the placement of the
	// bi-directional vertices
	for (int z = upperZ; z >= lowerZ; z--) {
		for (int x = upperX; x >= lowerX; x--) {
			const int idx = BlockPosToIdx(int2(x, z));

			if ((blockStates.nodeMask[idx] & PATHOPT_OBSOLETE) != 0)
				continue;

			updatedBlocks.emplace_back(x, z);
			blockStates.nodeMask[idx] |= PATHOPT_OBSOLETE;
		}
	}
}


/**
 * Update some obsolete blocks using the FIFO-principle
 */
void CPathEstimator::Update()
{
	pathCache[0]->Update();
	pathCache[1]->Update();

	const unsigned int numMoveDefs = moveDefHandler.GetNumMoveDefs();

	if (numMoveDefs == 0)
		return;

	// determine how many blocks we should update
	int blocksToUpdate = 0;
	int consumeBlocks = 0;
	{
		const int progressiveUpdates = updatedBlocks.size() * numMoveDefs * modInfo.pfUpdateRate;
		const int MIN_BLOCKS_TO_UPDATE = std::max<int>(BLOCKS_TO_UPDATE >> 1, 4U);
		const int MAX_BLOCKS_TO_UPDATE = std::max<int>(BLOCKS_TO_UPDATE << 1, MIN_BLOCKS_TO_UPDATE);

		blocksToUpdate = Clamp(progressiveUpdates, MIN_BLOCKS_TO_UPDATE, MAX_BLOCKS_TO_UPDATE);
		blockUpdatePenalty = std::max(0, blockUpdatePenalty - blocksToUpdate);

		if (blockUpdatePenalty > 0)
			blocksToUpdate = std::max(0, blocksToUpdate - blockUpdatePenalty);

		// we have to update blocks for all movedefs (PATHOPT_OBSOLETE applies per block, not per movedef)
		consumeBlocks = int(progressiveUpdates != 0) * int(ceil(float(blocksToUpdate) / numMoveDefs)) * numMoveDefs;
		blockUpdatePenalty += consumeBlocks;
	}

	if (blocksToUpdate == 0)
		return;

	if (updatedBlocks.empty())
		return;

	consumedBlocks.clear();
	consumedBlocks.reserve(consumeBlocks);

	// get blocks to update
	while (!updatedBlocks.empty()) {
		const int2& pos = updatedBlocks.front();
		const int idx = BlockPosToIdx(pos);

		if ((blockStates.nodeMask[idx] & PATHOPT_OBSOLETE) == 0) {
			updatedBlocks.pop_front();
			continue;
		}

		if (consumedBlocks.size() >= blocksToUpdate)
			break;

		// issue repathing for all active movedefs
		for (unsigned int i = 0; i < numMoveDefs; i++) {
			const MoveDef* md = moveDefHandler.GetMoveDefByPathType(i);

			consumedBlocks.emplace_back(pos, md);
		}

		// inform dependent estimator that costs were updated and it should do the same
		// FIXME?
		//   adjacent med-res PE blocks will cause a low-res block to be updated twice
		//   (in addition to the overlap that already exists because MapChanged() adds
		//   boundary blocks)
		if (true && nextPathEstimator != nullptr)
			nextPathEstimator->MapChanged(pos.x * BLOCK_SIZE, pos.y * BLOCK_SIZE, pos.x * BLOCK_SIZE, pos.y * BLOCK_SIZE);

		updatedBlocks.pop_front(); // must happen _after_ last usage of the `pos` reference!
		blockStates.nodeMask[idx] &= ~PATHOPT_OBSOLETE;
	}

	// FindOffset (threadsafe)
	{
		SCOPED_TIMER("Sim::Path::Estimator::FindOffset");
		for_mt(0, consumedBlocks.size(), [&](const int n) {
			// copy the next block in line
			const SingleBlock sb = consumedBlocks[n];
			const int blockN = BlockPosToIdx(sb.blockPos);
			const MoveDef* currBlockMD = sb.moveDef;
			blockStates.peNodeOffsets[currBlockMD->pathType][blockN] = FindBlockPosOffset(*currBlockMD, sb.blockPos.x, sb.blockPos.y);
		});
	}

	// CalcVertexPathCosts (not threadsafe)
	{
		SCOPED_TIMER("Sim::Path::Estimator::CalcVertexPathCosts");
		for (unsigned int n = 0; n < consumedBlocks.size(); ++n) {
			CalcVertexPathCosts(*consumedBlocks[n].moveDef, consumedBlocks[n].blockPos);
		}
	}
}


const CPathCache::CacheItem& CPathEstimator::GetCache(const int2 strtBlock, const int2 goalBlock, float goalRadius, int pathType, const bool synced) const
{
	return pathCache[synced]->GetCachedPath(strtBlock, goalBlock, goalRadius, pathType);
}

void CPathEstimator::AddCache(const IPath::Path* path, const IPath::SearchResult result, const int2 strtBlock, const int2 goalBlock, float goalRadius, int pathType, const bool synced)
{
	pathCache[synced]->AddPath(path, result, strtBlock, goalBlock, goalRadius, pathType);
}



IPath::SearchResult CPathEstimator::DoBlockSearch(
	const CSolidObject* owner,
	const MoveDef& moveDef,
	const int2 s,
	const int2 g
) {
	const float3 sw = float3(s.x * SQUARE_SIZE, 0, s.y * SQUARE_SIZE);
	const float3 gw = float3(g.x * SQUARE_SIZE, 0, g.y * SQUARE_SIZE);

	return (DoBlockSearch(owner, moveDef, sw, gw));
}

IPath::SearchResult CPathEstimator::DoBlockSearch(
	const CSolidObject* owner,
	const MoveDef& moveDef,
	const float3 sw,
	const float3 gw
) {
	// always use max-res (in addition to raw) search for this
	IPathFinder* pf = (BLOCK_SIZE == 32)? parentPathFinder->GetParent(): parentPathFinder;
	CRectangularSearchConstraint pfDef = CRectangularSearchConstraint(sw, gw, 8.0f, BLOCK_SIZE); // sets goalSquare{X,Z}
	IPath::Path path;

	pfDef.testMobile     = false;
	pfDef.needPath       = false;
	pfDef.exactPath      = true;
	pfDef.allowRawPath   = true;
	pfDef.dirIndependent = true;

	// search within the rectangle defined by sw and gw, corners snapped to the PE grid
	return (pf->GetPath(moveDef, pfDef, owner, sw, path, MAX_SEARCHED_NODES_PF >> 3));
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathEstimator::DoSearch(const MoveDef& moveDef, const CPathFinderDef& peDef, const CSolidObject* owner)
{
	bool foundGoal = false;

	// get the goal square offset
	const int2 goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);
	const float maxSpeedMod = maxSpeedMods[moveDef.pathType];

	while (!openBlocks.empty() && (openBlockBuffer.GetSize() < maxBlocksToBeSearched)) {
		// get the open block with lowest cost
		const PathNode* ob = openBlocks.top();
		openBlocks.pop();

		// check if the block has been marked as unaccessible during its time in the queue
		if (blockStates.nodeMask[ob->nodeNum] & (PATHOPT_BLOCKED | PATHOPT_CLOSED))
			continue;

		// no, check if the goal is already reached
		const int2 bSquare = blockStates.peNodeOffsets[moveDef.pathType][ob->nodeNum];
		const int2 gSquare = ob->nodePos * BLOCK_SIZE + goalSqrOffset;

		bool runBlkSearch = false;
		bool canReachGoal =  true;

		// NOTE:
		//   this is a radius-based check, so gSquare can be considered the goal
		//   even if it can not be reached (via a maximum-res path) from bSquare
		//   basically the same condition as in TestBlock, except needed for the
		//   case that <ob> neighbors the goal but no actual edge leads to it
		if (peDef.IsGoal(bSquare.x, bSquare.y) || (runBlkSearch = peDef.IsGoal(gSquare.x, gSquare.y))) {
			if (runBlkSearch)
				canReachGoal = (DoBlockSearch(owner, moveDef, bSquare, gSquare) == IPath::Ok);

			if (canReachGoal) {
				mGoalBlockIdx = ob->nodeNum;
				mGoalHeuristic = 0.0f;
				foundGoal = true;
				break;
			}
		}

		// no, test the 8 surrounding blocks
		// NOTE:
		//   each of these calls increments openBlockBuffer.idx by (at most) 1, so
		//   maxBlocksToBeSearched is always less than <MAX_SEARCHED_NODES_PE - 8>
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT,       PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT_UP,    PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_UP,         PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT_UP,   PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT,      PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_RIGHT_DOWN, PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_DOWN,       PATHOPT_OPEN, maxSpeedMod);
		TestBlock(moveDef, peDef, ob, owner, PATHDIR_LEFT_DOWN,  PATHOPT_OPEN, maxSpeedMod);

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
	LOG_L(L_ERROR, "%s - Unhandled end of search!", __func__);
	return IPath::Error;
}


/**
 * Test the accessability of a block and its value,
 * possibly also add it to the open-blocks pqueue.
 */
bool CPathEstimator::TestBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& peDef,
	const PathNode* parentOpenBlock,
	const CSolidObject* owner,
	const unsigned int pathDir,
	const unsigned int /*blockStatus*/,
	float maxSpeedMod
) {
	testedBlocks++;

	// step from parent to child block (e.g. PATHDIR_LEFT_TO_RIGHT=<+1,0>)
	const int2 openBlockPos = parentOpenBlock->nodePos;
	const int2 testBlockPos = openBlockPos + PE_DIRECTION_VECTORS[pathDir];
	const int2 goalBlockPos = {int(peDef.goalSquareX / BLOCK_SIZE), int(peDef.goalSquareZ / BLOCK_SIZE)};

	// bounds-check
	if (static_cast<unsigned int>(testBlockPos.x) >= nbrOfBlocks.x)
		return false;
	if (static_cast<unsigned int>(testBlockPos.y) >= nbrOfBlocks.y)
		return false;

	// read precached vertex costs
	const unsigned int openBlockIdx = BlockPosToIdx(openBlockPos);
	const unsigned int testBlockIdx = BlockPosToIdx(testBlockPos);

	// check if the block is unavailable
	if (blockStates.nodeMask[testBlockIdx] & (PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return false;

	const unsigned int vertexBaseIdx = moveDef.pathType * nbrOfBlocks.x * nbrOfBlocks.y * PATH_DIRECTION_VERTICES;
	const unsigned int vertexCostIdx =
		vertexBaseIdx +
		openBlockIdx * PATH_DIRECTION_VERTICES +
		GetBlockVertexOffset(pathDir, nbrOfBlocks.x);

	assert(testBlockIdx < blockStates.peNodeOffsets[moveDef.pathType].size());
	assert(vertexCostIdx < vertexCosts.size());

	// best accessible heightmap-coordinate within tested block
	// [DBG] const int2 openBlockSquare = blockStates.peNodeOffsets[moveDef.pathType][openBlockIdx];
	const int2 testBlockSquare = blockStates.peNodeOffsets[moveDef.pathType][testBlockIdx];

	// transition-cost from parent to tested child
	float testVertexCost = vertexCosts[vertexCostIdx];


	// inf-cost means we can not get from the parent VERTEX to the child
	// but the latter might still be reachable from peDef.wsStartPos (if
	// it is one of the first 8 expanded)
	// regular edges within the base-set are only valid to expand iff end
	// is reachable from wsStartPos, which can disagree with reachability
	// from openBlockSquare
	const bool infCostVertex = (testVertexCost >= PATHCOST_INFINITY);
	const bool baseSetVertex = (testedBlocks <= 8);
	const bool blockedSearch = (!baseSetVertex || peDef.skipSubSearches);

	if (infCostVertex) {
		// warning: we cannot naively set PATHOPT_BLOCKED here;
		// vertexCosts[] depends on the direction and nodeMask
		// does not
		// we would have to save the direction via PATHOPT_LEFT
		// etc. in the nodeMask but that is complicated and not
		// worth it: would just save the vertexCosts[] lookup
		//
		// blockStates.nodeMask[testBlockIdx] |= (PathDir2PathOpt(pathDir) | PATHOPT_BLOCKED);
		// dirtyBlocks.push_back(testBlockIdx);
		if (blockedSearch || DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare)) != IPath::Ok)
			return false;

		testVertexCost = peDef.Heuristic(testBlockSquare.x, testBlockSquare.y, peDef.startSquareX, peDef.startSquareZ, BLOCK_SIZE);
	} else {
		if (!blockedSearch && DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare)) != IPath::Ok)
			return false;
	}


	// check if the block is outside constraints
	if (!peDef.WithinConstraints(testBlockSquare)) {
		blockStates.nodeMask[testBlockIdx] |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(testBlockIdx);
		return false;
	}

	if (!peDef.skipSubSearches) {
		#if 0
		if (infCostVertex && baseSetVertex && DoBlockSearch(owner, moveDef, peDef.wsStartPos, SquareToFloat3(testBlockSquare.x, testBlockSquare.y)) != IPath::Ok)
			return false;
		#endif

		if (testBlockPos == goalBlockPos) {
			// must skip goal sub-searches during CalcVertexPathCosts
			//
			// if we have expanded the goal-block, check if a valid
			// max-resolution path exists (from where we entered it
			// to the actual goal position) since there might still
			// be impassable terrain in between
			//
			// const float3 gWorldPos = {testBlockPos.x * BLOCK_PIXEL_SIZE * 1.0f, 0.0f, testBlockPos.y * BLOCK_PIXEL_SIZE * 1.0f};
			// const float3 sWorldPos = {openBlockPos.x * BLOCK_PIXEL_SIZE * 1.0f, 0.0f, openBlockPos.y * BLOCK_PIXEL_SIZE * 1.0f};
			const float3 sWorldPos = SquareToFloat3(testBlockSquare.x, testBlockSquare.y);
			const float3 gWorldPos = peDef.wsGoalPos;

			if (sWorldPos.SqDistance2D(gWorldPos) > peDef.sqGoalRadius) {
				if (DoBlockSearch(owner, moveDef, sWorldPos, gWorldPos) != IPath::Ok) {
					// we cannot set PATHOPT_BLOCKED here either, result
					// depends on direction of entry from the parent node
					//
					// blockStates.nodeMask[testBlockIdx] |= PATHOPT_BLOCKED;
					// dirtyBlocks.push_back(testBlockIdx);
					return false;
				}
			}
		}
	}


	// evaluate this node (NOTE the max-resolution indexing for {flow,extra}Cost)
	//
	// nodeCost incorporates speed-modifiers, but the heuristic estimate does not
	// this causes an overestimation (and hence sub-optimal paths) if the average
	// modifier is greater than 1, which can only be fixed by choosing a constant
	// multiplier smaller than 1
	// however, since that would increase the number of explored nodes on "normal"
	// maps where the average is ~1, it is better to divide hCost by the (initial)
	// maximum modifier value
	//
	// const float  flowCost = (peDef.testMobile) ? (PathFlowMap::GetInstance())->GetFlowCost(testBlockSquare.x, testBlockSquare.y, moveDef, PathDir2PathOpt(pathDir)) : 0.0f;
	const float extraCost = blockStates.GetNodeExtraCost(testBlockSquare.x, testBlockSquare.y, peDef.synced);
	const float  nodeCost = testVertexCost + extraCost;

	const float gCost = parentOpenBlock->gCost + nodeCost;
	const float hCost = peDef.Heuristic(testBlockSquare.x, testBlockSquare.y, BLOCK_SIZE) * maxSpeedMod;
	const float fCost = gCost + hCost;

	// already in the open set?
	if (blockStates.nodeMask[testBlockIdx] & PATHOPT_OPEN) {
		// check if new found path is better or worse than the old one
		if (blockStates.fCost[testBlockIdx] <= fCost)
			return true;

		// no, clear old path data
		blockStates.nodeMask[testBlockIdx] &= ~PATHOPT_CARDINALS;
	}

	// look for improvements
	if (hCost < mGoalHeuristic) {
		mGoalBlockIdx = testBlockIdx;
		mGoalHeuristic = hCost;
	}

	// store this block as open
	openBlockBuffer.SetSize(openBlockBuffer.GetSize() + 1);
	assert(openBlockBuffer.GetSize() < MAX_SEARCHED_NODES_PE);

	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = fCost;
		ob->gCost   = gCost;
		ob->nodePos = testBlockPos;
		ob->nodeNum = testBlockIdx;
	openBlocks.push(ob);

	blockStates.SetMaxCost(NODE_COST_F, std::max(blockStates.GetMaxCost(NODE_COST_F), fCost));
	blockStates.SetMaxCost(NODE_COST_G, std::max(blockStates.GetMaxCost(NODE_COST_G), gCost));

	// mark this block as open
	blockStates.fCost[testBlockIdx] = fCost;
	blockStates.gCost[testBlockIdx] = gCost;
	blockStates.nodeMask[testBlockIdx] |= (PathDir2PathOpt(pathDir) | PATHOPT_OPEN);

	dirtyBlocks.push_back(testBlockIdx);
	return true;
}


/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& foundPath) const
{
	if (pfDef.needPath) {
		unsigned int blockIdx = mGoalBlockIdx;
		unsigned int numNodes = 0;

		{
			#if 1
			while (blockIdx != mStartBlockIdx) {
				const unsigned int pathOpt = blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS;
				const unsigned int pathDir = PathOpt2PathDir(pathOpt);

				blockIdx  = BlockPosToIdx(BlockIdxToPos(blockIdx) - PE_DIRECTION_VECTORS[pathDir]);
				numNodes += 1;
			}

			// PE's do not need the squares
			// foundPath.squares.reserve(numNodes);
			foundPath.path.reserve(numNodes);

			// reset
			blockIdx = mGoalBlockIdx;

			#else

			// more wasteful, but slightly faster for very long paths
			// foundPath.squares.reserve(1024 / BLOCK_SIZE);
			foundPath.path.reserve(1024 / BLOCK_SIZE);
			#endif
		}

		while (true) {
			// use offset defined by the block
			const int2 square = blockStates.peNodeOffsets[moveDef.pathType][blockIdx];

			// foundPath.squares.push_back(square);
			foundPath.path.emplace_back(square.x * SQUARE_SIZE, CMoveMath::yLevel(moveDef, square.x, square.y), square.y * SQUARE_SIZE);

			if (blockIdx == mStartBlockIdx)
				break;

			// next step backwards
			const unsigned int pathOpt = blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS;
			const unsigned int pathDir = PathOpt2PathDir(pathOpt);

			blockIdx = BlockPosToIdx(BlockIdxToPos(blockIdx) - PE_DIRECTION_VECTORS[pathDir]);
		}

		if (!foundPath.path.empty())
			foundPath.pathGoal = foundPath.path[0];
	}

	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx] - mGoalHeuristic;
}


bool CPathEstimator::RemoveCacheFile(const std::string& peFileName, const std::string& mapFileName)
{
	return (FileSystem::Remove(GetCacheFileName(IntToString(fileHashCode, "%x"), peFileName, mapFileName)));
}

/**
 * Try to read offset and vertices data from file, return false on failure
 */
bool CPathEstimator::ReadFile(const std::string& peFileName, const std::string& mapFileName)
{
	const std::string hashHexString = IntToString(fileHashCode, "%x");
	const std::string cacheFileName = GetCacheFileName(hashHexString, peFileName, mapFileName);

	LOG("[PathEstimator::%s] hash=%s file=\"%s\" (exists=%d)", __func__, hashHexString.c_str(), cacheFileName.c_str(), FileSystem::FileExists(cacheFileName));

	if (!FileSystem::FileExists(cacheFileName))
		return false;

	std::unique_ptr<IArchive> upfile(archiveLoader.OpenArchive(dataDirsAccess.LocateFile(cacheFileName), "sdz"));

	if (upfile == nullptr || !upfile->IsOpen()) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	char calcMsg[512];
	sprintf(calcMsg, "Reading Estimate PathCosts [%d]", BLOCK_SIZE);
	loadscreen->SetLoadMessage(calcMsg);

	const unsigned fid = upfile->FindFile("pathinfo");
	if (fid >= upfile->NumFiles()) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	std::vector<std::uint8_t> buffer;

	if (!upfile->GetFile(fid, buffer) || buffer.size() < 4) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	const unsigned int filehash = *(reinterpret_cast<unsigned int*>(&buffer[0]));
	const unsigned int blockSize = blockStates.GetSize() * sizeof(short2);
	unsigned int pos = sizeof(unsigned);

	if (filehash != fileHashCode) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	if (buffer.size() < (pos + blockSize * moveDefHandler.GetNumMoveDefs())) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	// read center-offset data
	for (int pathType = 0; pathType < moveDefHandler.GetNumMoveDefs(); ++pathType) {
		std::memcpy(&blockStates.peNodeOffsets[pathType][0], &buffer[pos], blockSize);
		pos += blockSize;
	}

	// read vertex-cost data
	if (buffer.size() < (pos + vertexCosts.size() * sizeof(float))) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	std::memcpy(&vertexCosts[0], &buffer[pos], vertexCosts.size() * sizeof(float));
	return true;
}


/**
 * Try to write offset and vertex data to file.
 */
bool CPathEstimator::WriteFile(const std::string& peFileName, const std::string& mapFileName)
{
	// we need this directory to exist
	if (!FileSystem::CreateDirectory(GetPathCacheDir()))
		return false;

	const std::string hashHexString = IntToString(fileHashCode, "%x");
	const std::string cacheFileName = GetCacheFileName(hashHexString, peFileName, mapFileName);

	LOG("[PathEstimator::%s] hash=%s file=\"%s\" (exists=%d)", __func__, hashHexString.c_str(), cacheFileName.c_str(), FileSystem::FileExists(cacheFileName));

	// open file for writing in a suitable location
	zipFile file = zipOpen(dataDirsAccess.LocateFile(cacheFileName, FileQueryFlags::WRITE).c_str(), APPEND_STATUS_CREATE);

	if (file == nullptr)
		return false;

	zipOpenNewFileInZip(file, "pathinfo", nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_BEST_COMPRESSION);

	// write hash-code (NOTE: this also affects the CRC!)
	zipWriteInFileInZip(file, (const void*) &fileHashCode, 4);

	// write center-offsets
	for (int pathType = 0; pathType < moveDefHandler.GetNumMoveDefs(); ++pathType) {
		zipWriteInFileInZip(file, (const void*) &blockStates.peNodeOffsets[pathType][0], blockStates.peNodeOffsets[pathType].size() * sizeof(short2));
	}

	// write vertex-costs
	zipWriteInFileInZip(file, vertexCosts.data(), vertexCosts.size() * sizeof(float));

	zipCloseFileInZip(file);
	zipClose(file, nullptr);


	// get the CRC over the written path data
	std::unique_ptr<IArchive> upfile(archiveLoader.OpenArchive(dataDirsAccess.LocateFile(cacheFileName), "sdz"));

	if (upfile == nullptr || !upfile->IsOpen()) {
		FileSystem::Remove(cacheFileName);
		return false;
	}

	assert(upfile->FindFile("pathinfo") < upfile->NumFiles());
  return true;
}


std::uint32_t CPathEstimator::CalcChecksum() const
{
	std::uint32_t chksum = 0;
	std::uint64_t nbytes = vertexCosts.size() * sizeof(float);
	std::uint64_t offset = 0;

	#if (ENABLE_NETLOG_CHECKSUM == 1)
	std::array<char, 128 + sha512::SHA_LEN * 2 + 1> msgBuffer;

	sha512::hex_digest hexChars;
	sha512::raw_digest shaBytes;
	sha512::msg_vector rawBytes;
	#endif

	#if (ENABLE_NETLOG_CHECKSUM == 1)
	for (const auto& pathTypeOffsets: blockStates.peNodeOffsets) {
		nbytes += (pathTypeOffsets.size() * sizeof(short2));
	}

	rawBytes.clear();
	rawBytes.resize(nbytes);

	for (const auto& pathTypeOffsets: blockStates.peNodeOffsets) {
		nbytes = pathTypeOffsets.size() * sizeof(short2);
		offset += nbytes;

		std::memcpy(&rawBytes[offset - nbytes], pathTypeOffsets.data(), nbytes);
	}

	{
		nbytes = vertexCosts.size() * sizeof(float);
		offset += nbytes;

		std::memcpy(&rawBytes[offset - nbytes], vertexCosts.data(), nbytes);

		sha512::calc_digest(rawBytes, shaBytes); // hash(offsets|costs)
		sha512::dump_digest(shaBytes, hexChars); // hexify(hash)

		SNPRINTF(msgBuffer.data(), msgBuffer.size(), "[PE::%s][BLK_SIZE=%d][SHA_DATA=%s]", __func__, BLOCK_SIZE, hexChars.data());
		CLIENT_NETLOG(gu->myPlayerNum, LOG_LEVEL_INFO, msgBuffer.data());
	}
	#endif

	// make path-estimator checksum part of synced state s.t. when
	// a client has a corrupted or stale cache it desyncs from the
	// start, not minutes later
	for (size_t i = 0, n = shaBytes.size() / 4; i < n; i += 1) {
		const uint16_t hi = (shaBytes[i * 4 + 0] << 8) | (shaBytes[i * 4 + 1] << 0);
		const uint16_t lo = (shaBytes[i * 4 + 2] << 8) | (shaBytes[i * 4 + 3] << 0);

		const SyncedUint su = (hi << 16) | (lo << 0);

		// copy first four bytes to reduced checksum
		if (chksum == 0)
			chksum = su;
	}

	return chksum;
}


/**
 * Returns a hash-code identifying the dataset of this estimator.
 */
std::uint32_t CPathEstimator::CalcHash(const char* caller) const
{
	const unsigned int hmChecksum = readMap->CalcHeightmapChecksum();
	const unsigned int tmChecksum = readMap->CalcTypemapChecksum();
	const unsigned int mdChecksum = moveDefHandler.GetCheckSum();
	const unsigned int bmChecksum = groundBlockingObjectMap.CalcChecksum();
	const unsigned int peHashCode = (hmChecksum + tmChecksum + mdChecksum + bmChecksum + BLOCK_SIZE + PATHESTIMATOR_VERSION);

	LOG("[PathEstimator::%s][%s] BLOCK_SIZE=%u", __func__, caller, BLOCK_SIZE);
	LOG("[PathEstimator::%s][%s] PATHESTIMATOR_VERSION=%u", __func__, caller, PATHESTIMATOR_VERSION);
	LOG("[PathEstimator::%s][%s] heightMapChecksum=%x", __func__, caller, hmChecksum);
	LOG("[PathEstimator::%s][%s] typeMapChecksum=%x", __func__, caller, tmChecksum);
	LOG("[PathEstimator::%s][%s] moveDefChecksum=%x", __func__, caller, mdChecksum);
	LOG("[PathEstimator::%s][%s] blockMapChecksum=%x", __func__, caller, bmChecksum);
	LOG("[PathEstimator::%s][%s] estimatorHashCode=%x", __func__, caller, peHashCode);

	return peHashCode;
}
