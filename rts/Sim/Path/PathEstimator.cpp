#include "StdAfx.h"
#include <fstream>
#include <boost/bind.hpp>
#include <boost/version.hpp>
#include "lib/minizip/zip.h"
#include "mmgr.h"

#include "PathEstimator.h"

#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include "Platform/ConfigHandler.h"

#include "Map/Ground.h"
#include "Game/SelectedUnits.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

#include "FileSystem/ArchiveZip.h"
#include "Platform/FileSystem.h"


#define PATHDEBUG false


const unsigned int PATHDIR_LEFT = 0;		// +x
const unsigned int PATHDIR_LEFT_UP = 1;		// +x+z
const unsigned int PATHDIR_UP = 2;			// +z
const unsigned int PATHDIR_RIGHT_UP = 3;	// -x+z
const unsigned int PATHDIR_RIGHT = 4;		// -x
const unsigned int PATHDIR_RIGHT_DOWN = 5;	// -x-z
const unsigned int PATHDIR_DOWN = 6;		// -z
const unsigned int PATHDIR_LEFT_DOWN = 7;	// +x-z

const unsigned int PATHOPT_OPEN = 8;
const unsigned int PATHOPT_CLOSED = 16;
const unsigned int PATHOPT_FORBIDDEN = 32;
const unsigned int PATHOPT_BLOCKED = 64;
const unsigned int PATHOPT_SEARCHRELATED = (PATHOPT_OPEN | PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED);
const unsigned int PATHOPT_OBSOLETE = 128;

const unsigned int PATHESTIMATOR_VERSION = 38;
const float PATHCOST_INFINITY = 10000000;
const int SQUARES_TO_UPDATE = 600;

extern std::string stupidGlobalMapname;


/*
 * constructor, loads precalculated data if it exists
 */
CPathEstimator::CPathEstimator(CPathFinder* pf, unsigned int BSIZE, unsigned int mmOpt, std::string name):
	pathFinder(pf),
	BLOCK_SIZE(BSIZE),
	BLOCK_PIXEL_SIZE(BSIZE * SQUARE_SIZE),
	BLOCKS_TO_UPDATE(SQUARES_TO_UPDATE / (BLOCK_SIZE * BLOCK_SIZE) + 1),
	moveMathOptions(mmOpt),
	pathChecksum(0)
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

	// create the block-map and the vertices-map
	nbrOfBlocksX = gs->mapx / BLOCK_SIZE;
	nbrOfBlocksZ = gs->mapy / BLOCK_SIZE;
	nbrOfBlocks = nbrOfBlocksX * nbrOfBlocksZ;
	blockState = SAFE_NEW BlockInfo[nbrOfBlocks];
	nbrOfVertices = moveinfo->moveData.size() * nbrOfBlocks * PATH_DIRECTION_VERTICES;
	vertex = SAFE_NEW float[nbrOfVertices];
	openBlockBufferPointer = openBlockBuffer;

	InitEstimator(name);

	// As all vertexes are bidirectional and have equal values
	// in both directions, only one value needs to be stored.
	// This vector helps getting the right vertex. (Needs to
	// be inited after pre-calculations.)
	directionVertex[PATHDIR_LEFT      ] = PATHDIR_LEFT;
	directionVertex[PATHDIR_LEFT_UP   ] = PATHDIR_LEFT_UP;
	directionVertex[PATHDIR_UP        ] = PATHDIR_UP;
	directionVertex[PATHDIR_RIGHT_UP  ] = PATHDIR_RIGHT_UP;
	directionVertex[PATHDIR_RIGHT     ] = int(PATHDIR_LEFT) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_RIGHT_DOWN] = int(PATHDIR_LEFT_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) - PATH_DIRECTION_VERTICES;
	directionVertex[PATHDIR_DOWN      ] = int(PATHDIR_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES);
	directionVertex[PATHDIR_LEFT_DOWN ] = int(PATHDIR_RIGHT_UP) - (nbrOfBlocksX * PATH_DIRECTION_VERTICES) + PATH_DIRECTION_VERTICES;

	pathCache = SAFE_NEW CPathCache(nbrOfBlocksX, nbrOfBlocksZ);
}


/*
 * free all used memory
 */
CPathEstimator::~CPathEstimator() {
	for (int i = 0; i < nbrOfBlocks; i++) {
		delete[] blockState[i].sqrCenter;
	}

	delete[] blockState;
	delete[] vertex;
	delete pathCache;
}



