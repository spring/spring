/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Win/win32.h"
#include "lib/gml/gml.h" // FIXME: linux for some reason does not compile without this

#include "PathEstimator.h"

#include <fstream>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include <boost/version.hpp>

#include "minizip/zip.h"
#include "System/mmgr.h"

#include "PathAllocator.h"
#include "PathCache.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathLog.h"
#include "Map/ReadMap.h"
#include "Game/LoadScreen.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/FileSystem/IArchive.h"
#include "System/FileSystem/ArchiveLoader.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/FileQueryFlags.h"
#include "System/Config/ConfigHandler.h"
#include "System/NetProtocol.h"

CONFIG(int, MaxPathCostsMemoryFootPrint).defaultValue(512 * 1024 * 1024);

static const std::string PATH_CACHE_DIR = "cache/paths/";

static size_t GetNumThreads() {
	size_t numThreads = std::max(0, configHandler->GetInt("HardwareThreadCount"));

	if (numThreads == 0) {
		// auto-detect
		#if (BOOST_VERSION >= 103500)
		numThreads = boost::thread::hardware_concurrency();
		#elif defined(USE_GML)
		numThreads = gmlCPUCount();
		#else
		numThreads = 1;
		#endif
	}

	return numThreads;
}

#if !defined(USE_MMGR)
void* CPathEstimator::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathEstimator::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }
#endif



CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, const std::string& cacheFileName, const std::string& map):
	BLOCK_SIZE(BSIZE),
	BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
	BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
	nbrOfBlocksX(gs->mapx / BLOCK_SIZE),
	nbrOfBlocksZ(gs->mapy / BLOCK_SIZE),
	blockStates(int2(nbrOfBlocksX, nbrOfBlocksZ), int2(gs->mapx, gs->mapy)),
	pathFinder(pf),
	pathChecksum(0),
	offsetBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	costBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	nextOffsetMessage(-1),
	nextCostMessage(-1)
{
	// these give the changes in (x, z) coors
	// when moving one step in given direction
	directionVector[PATHDIR_LEFT      ].x =  1;
	directionVector[PATHDIR_LEFT      ].y =  0;
	directionVector[PATHDIR_LEFT_UP   ].x =  1;
	directionVector[PATHDIR_LEFT_UP   ].y =  1;
	directionVector[PATHDIR_UP        ].x =  0;
	directionVector[PATHDIR_UP        ].y =  1;
	directionVector[PATHDIR_RIGHT_UP  ].x = -1;
	directionVector[PATHDIR_RIGHT_UP  ].y =  1;
	directionVector[PATHDIR_RIGHT     ].x = -1;
	directionVector[PATHDIR_RIGHT     ].y =  0;
	directionVector[PATHDIR_RIGHT_DOWN].x = -1;
	directionVector[PATHDIR_RIGHT_DOWN].y = -1;
	directionVector[PATHDIR_DOWN      ].x =  0;
	directionVector[PATHDIR_DOWN      ].y = -1;
	directionVector[PATHDIR_LEFT_DOWN ].x =  1;
	directionVector[PATHDIR_LEFT_DOWN ].y = -1;

	goalSqrOffset.x = BLOCK_SIZE / 2;
	goalSqrOffset.y = BLOCK_SIZE / 2;

	vertices.resize(moveDefHandler->moveDefs.size() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, 0.0f);

	// load precalculated data if it exists
	InitEstimator(cacheFileName, map);

	// As all vertexes are bidirectional and have equal values
	// in both directions, only one value needs to be stored.
	// This vector helps getting the right vertex. (Needs to
	// be inited after pre-calculations.)
	directionVertex[PATHDIR_LEFT      ] = PATHDIR_LEFT;
	directionVertex[PATHDIR_LEFT_UP   ] = PATHDIR_LEFT_UP;
	directionVertex[PATHDIR_UP        ] = PATHDIR_UP;
	directionVertex[PATHDIR_RIGHT_UP  ] = PATHDIR_RIGHT_UP;
	directionVertex[PATHDIR_RIGHT     ] = int(PATHDIR_LEFT    ) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_RIGHT_DOWN] = int(PATHDIR_LEFT_UP ) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_DOWN      ] = int(PATHDIR_UP      ) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES);
	directionVertex[PATHDIR_LEFT_DOWN ] = int(PATHDIR_RIGHT_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) + PATH_DIRECTION_VERTICES;

	pathCache = new CPathCache(nbrOfBlocksX, nbrOfBlocksZ);
}

