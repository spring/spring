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



CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, const std::string& cacheFileName, const std::string& map):
	BLOCK_SIZE(BSIZE),
	BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
	BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
	pathFinder(pf),
	nbrOfBlocksX(gs->mapx / BLOCK_SIZE),
	nbrOfBlocksZ(gs->mapy / BLOCK_SIZE),
	pathChecksum(0),
	offsetBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	costBlockNum(nbrOfBlocksX * nbrOfBlocksZ),
	nextOffsetMessage(-1),
	nextCostMessage(-1),
	blockStates(int2(nbrOfBlocksX, nbrOfBlocksZ), int2(gs->mapx, gs->mapy))
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

	vertices.resize(moveinfo->moveData.size() * blockStates.GetSize() * PATH_DIRECTION_VERTICES, 0.0f);

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
	for (int i = 0; i < blockStates.GetSize(); i++)
		blockStates[i].nodeOffsets.clear();

	delete pathCache;

	blockStates.Clear();
	vertices.clear();
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
	InitVertices();
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

		blockStates[blockNr].nodeOffsets.resize(moveinfo->moveData.size());
	}
}


void CPathEstimator::CalcOffsetsAndPathCosts(int thread) {
	//! reset FPU state for synced computations
	streflop_init<streflop::Simple>();

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

	for (vector<MoveData*>::iterator mi = moveinfo->moveData.begin(); mi != moveinfo->moveData.end(); mi++) {
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
		net->Send(CBaseNetProtocol::Get().SendCPUUsage(0x1 | BLOCK_SIZE | (idx<<8)));
		loadscreen->SetLoadMessage(calcMsg, (idx != 0));
	}

	for (vector<MoveData*>::iterator mi = moveinfo->moveData.begin(); mi != moveinfo->moveData.end(); mi++) {
		if ((*mi)->unitDefRefCount > 0) {
			CalculateVertices(**mi, x, z, thread);
		}
	}
}






/**
 * Finds a square accessable by the given movedata within the given block
 */