void CPathEstimator::SpawnThreads(int numThreads, int stage) {
	if (threads.size() != numThreads) {
		threads.resize(numThreads);
		pathFinders.resize(numThreads);
	}

	const int vertsPerThread = nbrOfVertices / numThreads;
	const int blocksPerThread = nbrOfBlocks / numThreads;

	for (int threadIdx = 0; threadIdx < numThreads; threadIdx++) {
		// if this is the last thread, we might have to do extra
		// work if numThreads did not evenly divide nbrOfVertices
		// or nbrOfBlocks, so calculate the remainder for both
		const bool isLastThread = (threadIdx == numThreads - 1);
		const int verticesRem = isLastThread? (nbrOfVertices - vertsPerThread * numThreads): 0;
		const int blocksRem = isLastThread? (nbrOfBlocks - blocksPerThread * numThreads): 0;

		const int minVertex = threadIdx * vertsPerThread;
		const int maxVertex = minVertex + vertsPerThread + verticesRem;
		const int minBlock = threadIdx * blocksPerThread;
		const int maxBlock = minBlock + blocksPerThread + blocksRem;

		if (stage == 0) {
			threads[threadIdx] = SAFE_NEW
				boost::thread(boost::bind(&CPathEstimator::InitVerticesAndBlocks, this, minVertex, maxVertex, minBlock, maxBlock));
		}
		if (stage == 1) {
			threads[threadIdx] = SAFE_NEW
				boost::thread(boost::bind(&CPathEstimator::CalculateBlockOffsets, this, minBlock, maxBlock, threadIdx));
		}
		if (stage == 2) {
			// allocate one private CPathFinder object per thread
			pathFinders[threadIdx] = SAFE_NEW CPathFinder();

			threads[threadIdx] = SAFE_NEW
				boost::thread(boost::bind(&CPathEstimator::EstimatePathCosts, this, minBlock, maxBlock, threadIdx));
		}
	}
}

void CPathEstimator::JoinThreads(int numThreads, int stage) {
	for (int threadIdx = 0; threadIdx < numThreads; threadIdx++) {
		threads[threadIdx]->join();
		delete threads[threadIdx];
		threads[threadIdx] = 0x0;

		if (stage == 2) {
			delete pathFinders[threadIdx];
			pathFinders[threadIdx] = 0x0;
		}
	}
}

void CPathEstimator::InitEstimator(const std::string& name) {
	int numThreads = configHandler.GetInt("HardwareThreadCount", 0);

#if 0	// FIXME mantis #1033
#if (BOOST_VERSION >= 103500)
	if (numThreads == 0)
		numThreads = boost::thread::hardware_concurrency();
#else
#  ifdef USE_GML
	if (numThreads == 0)
		numThreads = GML_CPU_COUNT;
#  endif
#endif
#else //if 0
	numThreads = 1;
#endif

	if (numThreads > 1) {
		// spawn the threads for InitVerticesAndBlocks()
		SpawnThreads(numThreads, 0);
		JoinThreads(numThreads, 0);

		char loadMsg[512];
		sprintf(loadMsg, "Reading estimate path costs (%d threads)", numThreads);
		PrintLoadMsg(loadMsg);

		if (!ReadFile(name)) {
			char calcMsg[512];
			sprintf(calcMsg, "Analyzing map accessibility [%d] (%d threads)", BLOCK_SIZE, numThreads);
			PrintLoadMsg(calcMsg);

			// re-spawn the threads for CalculateBlockOffsets()
			SpawnThreads(numThreads, 1);
			JoinThreads(numThreads, 1);
			// re-spawn the threads for EstimatePathCosts()
			SpawnThreads(numThreads, 2);
			JoinThreads(numThreads, 2);

			WriteFile(name);
		}
	} else {
		// no threading
		InitVerticesAndBlocks(0, nbrOfVertices,   0, nbrOfBlocks);

		PrintLoadMsg("Reading estimate path costs (1 thread)");

		if (!ReadFile(name)) {
			char calcMsg[512];
			sprintf(calcMsg, "Analyzing map accessibility [%d] (1 thread)", BLOCK_SIZE);
			PrintLoadMsg(calcMsg);

			CalcOffsetsAndPathCosts(0, nbrOfBlocks);

			WriteFile(name);
		}
	}
}



// wrapper
void CPathEstimator::InitVerticesAndBlocks(int minVertex, int maxVertex, int minBlock, int maxBlock) {
	InitVertices(minVertex, maxVertex);
	InitBlocks(minBlock, maxBlock);
}

void CPathEstimator::InitVertices(int minVertex, int maxVertex) {
	assert(maxVertex <= nbrOfVertices);

	for (int i = minVertex; i < maxVertex; i++)
		vertex[i] = PATHCOST_INFINITY;
}