CPathEstimator::~CPathEstimator()
{
	delete pathCache;
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
	InitVertices();
	InitBlocks();

	if (!ReadFile(cacheFileName, map)) {
		// start extra threads if applicable, but always keep the total
		// memory-footprint made by CPathFinder instances within bounds
		const unsigned int minMemFootPrint = sizeof(CPathFinder) + pathFinder->GetMemFootPrint();
		const unsigned int maxMemFootPrint = configHandler->GetInt("MaxPathCostsMemoryFootPrint");
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
}


void CPathEstimator::InitVertices() {
	for (unsigned int i = 0; i < vertices.size(); i++)
		vertices[i] = PATHCOST_INFINITY;
}

void CPathEstimator::InitBlocks() {
	for (int idx = 0; idx < blockStates.GetSize(); idx++) {
		const int x = idx % nbrOfBlocksX;
		const int z = idx / nbrOfBlocksX;
		const int blockNr = z * nbrOfBlocksX + x;

		blockStates.peNodeOffsets[blockNr].resize(moveDefHandler->moveDefs.size());
	}
}


void CPathEstimator::CalcOffsetsAndPathCosts(int thread) {
	//! reset FPU state for synced computations
	streflop::streflop_init<streflop::Simple>();

	// NOTE: EstimatePathCosts() [B] is temporally dependent on CalculateBlockOffsets() [A],
	// A must be completely finished before B_i can be safely called. This means we cannot
	// let thread i execute (A_i, B_i), but instead have to split the work such that every
	// thread finishes its part of A before any starts B_i.
	const int nbr = blockStates.GetSize() - 1;
	int i;

	while ((i = --offsetBlockNum) >= 0)
		CalculateBlockOffsets(nbr - i, thread);

	pathBarrier->wait();

	while ((i = --costBlockNum) >= 0)
		EstimatePathCosts(nbr - i, thread);
}


void CPathEstimator::CalculateBlockOffsets(int idx, int thread)
{
	const int x = idx % nbrOfBlocksX;
	const int z = idx / nbrOfBlocksX;

	if (thread == 0 && idx >= nextOffsetMessage) {
		nextOffsetMessage = idx + blockStates.GetSize() / 16;
		net->Send(CBaseNetProtocol::Get().SendCPUUsage(BLOCK_SIZE | (idx << 8)));
	}

	for (vector<MoveDef*>::iterator mi = moveDefHandler->moveDefs.begin(); mi != moveDefHandler->moveDefs.end(); ++mi) {
		if ((*mi)->unitDefRefCount > 0) {
			FindOffset(**mi, x, z);
		}
	}
}

void CPathEstimator::EstimatePathCosts(int idx, int thread) {
	const int x = idx % nbrOfBlocksX;
	const int z = idx / nbrOfBlocksX;

	if (thread == 0 && idx >= nextCostMessage) {
		nextCostMessage = idx + blockStates.GetSize() / 16;
		char calcMsg[128];
		sprintf(calcMsg, "PathCosts: precached %d of %d", idx, blockStates.GetSize());
		net->Send(CBaseNetProtocol::Get().SendCPUUsage(0x1 | BLOCK_SIZE | (idx << 8)));
		loadscreen->SetLoadMessage(calcMsg, (idx != 0));
	}

	for (vector<MoveDef*>::iterator mi = moveDefHandler->moveDefs.begin(); mi != moveDefHandler->moveDefs.end(); ++mi) {
		if ((*mi)->unitDefRefCount > 0) {
			CalculateVertices(**mi, x, z, thread);
		}
	}
}






/**
 * Finds a square accessable by the given MoveDef within the given block
 */
void CPathEstimator::FindOffset(const MoveDef& moveDef, int blockX, int blockZ) {
	//! lower corner position of block
	const int lowerX = blockX * BLOCK_SIZE;
	const int lowerZ = blockZ * BLOCK_SIZE;
	const unsigned int blockArea = (BLOCK_SIZE * BLOCK_SIZE) / SQUARE_SIZE;

	unsigned int bestPosX = BLOCK_SIZE >> 1;
	unsigned int bestPosZ = BLOCK_SIZE >> 1;

	float bestCost = std::numeric_limits<float>::max();
	const CMoveMath& moveMath = *(moveDef.moveMath);

	float speedMod = moveMath.GetPosSpeedMod(moveDef, lowerX, lowerZ);
	bool curblock = (speedMod == 0.0f) || moveMath.IsBlockedStructure(moveDef, lowerX, lowerZ);
	// search for an accessible position
	unsigned int z = 0;
	while (true) {
		bool zcurblock = curblock;
		unsigned int x = 0;
		while (true) {
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
			if (++x >= BLOCK_SIZE)
				break;
			// if last position was not blocked, then we do not need to check the entire square
			speedMod = moveMath.GetPosSpeedMod(moveDef, lowerX + x, lowerZ + z);
			curblock = (speedMod == 0.0f) || (curblock ? moveMath.IsBlockedStructure(moveDef, lowerX + x, lowerZ + z) :
						moveMath.IsBlockedStructureXmax(moveDef, lowerX + x, lowerZ + z));
		}
		if (++z >= BLOCK_SIZE)
			break;
		speedMod = moveMath.GetPosSpeedMod(moveDef, lowerX, lowerZ + z);
		curblock = (speedMod == 0.0f) || (zcurblock ? moveMath.IsBlockedStructure(moveDef, lowerX, lowerZ + z) :
						moveMath.IsBlockedStructureZmax(moveDef, lowerX, lowerZ + z));
	}

	// store the offset found
	blockStates.peNodeOffsets[blockZ * nbrOfBlocksX + blockX][moveDef.pathType] = int2(blockX * BLOCK_SIZE + bestPosX, blockZ * BLOCK_SIZE + bestPosZ);
}


/**
 * Calculate all vertices connected from the given block
 * (always 4 out of 8 vertices connected to the block)
 */
void CPathEstimator::CalculateVertices(const MoveDef& moveDef, int blockX, int blockZ, int thread) {
	for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++)
		CalculateVertex(moveDef, blockX, blockZ, dir, thread);
}