void CPathEstimator::FindOffset(const MoveData& moveData, int blockX, int blockZ) {
	// lower corner position of block
	const int lowerX = blockX * BLOCK_SIZE;
	const int lowerZ = blockZ * BLOCK_SIZE;
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

			if (moveData.moveMath->IsBlocked(moveData, lowerX + x, lowerZ + z) & CMoveMath::BLOCK_STRUCTURE) {
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
void CPathEstimator::CalculateVertices(const MoveData& moveData, int blockX, int blockZ, int thread) {
	for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++)
		CalculateVertex(moveData, blockX, blockZ, dir, thread);
}


/**
 * Calculate requested vertex
 */
void CPathEstimator::CalculateVertex(const MoveData& moveData, int parentBlockX, int parentBlockZ, unsigned int direction, int thread) {
	// initial calculations
	const int parentBlocknr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	const int childBlockX = parentBlockX + directionVector[direction].x;
	const int childBlockZ = parentBlockZ + directionVector[direction].y;
	const int vertexNbr = moveData.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES + parentBlocknr * PATH_DIRECTION_VERTICES + direction;

	// outside map?
	if (childBlockX < 0 || childBlockZ < 0 ||
		childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertices[vertexNbr] = PATHCOST_INFINITY;
		return;
	}

	// start position
	const int parentXSquare = blockStates[parentBlocknr].nodeOffsets[moveData.pathType].x;
	const int parentZSquare = blockStates[parentBlocknr].nodeOffsets[moveData.pathType].y;
	const float3 startPos = SquareToFloat3(parentXSquare, parentZSquare);

	// goal position
	const int childBlocknr = childBlockZ * nbrOfBlocksX + childBlockX;
	const int childXSquare = blockStates[childBlocknr].nodeOffsets[moveData.pathType].x;
	const int childZSquare = blockStates[childBlocknr].nodeOffsets[moveData.pathType].y;
	const float3 goalPos = SquareToFloat3(childXSquare, childZSquare);

	// PathFinder definition
	CRangedGoalWithCircularConstraint pfDef(startPos, goalPos, 0, 1.1f, 2);

	// the path to find
	IPath::Path path;
	IPath::SearchResult result;

	// since CPathFinder::GetPath() is not thread-safe,
	// use this thread's "private" CPathFinder instance
	// (rather than locking pathFinder->GetPath()) if we
	// are in one
	result = pathFinders[thread]->GetPath(moveData, startPos, pfDef, path, false, true, MAX_SEARCHED_NODES_PE, false, 0, true);

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
	if (lowerX < 0) lowerX = 0;
	if (lowerZ < 0) lowerZ = 0;

	// mark the blocks inside the rectangle, enqueue them
	// from upper to lower because of the placement of the
	// bi-directional vertices
	for (int z = upperZ; z >= lowerZ; z--) {
		for (int x = upperX; x >= lowerX; x--) {
			if (!(blockStates[z * nbrOfBlocksX + x].nodeMask & PATHOPT_OBSOLETE)) {
				vector<MoveData*>::iterator mi;

				for (mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
					if ((*mi)->unitDefRefCount > 0) {
						SingleBlock sb;
							sb.block.x = x;
							sb.block.y = z;
							sb.moveData = *mi;
						needUpdate.push_back(sb);
						blockStates[z * nbrOfBlocksX + x].nodeMask |= PATHOPT_OBSOLETE;
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
	unsigned int counter = 0;

	while (!needUpdate.empty() && counter < BLOCKS_TO_UPDATE) {
		// next block in line
		SingleBlock sb = needUpdate.front();
		needUpdate.pop_front();

		const int blocknr = sb.block.y * nbrOfBlocksX + sb.block.x;

		// check if it's already updated
		if (!(blockStates[blocknr].nodeMask & PATHOPT_OBSOLETE))
			continue;

		// no, update the block
		FindOffset(*sb.moveData, sb.block.x, sb.block.y);
		CalculateVertices(*sb.moveData, sb.block.x, sb.block.y);

		// mark it as updated
		if (sb.moveData == moveinfo->moveData.back()) {
			blockStates[blocknr].nodeMask &= ~PATHOPT_OBSOLETE;
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

	startBlock.x = (int)(start.x / BLOCK_PIXEL_SIZE);
	startBlock.y = (int)(start.z / BLOCK_PIXEL_SIZE);
	startBlocknr = startBlock.y * nbrOfBlocksX + startBlock.x;
	int2 goalBlock;
	goalBlock.x = peDef.goalSquareX / BLOCK_SIZE;
	goalBlock.y = peDef.goalSquareZ / BLOCK_SIZE;

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

		if (synced && result == IPath::Ok) {
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
	// is starting square inside goal area?
	const int xSquare = blockStates[startBlocknr].nodeOffsets[moveData.pathType].x;
	const int zSquare = blockStates[startBlocknr].nodeOffsets[moveData.pathType].y;

	if (peDef.IsGoal(xSquare, zSquare))
		return IPath::CantGetCloser;

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockStates[startBlocknr].nodeMask |= PATHOPT_OPEN;
	blockStates[startBlocknr].fCost = 0.0f;
	blockStates[startBlocknr].gCost = 0.0f;
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
	goalHeuristic = peDef.Heuristic(xSquare, zSquare);

	// get the goal square offset
	goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	// perform the search
	IPath::SearchResult result = DoSearch(moveData, peDef, synced);

	// if no improvements are found, then return CantGetCloser instead
	if (goalBlock.x == startBlock.x && goalBlock.y == startBlock.y) {
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
		const int xBSquare = blockStates[ob->nodeNum].nodeOffsets[moveData.pathType].x;
		const int zBSquare = blockStates[ob->nodeNum].nodeOffsets[moveData.pathType].y;
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
	unsigned int direction,
	bool synced
) {
	testedBlocks++;

	// initial calculations of the new block
	int2 block;
		block.x = parentOpenBlock.nodePos.x + directionVector[direction].x;
		block.y = parentOpenBlock.nodePos.y + directionVector[direction].y;
	const int vertexIdx =
		moveData.pathType * blockStates.GetSize() * PATH_DIRECTION_VERTICES +
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
	if (blockStates[blockIdx].nodeMask & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	const int xSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].x;
	const int zSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].y;

	// check if the block is blocked or out of constraints
	if (!peDef.WithinConstraints(xSquare, zSquare)) {
		blockStates[blockIdx].nodeMask |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blockIdx);
		return;
	}

	// evaluate this node (NOTE the max-res. indexing for extraCost)
	const float extraCost = blockStates.GetNodeExtraCost(xSquare, zSquare, synced);
	const float nodeCost = vertices[vertexIdx] + extraCost;

	const float gCost = parentOpenBlock.gCost + nodeCost;  // g
	const float hCost = peDef.Heuristic(xSquare, zSquare); // h
	const float fCost = gCost + hCost;                     // f


	if (blockStates[blockIdx].nodeMask & PATHOPT_OPEN) {
		// already in the open set
		if (blockStates[blockIdx].fCost <= fCost)
			return;

		blockStates[blockIdx].nodeMask &= ~PATHOPT_DIRECTION;
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
	blockStates[blockIdx].fCost = fCost;
	blockStates[blockIdx].gCost = gCost;
	blockStates[blockIdx].nodeMask |= (direction | PATHOPT_OPEN);
	blockStates[blockIdx].parentNodePos = parentOpenBlock.nodePos;

	dirtyBlocks.push_back(blockIdx);
}


/**
 * Recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveData& moveData, IPath::Path& foundPath) {
	int2 block = goalBlock;

	while (block.x != startBlock.x || block.y != startBlock.y) {
		const int blockIdx = block.y * nbrOfBlocksX + block.x;

		{
			// use offset defined by the block
			const int xBSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].x;
			const int zBSquare = blockStates[blockIdx].nodeOffsets[moveData.pathType].y;
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
	foundPath.pathCost = blockStates[goalBlock.y * nbrOfBlocksX + goalBlock.x].fCost - goalHeuristic;
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
		zipWriteInFileInZip(file, &vertices[0], vertices.size() * sizeof(float));

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