void CPathEstimator::InitBlocks(int minBlock, int maxBlock) {
	assert(maxBlock <= nbrOfBlocks);

	for (int idx = minBlock; idx < maxBlock; idx++) {
		int x = idx % nbrOfBlocksX;
		int z = idx / nbrOfBlocksX;
		int blockNr = z * nbrOfBlocksX + x;

		blockState[blockNr].cost = PATHCOST_INFINITY;
		blockState[blockNr].options = 0;
		blockState[blockNr].parentBlock.x = -1;
		blockState[blockNr].parentBlock.y = -1;
		blockState[blockNr].sqrCenter = SAFE_NEW int2[moveinfo->moveData.size()];
	}
}



// wrapper
void CPathEstimator::CalcOffsetsAndPathCosts(int minBlock, int maxBlock, int threadID) {
	// NOTE: EstimatePathCosts() [B] is temporally dependent on CalculateBlockOffsets() [A],
	// A must be completely finished before B_i can be safely called. This means we cannot
	// let thread i execute (A_i, B_i), but instead have to split the work such that every
	// thread finishes its part of A before any starts B_i. Hence this wrapper function is
	// no longer called when more than one thread is spawned in InitEstimator().
	CalculateBlockOffsets(minBlock, maxBlock, threadID);
	EstimatePathCosts(minBlock, maxBlock, threadID);
}

void CPathEstimator::CalculateBlockOffsets(int minBlock, int maxBlock, int) {
	assert(maxBlock <= nbrOfBlocks);

	for (int idx = minBlock; idx < maxBlock; idx++) {
		int x = idx % nbrOfBlocksX;
		int z = idx / nbrOfBlocksX;

		vector<MoveData*>::iterator mi;
		for (mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
			FindOffset(**mi, x, z);
		}
	}
}

void CPathEstimator::EstimatePathCosts(int minBlock, int maxBlock, int threadID) {
	assert(maxBlock <= nbrOfBlocks);

	for (int move = 0; move < moveinfo->moveData.size(); move++) {
		MoveData* mdi = moveinfo->moveData[move];

		if (threadID == -1) {
			char calcMsg[512];
			sprintf(calcMsg, "Blocks %d to %d (block-size %d, path-type %d of %d)",
				minBlock, maxBlock, BLOCK_SIZE, mdi->pathType, moveinfo->moveData.size());
			PrintLoadMsg(calcMsg);
		} else {
			boost::mutex::scoped_lock lock(loadMsgMutex);

			// NOTE: locking PrintLoadMsg() is not enough, can't call it here
			logOutput.Print("Estimating path costs for blocks %d to %d (block-size %d, path-type %d of %d, thread-ID %d)",
				minBlock, maxBlock, BLOCK_SIZE, mdi->pathType, moveinfo->moveData.size(), threadID);
		}

		for (int idx = minBlock; idx < maxBlock; idx++) {
			int x = idx % nbrOfBlocksX;
			int z = idx / nbrOfBlocksX;
			CalculateVertices(*mdi, x, z, threadID);
		}
	}
}






/*
 * finds a square accessable by the given movedata within the given block
 */
void CPathEstimator::FindOffset(const MoveData& moveData, int blockX, int blockZ) {
	// lower corner position of block
	int lowerX = blockX * BLOCK_SIZE;
	int lowerZ = blockZ * BLOCK_SIZE;

	float best = 100000000.0f;
	int bestX = BLOCK_SIZE >> 1;
	int bestZ = BLOCK_SIZE >> 1;
	int num = (BLOCK_SIZE * BLOCK_SIZE) >> 3;

	// search for an accessible position
	for (int z = 1; z < BLOCK_SIZE; z += 2) {
		for (int x = 1; x < BLOCK_SIZE; x += 2) {
			int dx = x - (BLOCK_SIZE >> 1);
			int dz = z - (BLOCK_SIZE >> 1);
			float cost = (dx * dx + dz * dz) + num / (0.001f + moveData.moveMath->SpeedMod(moveData, lowerX + x, lowerZ + z));
			int mask = CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN;

			if (moveData.moveMath->IsBlocked2(moveData, lowerX + x, lowerZ + z, true) & mask) {
				cost += 1000000.0f;
			}
			if (cost < best) {
				best = cost;
				bestX = x;
				bestZ = z;
			}
		}
	}

	// store the offset found
	blockState[blockZ * nbrOfBlocksX + blockX].sqrCenter[moveData.pathType].x = blockX * BLOCK_SIZE + bestX;
	blockState[blockZ * nbrOfBlocksX + blockX].sqrCenter[moveData.pathType].y = blockZ * BLOCK_SIZE + bestZ;
}


/*
 * calculate all vertices connected from given block
 * (always 4 out of 8 vertices connected to the block)
 */
void CPathEstimator::CalculateVertices(const MoveData& moveData, int blockX, int blockZ, int threadID) {
	for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
		CalculateVertex(moveData, blockX, blockZ, dir, threadID);
	}
}


/*
 * calculate requested vertex
 */