/**
 * Calculate requested vertex
 */
void CPathEstimator::CalculateVertex(const MoveDef& moveDef, int parentBlockX, int parentBlockZ, unsigned int direction, int thread) {
	// initial calculations
	const int parentBlocknr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	const int childBlockX = parentBlockX + directionVector[direction].x;
	const int childBlockZ = parentBlockZ + directionVector[direction].y;
	const int vertexNbr = moveDef.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES + parentBlocknr * PATH_DIRECTION_VERTICES + direction;

	// outside map?
	if (childBlockX < 0 || childBlockZ < 0 ||
		childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertices[vertexNbr] = PATHCOST_INFINITY;
		return;
	}

	// start position
	const int2 parentSquare = blockStates.peNodeOffsets[parentBlocknr][moveDef.pathType];
	const float3 startPos = SquareToFloat3(parentSquare.x, parentSquare.y);

	// goal position
	const int childBlocknr = childBlockZ * nbrOfBlocksX + childBlockX;
	const int2 childSquare = blockStates.peNodeOffsets[childBlocknr][moveDef.pathType];
	const float3 goalPos = SquareToFloat3(childSquare.x, childSquare.y);

	// PathFinder definition
	CRangedGoalWithCircularConstraint pfDef(startPos, goalPos, 0, 1.1f, 2);

	// the path to find
	IPath::Path path;
	IPath::SearchResult result;

	// since CPathFinder::GetPath() is not thread-safe,
	// use this thread's "private" CPathFinder instance
	// (rather than locking pathFinder->GetPath()) if we
	// are in one
	result = pathFinders[thread]->GetPath(moveDef, startPos, pfDef, path, false, true, MAX_SEARCHED_NODES_PF >> 2, false, 0, true);

	// store the result
	if (result == IPath::Ok)
		vertices[vertexNbr] = path.pathCost;
	else
		vertices[vertexNbr] = PATHCOST_INFINITY;
}