void CPathEstimator::CalculateVertex(const MoveData& moveData, int parentBlockX, int parentBlockZ, unsigned int direction, int threadID) {
	// initial calculations
	int parentBlocknr = parentBlockZ * nbrOfBlocksX + parentBlockX;
	int childBlockX = parentBlockX + directionVector[direction].x;
	int childBlockZ = parentBlockZ + directionVector[direction].y;
	int vertexNbr = moveData.pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + parentBlocknr * PATH_DIRECTION_VERTICES + direction;

	// outside map?
	if (childBlockX < 0 || childBlockZ < 0 ||
		childBlockX >= nbrOfBlocksX || childBlockZ >= nbrOfBlocksZ) {
		vertex[vertexNbr] = PATHCOST_INFINITY;
		return;
	}

	// start position
	int parentXSquare = blockState[parentBlocknr].sqrCenter[moveData.pathType].x;
	int parentZSquare = blockState[parentBlocknr].sqrCenter[moveData.pathType].y;
	float3 startPos = SquareToFloat3(parentXSquare, parentZSquare);

	// goal position
	int childBlocknr = childBlockZ * nbrOfBlocksX + childBlockX;
	int childXSquare = blockState[childBlocknr].sqrCenter[moveData.pathType].x;
	int childZSquare = blockState[childBlocknr].sqrCenter[moveData.pathType].y;
	float3 goalPos = SquareToFloat3(childXSquare, childZSquare);

	// PathFinder definition
	CRangedGoalWithCircularConstraint pfDef(startPos, goalPos, 0, 1.1f, 2);

	// the path to find
	Path path;
	SearchResult result;

	if (threadID >= 0) {
		// since CPathFinder::GetPath() is not thread-safe,
		// use this thread's "private" CPathFinder instance
		// (rather than locking pathFinder->GetPath()) if we
		// are in one
		result = pathFinders[threadID]->GetPath(moveData, startPos, pfDef, path, false, true, 10000, false);
	} else {
		// otherwise use the pathFinder instance passed to
		// the constructor of this CPathEstimator object
		result = pathFinder->GetPath(moveData, startPos, pfDef, path, false, true, 10000, false);
	}

	// store the result
	if (result == Ok) {
		vertex[vertexNbr] = path.pathCost;
	} else {
		vertex[vertexNbr] = PATHCOST_INFINITY;
	}
}


/*
 * mark affected blocks as obsolete
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
			if (!(blockState[z * nbrOfBlocksX + x].options & PATHOPT_OBSOLETE)) {
				vector<MoveData*>::iterator mi;

				for (mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
					SingleBlock sb;
					sb.block.x = x;
					sb.block.y = z;
					sb.moveData = *mi;
					needUpdate.push_back(sb);
					blockState[z * nbrOfBlocksX + x].options |= PATHOPT_OBSOLETE;
				}
			}
		}
	}
}


/*
 * update some obsolete blocks using the FIFO-principle
 */
void CPathEstimator::Update() {
	pathCache->Update();
	int counter = 0;

	while (!needUpdate.empty() && counter < BLOCKS_TO_UPDATE) {
		// next block in line
		SingleBlock sb = needUpdate.front();
		needUpdate.pop_front();
		int blocknr = sb.block.y * nbrOfBlocksX + sb.block.x;

		// check if it's already updated
		if (!(blockState[blocknr].options & PATHOPT_OBSOLETE))
			continue;

		// no, update the block
		FindOffset(*sb.moveData, sb.block.x, sb.block.y);
		CalculateVertices(*sb.moveData, sb.block.x, sb.block.y);

		// mark it as updated
		if (sb.moveData == moveinfo->moveData.back()) {
			blockState[blocknr].options &= ~PATHOPT_OBSOLETE;
		}

		// one block updated
		counter++;
	}
}


/*
 * stores data and does some top-administration
 */
IPath::SearchResult CPathEstimator::GetPath(const MoveData& moveData, float3 start, const CPathFinderDef& peDef, Path& path, unsigned int maxSearchedBlocks) {
	start.CheckInBounds();
	// clear the path
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	// initial calculations
	maxBlocksToBeSearched = std::min(maxSearchedBlocks, (unsigned int) MAX_SEARCHED_BLOCKS);
	startBlock.x = (int)(start.x / BLOCK_PIXEL_SIZE);
	startBlock.y = (int)(start.z / BLOCK_PIXEL_SIZE);
	startBlocknr = startBlock.y * nbrOfBlocksX + startBlock.x;
	int2 goalBlock;
	goalBlock.x = peDef.goalSquareX / BLOCK_SIZE;
	goalBlock.y = peDef.goalSquareZ / BLOCK_SIZE;

	CPathCache::CacheItem* ci = pathCache->GetCachedPath(startBlock, goalBlock, peDef.sqGoalRadius, moveData.pathType);
	if (ci) {
		// use a cached path if we have one
		path = ci->path;
		return ci->result;
	}

	// oterhwise search
	SearchResult result = InitSearch(moveData, peDef);

	// if search successful, generate new path
	if (result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
		// only add succesful paths to the cache
		pathCache->AddPath(&path, result, startBlock, goalBlock, peDef.sqGoalRadius, moveData.pathType);

		if (PATHDEBUG) {
			logOutput << "PE: Search completed.\n";
			logOutput << "Tested blocks: " << testedBlocks << "\n";
			logOutput << "Open blocks: " << (float)(openBlockBufferPointer - openBlockBuffer) << "\n";
			logOutput << "Path length: " << (int)(path.path.size()) << "\n";
			logOutput << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if (PATHDEBUG) {
			logOutput << "PE: Search failed!\n";
			logOutput << "Tested blocks: " << testedBlocks << "\n";
			logOutput << "Open blocks: " << (float)(openBlockBufferPointer - openBlockBuffer) << "\n";
		}
	}

	return result;
}


/*
 * make some initial calculations and preparations
 */
IPath::SearchResult CPathEstimator::InitSearch(const MoveData& moveData, const CPathFinderDef& peDef) {
	// is starting square inside goal area?
	int xSquare = blockState[startBlocknr].sqrCenter[moveData.pathType].x;
	int zSquare = blockState[startBlocknr].sqrCenter[moveData.pathType].y;
	if (peDef.IsGoal(xSquare, zSquare))
		return CantGetCloser;

	// no, clean the system from last search
	ResetSearch();

	// mark and store the start-block
	blockState[startBlocknr].options |= PATHOPT_OPEN;
	blockState[startBlocknr].cost = 0;
	dirtyBlocks.push_back(startBlocknr);

	// add the starting block to the open-blocks-queue
	OpenBlock* ob = openBlockBufferPointer = openBlockBuffer;
	ob->cost = 0;
	ob->currentCost = 0;
	ob->block = startBlock;
	ob->blocknr = startBlocknr;
	openBlocks.push(ob);

	// mark starting point as best found position
	goalBlock = startBlock;
	goalHeuristic = peDef.Heuristic(xSquare, zSquare);

	// get the goal square offset
	goalSqrOffset = peDef.GoalSquareOffset(BLOCK_SIZE);

	// perform the search
	SearchResult result = DoSearch(moveData, peDef);

	// if no improvements are found, then return CantGetCloser instead
	if (goalBlock.x == startBlock.x && goalBlock.y == startBlock.y)
		return CantGetCloser;
	else
		return result;
}


/*
 * performs the actual search.
 */
IPath::SearchResult CPathEstimator::DoSearch(const MoveData& moveData, const CPathFinderDef& peDef) {
	bool foundGoal = false;
	while (!openBlocks.empty() && (openBlockBufferPointer - openBlockBuffer) < (maxBlocksToBeSearched - 8)) {
		// get the open block with lowest cost
		OpenBlock* ob = openBlocks.top();
		openBlocks.pop();

		// check if the block has been marked as unaccessible during its time in the queue
		if (blockState[ob->blocknr].options & (PATHOPT_BLOCKED | PATHOPT_CLOSED | PATHOPT_FORBIDDEN))
			continue;

		// no, check if the goal is already reached
		int xBSquare = blockState[ob->blocknr].sqrCenter[moveData.pathType].x;
		int zBSquare = blockState[ob->blocknr].sqrCenter[moveData.pathType].y;
		int xGSquare = ob->block.x * BLOCK_SIZE + goalSqrOffset.x;
		int zGSquare = ob->block.y * BLOCK_SIZE + goalSqrOffset.y;

		if (peDef.IsGoal(xBSquare, zBSquare) || peDef.IsGoal(xGSquare, zGSquare)) {
			goalBlock = ob->block;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		// no, test the 8 surrounding blocks
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_UP);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT);
		TestBlock(moveData, peDef, *ob, PATHDIR_RIGHT_DOWN);
		TestBlock(moveData, peDef, *ob, PATHDIR_DOWN);
		TestBlock(moveData, peDef, *ob, PATHDIR_LEFT_DOWN);

		// mark this block as closed
		blockState[ob->blocknr].options |= PATHOPT_CLOSED;
	}

	// we found our goal
	if (foundGoal)
		return Ok;

	// we could not reach the goal
	if (openBlockBufferPointer - openBlockBuffer >= (maxBlocksToBeSearched - 8))
		return GoalOutOfRange;

	// search could not reach the goal due to the unit being locked in
	if (openBlocks.empty())
		return GoalOutOfRange;

	// should never happen
	logOutput << "ERROR: CPathEstimator::DoSearch() - Unhandled end of search!\n";
	return Error;
}