/**
 * Mark affected blocks as obsolete
 */
void CPathEstimator::MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned z2) {
	// find the upper and lower corner of the rectangular area
	int lowerX, upperX, lowerZ, upperZ;

	if (x1 < x2) {
		lowerX = x1 / BLOCK_SIZE - 1;
		upperX = x2 / BLOCK_SIZE;
	} else {
		lowerX = x2 / BLOCK_SIZE - 1;
		upperX = x1 / BLOCK_SIZE;
	}
	if (z1 < z2) {
		lowerZ = z1 / BLOCK_SIZE - 1;
		upperZ = z2 / BLOCK_SIZE;
	} else {
		lowerZ = z2 / BLOCK_SIZE - 1;
		upperZ = z1 / BLOCK_SIZE;
	}

	// error-check
	upperX = std::min(upperX, nbrOfBlocksX - 1);
	upperZ = std::min(upperZ, nbrOfBlocksZ - 1);
	lowerX = std::max(0, lowerX);
	lowerZ = std::max(0, lowerZ);

	// mark the blocks inside the rectangle, enqueue them
	// from upper to lower because of the placement of the
	// bi-directional vertices
	for (int z = upperZ; z >= lowerZ; z--) {
		for (int x = upperX; x >= lowerX; x--) {
			if (!(blockStates.nodeMask[z * nbrOfBlocksX + x] & PATHOPT_OBSOLETE)) {
				vector<MoveDef*>::iterator mi;

				for (mi = moveDefHandler->moveDefs.begin(); mi < moveDefHandler->moveDefs.end(); ++mi) {
					if ((*mi)->unitDefRefCount > 0) {
						SingleBlock sb;
							sb.block.x = x;
							sb.block.y = z;
							sb.moveDef = *mi;
						needUpdate.push_back(sb);
						blockStates.nodeMask[z * nbrOfBlocksX + x] |= PATHOPT_OBSOLETE;
					}
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

	for (unsigned int n = 0; !needUpdate.empty() && n < BLOCKS_TO_UPDATE; ) {
		// copy the next block in line
		const SingleBlock sb = needUpdate.front();

		const unsigned int blockX = sb.block.x;
		const unsigned int blockZ = sb.block.y;
		const unsigned int blockN = blockZ * nbrOfBlocksX + blockX;

		needUpdate.pop_front();

		// check if it's not already updated
		if (blockStates.nodeMask[blockN] & PATHOPT_OBSOLETE) {
			const MoveDef* currBlockMD = sb.moveDef;
			const MoveDef* nextBlockMD = (needUpdate.empty())? NULL: (needUpdate.front()).moveDef;

			// no, update the block
			FindOffset(*currBlockMD, blockX, blockZ);
			CalculateVertices(*currBlockMD, blockX, blockZ);

			// each MapChanged() call adds AT MOST <moveDefs.size()> SingleBlock's
			// in ascending pathType order per (x, z) PE-block, therefore when the
			// next SingleBlock's pathType is less or equal to the current we know
			// that all have been processed (for one PE-block)
			if (nextBlockMD == NULL || nextBlockMD->pathType <= currBlockMD->pathType) {
				blockStates.nodeMask[blockN] &= ~PATHOPT_OBSOLETE;
			}

			// one stale SingleBlock consumed
			n++;
		}
	}
}


/**
 * Stores data and does some top-administration
 */
IPath::SearchResult CPathEstimator::GetPath(
	const MoveDef& moveDef,
	float3 start,
	const CPathFinderDef& peDef,
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

	startBlock.x = (int)(start.x / BLOCK_PIXEL_SIZE);
	startBlock.y = (int)(start.z / BLOCK_PIXEL_SIZE);
	startBlocknr = startBlock.y * nbrOfBlocksX + startBlock.x;
	int2 goalBlock;
	goalBlock.x = peDef.goalSquareX / BLOCK_SIZE;
	goalBlock.y = peDef.goalSquareZ / BLOCK_SIZE;

	if (synced) {
		CPathCache::CacheItem* ci = pathCache->GetCachedPath(startBlock, goalBlock, peDef.sqGoalRadius, moveDef.pathType);
		if (ci) {
			// use a cached path if we have one (NOTE: only when in synced context)
			path = ci->path;
			return ci->result;
		}
	}

	// oterhwise search
	IPath::SearchResult result = InitSearch(moveDef, peDef, synced);

	// if search successful, generate new path
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, path);

		if (synced && result == IPath::Ok) {
			// add succesful paths to the cache (NOTE: only when in synced context)
			pathCache->AddPath(&path, result, startBlock, goalBlock, peDef.sqGoalRadius, moveDef.pathType);
		}

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "PE: Search completed.");
			LOG_L(L_DEBUG, "Tested blocks: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open blocks: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "Path length: "_STPF_, path.path.size());
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
	// is starting square inside goal area?
	const int2 square = blockStates.peNodeOffsets[startBlocknr][moveDef.pathType];

	const bool isStartGoal = peDef.IsGoal(square.x, square.y);
	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && peDef.startInGoalRadius)
		return IPath::CantGetCloser;

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockStates.nodeMask[startBlocknr] |= PATHOPT_OPEN;
	blockStates.fCost[startBlocknr] = 0.0f;
	blockStates.gCost[startBlocknr] = 0.0f;
	blockStates.SetMaxFCost(0.0f);
	blockStates.SetMaxGCost(0.0f);

	dirtyBlocks.push_back(startBlocknr);

	openBlockBuffer.SetSize(0);
	// add the starting block to the open-blocks-queue
	PathNode* ob = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		ob->fCost   = 0.0f;
		ob->gCost   = 0.0f;
		ob->nodePos = startBlock;
		ob->nodeNum = startBlocknr;
	openBlocks.push(ob);

	// mark starting point as best found position
	goalBlock = startBlock;
	goalHeuristic = peDef.Heuristic(square.x, square.y);

	// get the goal square offset
	goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, peDef, synced);

	// if no improvements are found, then return CantGetCloser instead
	if (goalBlock.x == startBlock.x && goalBlock.y == startBlock.y && (!isStartGoal || peDef.startInGoalRadius)) {
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
		const int xBSquare = blockStates.peNodeOffsets[ob->nodeNum][moveDef.pathType].x;
		const int zBSquare = blockStates.peNodeOffsets[ob->nodeNum][moveDef.pathType].y;
		const int xGSquare = ob->nodePos.x * BLOCK_SIZE + goalSqrOffset.x;
		const int zGSquare = ob->nodePos.y * BLOCK_SIZE + goalSqrOffset.y;

		if (peDef.IsGoal(xBSquare, zBSquare) || peDef.IsGoal(xGSquare, zGSquare)) {
			goalBlock = ob->nodePos;
			goalHeuristic = 0;
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
	unsigned int direction,
	bool synced
) {
	testedBlocks++;

	// initial calculations of the new block
	int2 block;
		block.x = parentOpenBlock.nodePos.x + directionVector[direction].x;
		block.y = parentOpenBlock.nodePos.y + directionVector[direction].y;
	const int vertexIdx =
		moveDef.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
		parentOpenBlock.nodeNum * PATH_DIRECTION_VERTICES +
		directionVertex[direction];
	const int blockIdx = block.y * nbrOfBlocksX + block.x;

	if (block.x < 0 || block.x >= nbrOfBlocksX || block.y < 0 || block.y >= nbrOfBlocksZ) {
		// blocks should never be able to lie outside map to the infinite vertices at the edges
		return;
	}

	if (vertexIdx < 0 || (unsigned int)vertexIdx >= vertices.size())
		return;

	if (vertices[vertexIdx] >= PATHCOST_INFINITY)
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

	// evaluate this node (NOTE the max-res. indexing for extraCost)
	const float extraCost = blockStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = vertices[vertexIdx] + extraCost;

	const float gCost = parentOpenBlock.gCost + nodeCost;    // g
	const float hCost = peDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f


	if (blockStates.nodeMask[blockIdx] & PATHOPT_OPEN) {
		// already in the open set
		if (blockStates.fCost[blockIdx] <= fCost)
			return;

		blockStates.nodeMask[blockIdx] &= ~PATHOPT_DIRECTION;
	}

	// look for improvements
	if (hCost < goalHeuristic) {
		goalBlock = block;
		goalHeuristic = hCost;
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
	blockStates.fCost[blockIdx] = fCost;
	blockStates.gCost[blockIdx] = gCost;
	blockStates.nodeMask[blockIdx] |= (direction | PATHOPT_OPEN);
	blockStates.peParentNodePos[blockIdx] = parentOpenBlock.nodePos;

	dirtyBlocks.push_back(blockIdx);
}


/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveDef& moveDef, IPath::Path& foundPath) {
	int2 block = goalBlock;

	while (block.x != startBlock.x || block.y != startBlock.y) {
		const int blockIdx = block.y * nbrOfBlocksX + block.x;

		{
			// use offset defined by the block
			const int xBSquare = blockStates.peNodeOffsets[blockIdx][moveDef.pathType].x;
			const int zBSquare = blockStates.peNodeOffsets[blockIdx][moveDef.pathType].y;
			const float3& pos = SquareToFloat3(xBSquare, zBSquare);

			foundPath.path.push_back(pos);
		}

		// next step backwards
		block = blockStates.peParentNodePos[blockIdx];
	}

	if (!foundPath.path.empty()) {
		foundPath.pathGoal = foundPath.path.front();
	}

	// set some additional information
	foundPath.pathCost = blockStates.fCost[goalBlock.y * nbrOfBlocksX + goalBlock.x] - goalHeuristic;
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
	char hashString[50];
	sprintf(hashString, "%u", hash);

	std::string filename = std::string(PATH_CACHE_DIR) + map + hashString + "." + cacheFileName + ".zip";
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
		const unsigned blockSize = moveDefHandler->moveDefs.size() * sizeof(int2);
		if (buffer.size() < pos + blockSize * blockStates.GetSize())
			return false;

		for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++) {
			std::memcpy(&blockStates.peNodeOffsets[blocknr][0], &buffer[pos], blockSize);
			pos += blockSize;
		}

		// Read vertices data.
		if (buffer.size() < pos + vertices.size() * sizeof(float))
			return false;
		std::memcpy(&vertices[0], &buffer[pos], vertices.size() * sizeof(float));

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
	if (!FileSystem::CreateDirectory(PATH_CACHE_DIR))
		return;

	const unsigned int hash = Hash();
	char hashString[64] = {0};

	sprintf(hashString, "%u", hash);

	const std::string filename = std::string(PATH_CACHE_DIR) + map + hashString + "." + cacheFileName + ".zip";
	zipFile file;

	// open file for writing in a suitable location
	file = zipOpen(dataDirsAccess.LocateFile(filename, FileQueryFlags::WRITE).c_str(), APPEND_STATUS_CREATE);

	if (file) {
		zipOpenNewFileInZip(file, "pathinfo", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);

		// Write hash.
		zipWriteInFileInZip(file, (void*) &hash, 4);

		// Write block-center-offsets.
		for (int blocknr = 0; blocknr < blockStates.GetSize(); blocknr++)
			zipWriteInFileInZip(file, (void*) &blockStates.peNodeOffsets[blocknr][0], moveDefHandler->moveDefs.size() * sizeof(int2));

		// Write vertices.
		zipWriteInFileInZip(file, &vertices[0], vertices.size() * sizeof(float));

		zipCloseFileInZip(file);
		zipClose(file, NULL);


		// get the CRC over the written path data
		IArchive* pfile = archiveLoader.OpenArchive(dataDirsAccess.LocateFile(filename), "sdz");

		if (!pfile || !pfile->IsOpen()) {
			delete pfile;
			return;
		}

		std::auto_ptr<IArchive> auto_pfile(pfile);
		IArchive& file(*pfile);

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
	return (readmap->mapChecksum + moveDefHandler->GetCheckSum() + BLOCK_SIZE + PATHESTIMATOR_VERSION);
}