/*
 * test the accessability of a block and its value,
 * possibly also add it to the open-blocks pqueue
 */
void CPathEstimator::TestBlock(const MoveData& moveData, const CPathFinderDef &peDef, OpenBlock& parentOpenBlock, unsigned int direction) {
	testedBlocks++;

	// initial calculations of the new block
	int2 block;
	block.x = parentOpenBlock.block.x + directionVector[direction].x;
	block.y = parentOpenBlock.block.y + directionVector[direction].y;
	int vertexNbr = moveData.pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + parentOpenBlock.blocknr * PATH_DIRECTION_VERTICES + directionVertex[direction];

	/*
	if (block.x < 0 || block.x >= nbrOfBlocksX || block.y < 0 || block.y >= nbrOfBlocksZ) {
		// blocks should never be able to lie outside map to the infinite vertices at the edges
		return;
	}
	*/

	if (vertexNbr < 0 || vertexNbr >= nbrOfVertices)
		return;

	int blocknr = block.y * nbrOfBlocksX + block.x;
	float blockCost = vertex[vertexNbr];
	if (blockCost >= PATHCOST_INFINITY)
		return;

	// check if the block is unavailable
	if (blockState[blocknr].options & (PATHOPT_FORBIDDEN | PATHOPT_BLOCKED | PATHOPT_CLOSED))
		return;

	int xSquare = blockState[blocknr].sqrCenter[moveData.pathType].x;
	int zSquare = blockState[blocknr].sqrCenter[moveData.pathType].y;

	// check if the block is blocked or out of constraints
	if (!peDef.WithinConstraints(xSquare, zSquare)) {
		blockState[blocknr].options |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(blocknr);
		return;
	}

	// evaluate this node
	float heuristicCost = peDef.Heuristic(xSquare, zSquare);
	float currentCost = parentOpenBlock.currentCost + blockCost;
	float cost = currentCost + heuristicCost;

	// check if the block is already in queue and keep it if it's better
	if (blockState[blocknr].options & PATHOPT_OPEN) {
		if (blockState[blocknr].cost <= cost)
			return;
		blockState[blocknr].options &= 255 - 7;
	}

	// look for improvements
	if (heuristicCost < goalHeuristic) {
		goalBlock = block;
		goalHeuristic = heuristicCost;
	}

	// store this block as open.
	OpenBlock* ob = ++openBlockBufferPointer;
	ob->block = block;
	ob->blocknr = blocknr;
	ob->cost = cost;
	ob->currentCost = currentCost;
	openBlocks.push(ob);

	// Mark the block as open, and its parent.
	blockState[blocknr].cost = cost;
	blockState[blocknr].options |= (direction | PATHOPT_OPEN);
	blockState[blocknr].parentBlock = parentOpenBlock.block;
	dirtyBlocks.push_back(blocknr);
}


/*
 * recreate the path taken to the goal
 */
void CPathEstimator::FinishSearch(const MoveData& moveData, Path& path) {
	int2 block = goalBlock;
	while (block.x != startBlock.x || block.y != startBlock.y) {
		int blocknr = block.y * nbrOfBlocksX + block.x;

		/*
		int xGSquare = block.x * BLOCK_SIZE + goalSqrOffset.x;
		int zGSquare = block.y * BLOCK_SIZE + goalSqrOffset.y;
		// in first case try to use an offset defined by goal...
		if (!moveData.moveMath->IsBlocced(moveData, moveMathOptions, xGSquare, zGSquare)) {
			float3 pos = SquareToFloat3(xGSquare, zGSquare);
			pos.y = moveData.moveMath->yLevel(xGSquare, zGSquare);
			path.path.push_back(pos);
		}
		else */{
			// ...if not possible, then use offset defined by the block
			int xBSquare = blockState[blocknr].sqrCenter[moveData.pathType].x;
			int zBSquare = blockState[blocknr].sqrCenter[moveData.pathType].y;
			float3 pos = SquareToFloat3(xBSquare, zBSquare);
			path.path.push_back(pos);
		}

		// next step backwards
		block = blockState[blocknr].parentBlock;
	}

	// set some additional information
	path.pathCost = blockState[goalBlock.y * nbrOfBlocksX + goalBlock.x].cost - goalHeuristic;
	path.pathGoal = path.path.front();
}


/*
 * clean lists from last search
 */
void CPathEstimator::ResetSearch() {
	while (!openBlocks.empty())
		openBlocks.pop();
	while (!dirtyBlocks.empty()) {
		blockState[dirtyBlocks.back()].cost = PATHCOST_INFINITY;
		blockState[dirtyBlocks.back()].parentBlock.x = -1;
		blockState[dirtyBlocks.back()].parentBlock.y = -1;
		blockState[dirtyBlocks.back()].options &= PATHOPT_OBSOLETE;
		dirtyBlocks.pop_back();
	}
	testedBlocks = 0;
}


/*
 * try to read offset and vertices data from file, return false on failure
 * TODO: Read-error-check.
 */
bool CPathEstimator::ReadFile(std::string name)
{
	unsigned int hash = Hash();
	char hashString[50];
	sprintf(hashString, "%u", hash);

	std::string filename = std::string("maps/paths/") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find_last_of('.') + 1) + hashString + "." + name + ".zip";

	// open file for reading from a suitable location (where the file exists)
	CArchiveZip* pfile = SAFE_NEW CArchiveZip(filesystem.LocateFile(filename));

	if (!pfile || !pfile->IsOpen()) {
		delete pfile;
		return false;
	}

	std::auto_ptr<CArchiveZip> auto_pfile(pfile);
	CArchiveZip& file(*pfile);

	int fh = file.OpenFile("pathinfo");

	if (fh) {
		pathChecksum = file.GetCrc32("pathinfo");

		unsigned int filehash = 0;
 		// Check hash.
		file.ReadFile(fh, &filehash, 4);
		if (filehash != hash)
			return false;

		// Read block-center-offset data.
		for (int blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			file.ReadFile(fh, blockState[blocknr].sqrCenter, moveinfo->moveData.size() * sizeof(int2));
		}

		// Read vertices data.
		file.ReadFile(fh, vertex, nbrOfVertices * sizeof(float));

		// File read successful.
		return true;
	} else {
		return false;
	}
}


/*
 * try to write offset and vertex data to file
 */
void CPathEstimator::WriteFile(std::string name) {
	// We need this directory to exist
	if (!filesystem.CreateDirectory("maps/paths"))
		return;

	unsigned int hash = Hash();
	char hashString[50];
	sprintf(hashString,"%u",hash);

	std::string filename = std::string("maps/paths/") + stupidGlobalMapname.substr(0, stupidGlobalMapname.find_last_of('.') + 1) + hashString + "." + name + ".zip";
	zipFile file;

	// open file for writing in a suitable location
	file = zipOpen(filesystem.LocateFile(filename, FileSystem::WRITE).c_str(), APPEND_STATUS_CREATE);

	if (file) {
		zipOpenNewFileInZip(file, "pathinfo", NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);

		// Write hash.
		unsigned int hash = Hash();
		zipWriteInFileInZip(file, (void*) &hash, 4);

		// Write block-center-offsets.
		for (int blocknr = 0; blocknr < nbrOfBlocks; blocknr++) {
			zipWriteInFileInZip(file, (void*) blockState[blocknr].sqrCenter, moveinfo->moveData.size() * sizeof(int2));
		}

		// Write vertices.
		zipWriteInFileInZip(file, (void*) vertex, nbrOfVertices * sizeof(float));

		zipCloseFileInZip(file);
		zipClose(file, NULL);


		// get the CRC over the written path data
		CArchiveZip* pfile = SAFE_NEW CArchiveZip(filesystem.LocateFile(filename));

		if (!pfile || !pfile->IsOpen()) {
			delete pfile;
			return;
		}

		std::auto_ptr<CArchiveZip> auto_pfile(pfile);
		CArchiveZip& file(*pfile);
		pathChecksum = file.GetCrc32("pathinfo");
	}
}


/*
Gives a hash-code identifying the dataset of this estimator.
*/
unsigned int CPathEstimator::Hash()
{
	return (readmap->mapChecksum + moveinfo->moveInfoChecksum + BLOCK_SIZE + moveMathOptions + PATHESTIMATOR_VERSION);
}

uint32_t CPathEstimator::GetPathChecksum()
{
	return pathChecksum;
}

void CPathEstimator::Draw(void)
{
	MoveData* md = moveinfo->GetMoveDataFromName("TANKSH2");
	if (!selectedUnits.selectedUnits.empty() && (*selectedUnits.selectedUnits.begin())->unitDef->movedata)
		md = (*selectedUnits.selectedUnits.begin())->unitDef->movedata;

	glDisable(GL_TEXTURE_2D);
	glColor3f(1, 1, 0);


/*
	float blue = BLOCK_SIZE == 32? 1: 0;
	glBegin(GL_LINES);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * 8;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * 8;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			glVertexf3(p1);
			glVertexf3(p1 - UpVector * 10);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * 8;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * 8;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1 / (sqrt(cost/BLOCK_SIZE)), 1 / (cost/BLOCK_SIZE), blue);
					glVertexf3(p1);
					glVertexf3(p2);
				}
			}
		}

	}
	glEnd();


/*
	glEnable(GL_TEXTURE_2D);
	for (int z = 0; z < nbrOfBlocksZ; z++) {
		for (int x = 0; x < nbrOfBlocksX; x++) {
			int blocknr = z * nbrOfBlocksX + x;
			float3 p1;
			p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p1.y = ground->GetHeight(p1.x, p1.z) + 10;

			glColor3f(1, 1, blue);
			for (int dir = 0; dir < PATH_DIRECTION_VERTICES; dir++) {
				int obx = x + directionVector[dir].x;
				int obz = z + directionVector[dir].y;

				if (obx >= 0 && obz >= 0 && obx < nbrOfBlocksX && obz < nbrOfBlocksZ) {
					float3 p2;
					int obblocknr = obz * nbrOfBlocksX + obx;

					p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
					p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
					p2.y = ground->GetHeight(p2.x, p2.z) + 10;

					int vertexNbr = md->pathType * nbrOfBlocks * PATH_DIRECTION_VERTICES + blocknr * PATH_DIRECTION_VERTICES + directionVertex[dir];
					float cost = vertex[vertexNbr];

					glColor3f(1, 1 / (cost/BLOCK_SIZE), blue);

					p2 = (p1 + p2) / 2;
					if (camera->pos.distance(p2) < 500) {
						glPushMatrix();
						glTranslatef3(p2);
						glScalef(5, 5, 5);
						font->glWorldPrint("%.0f", cost);
						glPopMatrix();
					}
				}
			}
		}
	}
*/


	if (BLOCK_SIZE == 8)
		glColor3f(0.2f, 0.7f, 0.2f);
	else
		glColor3f(0.2f, 0.2f, 0.7f);

	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);

	for (OpenBlock* ob = openBlockBuffer; ob != openBlockBufferPointer; ++ob) {
		int blocknr = ob->blocknr;
		float3 p1;
		p1.x = (blockState[blocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
		p1.z = (blockState[blocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
		p1.y = ground->GetHeight(p1.x, p1.z) + 15;

		float3 p2;
		int obx = blockState[ob->blocknr].parentBlock.x;
		int obz = blockState[ob->blocknr].parentBlock.y;
		int obblocknr = obz * nbrOfBlocksX + obx;

		if (obblocknr >= 0) {
			p2.x = (blockState[obblocknr].sqrCenter[md->pathType].x) * SQUARE_SIZE;
			p2.z = (blockState[obblocknr].sqrCenter[md->pathType].y) * SQUARE_SIZE;
			p2.y = ground->GetHeight(p2.x, p2.z) + 15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}

	glEnd();


/*
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(1,0,blue,0.7f);
	glAlphaFunc(GL_GREATER,0.05f);
	int a=0;
	for(OpenBlock*  ob=openBlockBuffer;ob!=openBlockBufferPointer;++ob){
		int blocknr = ob->blocknr;
		float3 p1;
		p1.x=(ob->block.x * BLOCK_SIZE + blockState[blocknr].sqrCenter[md->pathType].x)*SQUARE_SIZE;
		p1.z=(ob->block.y * BLOCK_SIZE + blockState[blocknr].sqrCenter[md->pathType].y)*SQUARE_SIZE;
		p1.y=ground->GetHeight(p1.x,p1.z)+15;

		if(camera->pos.distance(p1)<500){
			glPushMatrix();
			glTranslatef3(p1);
			glScalef(5,5,5);
			font->glWorldPrint("%.0f %.0f",ob->cost,ob->currentCost);
			glPopMatrix();
		}
		++a;
	}
	glDisable(GL_BLEND);
*/
}

float3 CPathEstimator::FindBestBlockCenter(const MoveData* moveData, float3 pos)
{
	int pathType = moveData->pathType;
	CRangedGoalWithCircularConstraint rangedGoal(pos, pos, 0, 0, SQUARE_SIZE * BLOCK_SIZE * SQUARE_SIZE * BLOCK_SIZE * 4);
	IPath::Path path;

	std::vector<float3> startPos;

	int xm = (int) (pos.x / (SQUARE_SIZE * BLOCK_SIZE));
	int ym = (int) (pos.z / (SQUARE_SIZE * BLOCK_SIZE));

	for (int y = std::max(0, ym - 1); y <= std::min(nbrOfBlocksZ - 1, ym + 1); ++y) {
		for (int x = std::max(0, xm - 1); x <= std::min(nbrOfBlocksX - 1, xm + 1); ++x) {
			startPos.push_back(float3(blockState[y * nbrOfBlocksX + x].sqrCenter[pathType].x * SQUARE_SIZE, 0, blockState[y * nbrOfBlocksX+x].sqrCenter[pathType].y * SQUARE_SIZE));
		}
	}

	IPath::SearchResult result = pathFinder->GetPath(*moveData, startPos, rangedGoal, path);

	if (result == IPath::Ok && !path.path.empty()) {
		return path.path.back();
	}
	return ZeroVector;
}
