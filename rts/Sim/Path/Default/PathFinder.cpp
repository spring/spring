/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <ostream>
#include <deque>

#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathFlowMap.hpp"
#include "PathHeatMap.hpp"
#include "PathLog.h"
#include "PathMemPool.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/GeometricObjects.h"


#define ENABLE_PATH_DEBUG 0
#define ENABLE_DIAG_TESTS 1

using namespace Bitwise;

PFMemPool pfMemPool;


static const CMoveMath::BlockType squareMobileBlockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);
static const CPathFinder::BlockCheckFunc blockCheckFuncs[2] = {
	CMoveMath::IsBlockedNoSpeedModCheckThreadUnsafe,
	CMoveMath::IsBlockedNoSpeedModCheck
};

// both indexed by PATHOPT* bitmasks
static float3 PF_DIRECTION_VECTORS_3D[PATH_DIRECTIONS << 1];
static float  PF_DIRECTION_COSTS[PATH_DIRECTIONS << 1];


CPathFinder::CPathFinder(bool threadSafe): IPathFinder(1)
{
	blockCheckFunc = blockCheckFuncs[threadSafe];
	dummyCacheItem = CPathCache::CacheItem{IPath::Error, {}, {-1, -1}, {-1, -1}, -1.0f, -1};
}


void CPathFinder::InitStatic() {
	// initialize direction-vectors table
	for (int i = 0; i < (PATH_DIRECTIONS << 1); ++i) {
		PF_DIRECTION_VECTORS_3D[i].x = PF_DIRECTION_VECTORS_2D[i].x;
		PF_DIRECTION_VECTORS_3D[i].z = PF_DIRECTION_VECTORS_2D[i].y;
		PF_DIRECTION_VECTORS_3D[i].Normalize();
	}

	// initialize direction-costs table
	// note: PATH_NODE_SPACING should not affect these
	PF_DIRECTION_COSTS[PATHOPT_LEFT                ] =    1.0f;
	PF_DIRECTION_COSTS[PATHOPT_RIGHT               ] =    1.0f;
	PF_DIRECTION_COSTS[PATHOPT_UP                  ] =    1.0f;
	PF_DIRECTION_COSTS[PATHOPT_DOWN                ] =    1.0f;
	PF_DIRECTION_COSTS[PATHOPT_LEFT  | PATHOPT_UP  ] = 1.4142f;
	PF_DIRECTION_COSTS[PATHOPT_RIGHT | PATHOPT_UP  ] = 1.4142f;
	PF_DIRECTION_COSTS[PATHOPT_RIGHT | PATHOPT_DOWN] = 1.4142f;
	PF_DIRECTION_COSTS[PATHOPT_LEFT  | PATHOPT_DOWN] = 1.4142f;
}

const   int2* CPathFinder::GetDirectionVectorsTable2D() { return (&PF_DIRECTION_VECTORS_2D[0]); }
const float3* CPathFinder::GetDirectionVectorsTable3D() { return (&PF_DIRECTION_VECTORS_3D[0]); }



IPath::SearchResult CPathFinder::DoRawSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner
) {
	if (!moveDef.allowRawMovement || !pfDef.allowRawPath)
		return IPath::Error;

	const int2 strtBlk = BlockIdxToPos(mStartBlockIdx);
	const int2 goalBlk = {int(pfDef.goalSquareX), int(pfDef.goalSquareZ)};
	const int2 diffBlk = goalBlk - strtBlk;
	// const int2 goalBlk = BlockIdxToPos(mGoalBlockIdx);

	const float pathLen = math::sqrt(Square(diffBlk.x) + Square(diffBlk.y));
	const float testLen = math::ceil(pathLen);
	const float halfLen = math::floor(testLen * 0.5f);

	const float2 step = {diffBlk.x / pathLen, diffBlk.y / pathLen};
	// const float2 mods = {CMoveMath::GetPosSpeedMod(moveDef, strtBlk.x, strtBlk.y), CMoveMath::GetPosSpeedMod(moveDef, goalBlk.x, goalBlk.y)};
	const float2 lims = {pfDef.maxRawPathLen, pfDef.minRawSpeedMod};

	if (pathLen > lims.x)
		return IPath::Error;

	int2 fwdTestBlk;
	int2 revTestBlk;
	int2 fwdPrevBlk = {-1, -1};
	int2 revPrevBlk = {-1, -1};

	// test bidirectionally so bad goal-squares cause early exits
	// NOTE:
	//   no need to integrate with backtracking in FinishSearch
	//   the final "path" only contains startPos which is consumed
	//   immediately, after which NextWayPoint keeps returning the
	//   goal until owner reaches it
	for (float i = 0.0f, j = testLen; i <= halfLen; ) {
		assert(fwdPrevBlk.x == -1 || std::abs(fwdTestBlk.x - fwdPrevBlk.x) <= 1);
		assert(fwdPrevBlk.y == -1 || std::abs(fwdTestBlk.y - fwdPrevBlk.y) <= 1);
		assert(revPrevBlk.x == -1 || std::abs(revTestBlk.x - revPrevBlk.x) <= 1);
		assert(revPrevBlk.y == -1 || std::abs(revTestBlk.y - revPrevBlk.y) <= 1);

		if ((fwdTestBlk = strtBlk + step * i) != fwdPrevBlk) {
			if ((blockCheckFunc(moveDef, fwdTestBlk.x, fwdTestBlk.y, owner) & CMoveMath::BLOCK_STRUCTURE) != 0)
				return IPath::Error;
			if (CMoveMath::GetPosSpeedMod(moveDef, fwdTestBlk.x, fwdTestBlk.y) <= lims.y)
				return IPath::Error;

			fwdPrevBlk = fwdTestBlk;
		}

		// odd-length tests meet in the middle
		if (i >= j)
			break;

		if ((revTestBlk = strtBlk + step * j) != revPrevBlk) {
			if ((blockCheckFunc(moveDef, revTestBlk.x, revTestBlk.y, owner) & CMoveMath::BLOCK_STRUCTURE) != 0)
				return IPath::Error;
			if (CMoveMath::GetPosSpeedMod(moveDef, revTestBlk.x, revTestBlk.y) <= lims.y)
				return IPath::Error;

			revPrevBlk = revTestBlk;
		}

		i += 1.0f;
		j -= 1.0f;
	}

	return IPath::Ok;
}

IPath::SearchResult CPathFinder::DoSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner
) {
	bool foundGoal = false;

	while (!openBlocks.empty() && (openBlockBuffer.GetSize() < maxBlocksToBeSearched)) {
		// Get the open square with lowest expected path-cost.
		PathNode* openSquare = const_cast<PathNode*>(openBlocks.top());
		openBlocks.pop();

		// check if this PathNode has become obsolete
		if (blockStates.fCost[openSquare->nodeNum] != openSquare->fCost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(openSquare->nodePos.x, openSquare->nodePos.y)) {
			mGoalBlockIdx = openSquare->nodeNum;
			mGoalHeuristic = 0.0f;
			foundGoal = true;
			break;
		}

		if (!pfDef.WithinConstraints(openSquare->nodePos.x, openSquare->nodePos.y)) {
			blockStates.nodeMask[openSquare->nodeNum] |= PATHOPT_CLOSED;
			dirtyBlocks.push_back(openSquare->nodeNum);
			continue;
		}

		TestNeighborSquares(moveDef, pfDef, openSquare, owner);
	}

	if (foundGoal)
		return IPath::Ok;

	// could not reach goal within <maxBlocksToBeSearched> exploration limit
	if (openBlockBuffer.GetSize() >= maxBlocksToBeSearched)
		return IPath::GoalOutOfRange;

	// could not reach goal from this starting position if nothing to left to explore
	if (openBlocks.empty())
		return IPath::GoalOutOfRange;

	// should be unreachable
	return IPath::Error;
}

void CPathFinder::TestNeighborSquares(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* square,
	const CSolidObject* owner
) {
	struct SquareState {
		CMoveMath::BlockType blockMask = CMoveMath::BLOCK_IMPASSABLE;
		float speedMod = 0.0f;
		bool inSearch = false;
	};

	SquareState ngbStates[PATH_DIRECTIONS];

	// precompute structure-blocked state and speedmod for all neighbors
	for (unsigned int dir = 0; dir < PATH_DIRECTIONS; dir++) {
		const unsigned int optDir = PathDir2PathOpt(dir);
		const int2 ngbSquareCoors = square->nodePos + PF_DIRECTION_VECTORS_2D[optDir];
		const unsigned int ngbSquareIdx = BlockPosToIdx(ngbSquareCoors);

		if (static_cast<unsigned>(ngbSquareCoors.x) >= nbrOfBlocks.x || static_cast<unsigned>(ngbSquareCoors.y) >= nbrOfBlocks.y)
			continue;

		if (blockStates.nodeMask[ngbSquareIdx] & (PATHOPT_CLOSED | PATHOPT_BLOCKED)) //FIXME
			continue;

		SquareState& sqState = ngbStates[dir];

		// IsBlockedNoSpeedModCheck; very expensive call
		if ((sqState.blockMask = blockCheckFunc(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, owner)) & CMoveMath::BLOCK_STRUCTURE) {
			blockStates.nodeMask[ngbSquareIdx] |= PATHOPT_CLOSED;
			dirtyBlocks.push_back(ngbSquareIdx);
			continue; // early-out (20% chance)
		}


		if (!pfDef.dirIndependent) {
			sqState.speedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, PF_DIRECTION_VECTORS_3D[optDir]);
		} else {
			// PE search; use positional speed-mods since PE assumes path-costs
			// are bidirectionally symmetric between parent and child vertices
			// no gain placing this in front of the above code, only has a ~2%
			// chance (heavily depending on the map) to early-out
			//
			// only close node if search is directionally independent, otherwise
			// it is possible we might enter it from another (better) direction
			if ((sqState.speedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y)) == 0.0f) {
				blockStates.nodeMask[ngbSquareIdx] |= PATHOPT_CLOSED;
				dirtyBlocks.push_back(ngbSquareIdx);
			}
		}

		sqState.inSearch = (sqState.speedMod != 0.0f && pfDef.WithinConstraints(ngbSquareCoors.x, ngbSquareCoors.y));
	}

	const auto CanTestSquareSM = [&](const int dir) { return (ngbStates[dir].speedMod != 0.0f); };
	const auto CanTestSquareIS = [&](const int dir) { return (ngbStates[dir].inSearch); };

	// first test squares along the cardinal directions
	for (unsigned int dir: PATHDIR_CARDINALS) {
		if (!CanTestSquareSM(dir))
			continue;

		TestBlock(moveDef, pfDef, square, owner, PathDir2PathOpt(dir), ngbStates[dir].blockMask, ngbStates[dir].speedMod);
	}

	#if ENABLE_DIAG_TESTS
	// next test the diagonal squares
	//
	// don't search diagonally if there is a blocking object
	// (or blocking terrain!) in one of the two side squares
	// e.g. do not consider the edge (p, q) passable if X is
	// impassable in this situation:
	//   +---+---+
	//   | X | q |
	//   +---+---+
	//   | p | X |
	//   +---+---+
	//
	// *** IMPORTANT ***
	//
	// if either side-square is merely outside the constrained
	// area but the diagonal square is not, we do consider the
	// edge passable since we still need to be able to jump to
	// diagonally adjacent PE-blocks!
	//
	const auto TestDiagSquare = [&](const int dirX, const int dirY, const int dirXY) {
		if (!CanTestSquareSM(dirXY) || !CanTestSquareSM(dirX) || !CanTestSquareSM(dirY))
			return;
		if (!CanTestSquareIS(dirXY) && (!CanTestSquareIS(dirX) || !CanTestSquareIS(dirY)))
			return;

		TestBlock(moveDef, pfDef, square, owner, PathDir2PathOpt(dirXY), ngbStates[dirXY].blockMask, ngbStates[dirXY].speedMod);
	};

	TestDiagSquare(PATHDIR_LEFT,  PATHDIR_UP,   PATHDIR_LEFT_UP   );
	TestDiagSquare(PATHDIR_RIGHT, PATHDIR_UP,   PATHDIR_RIGHT_UP  );
	TestDiagSquare(PATHDIR_LEFT,  PATHDIR_DOWN, PATHDIR_LEFT_DOWN );
	TestDiagSquare(PATHDIR_RIGHT, PATHDIR_DOWN, PATHDIR_RIGHT_DOWN);
	#endif

	// mark this square as closed
	blockStates.nodeMask[square->nodeNum] |= PATHOPT_CLOSED;
	dirtyBlocks.push_back(square->nodeNum);
}

bool CPathFinder::TestBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* parentSquare,
	const CSolidObject* owner,
	const unsigned int pathOptDir,
	const unsigned int blockStatus,
	float speedMod
) {
	testedBlocks++;

	// initial calculations of the new block
	const int2 square = parentSquare->nodePos + PF_DIRECTION_VECTORS_2D[pathOptDir];
	const unsigned int sqrIdx = BlockPosToIdx(square);

	// bounds-check
	assert(static_cast<unsigned>(square.x) < nbrOfBlocks.x);
	assert(static_cast<unsigned>(square.y) < nbrOfBlocks.y);
	assert((blockStates.nodeMask[sqrIdx] & (PATHOPT_CLOSED | PATHOPT_BLOCKED)) == 0);
	assert((blockStatus & CMoveMath::BLOCK_STRUCTURE) == 0);
	assert(speedMod != 0.0f);

	if (pfDef.testMobile && moveDef.avoidMobilesOnPath && (blockStatus & squareMobileBlockBits)) {
		if (blockStatus & CMoveMath::BLOCK_MOBILE_BUSY) {
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_BUSY_MULT];
		} else if (blockStatus & CMoveMath::BLOCK_MOBILE) {
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_IDLE_MULT];
		} else { // (blockStatus & CMoveMath::BLOCK_MOVING)
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_MOVE_MULT];
		}
	}

	const float heatCost  = (pfDef.testMobile) ? (PathHeatMap::GetInstance())->GetHeatCost(square.x, square.y, moveDef, ((owner != NULL)? owner->id: -1U)) : 0.0f;
	//const float flowCost  = (pfDef.testMobile) ? (PathFlowMap::GetInstance())->GetFlowCost(square.x, square.y, moveDef, pathOptDir) : 0.0f;
	const float extraCost = blockStates.GetNodeExtraCost(square.x, square.y, pfDef.synced);

	const float dirMoveCost = (1.0f + heatCost) * PF_DIRECTION_COSTS[pathOptDir];
	const float nodeCost = (dirMoveCost / speedMod) + extraCost;

	const float gCost = parentSquare->gCost + nodeCost;                  // g
	const float hCost = pfDef.Heuristic(square.x, square.y, BLOCK_SIZE); // h
	const float fCost = gCost + hCost;                                   // f

	if (blockStates.nodeMask[sqrIdx] & PATHOPT_OPEN) {
		// already in the open set, look for a cost-improvement
		if (blockStates.fCost[sqrIdx] <= fCost)
			return true;

		blockStates.nodeMask[sqrIdx] &= ~PATHOPT_CARDINALS;
	}

	// if heuristic says this node is closer to goal than previous h-estimate, keep it
	if (!pfDef.exactPath && hCost < mGoalHeuristic) {
		mGoalBlockIdx = sqrIdx;
		mGoalHeuristic = hCost;
	}

	// store and mark this square as open (expanded, but not yet pulled from pqueue)
	openBlockBuffer.SetSize(openBlockBuffer.GetSize() + 1);
	assert(openBlockBuffer.GetSize() < MAX_SEARCHED_NODES_PF);

	PathNode* os = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		os->fCost   = fCost;
		os->gCost   = gCost;
		os->nodePos = square;
		os->nodeNum = sqrIdx;
	openBlocks.push(os);

	blockStates.SetMaxCost(NODE_COST_F, std::max(blockStates.GetMaxCost(NODE_COST_F), fCost));
	blockStates.SetMaxCost(NODE_COST_G, std::max(blockStates.GetMaxCost(NODE_COST_G), gCost));

	blockStates.fCost[sqrIdx] = os->fCost;
	blockStates.gCost[sqrIdx] = os->gCost;
	blockStates.nodeMask[sqrIdx] |= (PATHOPT_OPEN | pathOptDir);

	dirtyBlocks.push_back(sqrIdx);
	return true;
}


IPath::SearchResult CPathFinder::FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& foundPath) const
{
	// backtrack
	if (pfDef.needPath) {
		int2 square = BlockIdxToPos(mGoalBlockIdx);

		unsigned int blockIdx = mGoalBlockIdx;
		unsigned int numNodes = 0;

		{
			while (blockIdx != mStartBlockIdx) {
				blockIdx  = BlockPosToIdx(square -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS]);
				numNodes += 1;
			}

			// note: <squares> is only used for heatmapping (via PathManager::GetDetailedPathSquares)
			foundPath.squares.reserve(numNodes);
			foundPath.path.reserve(numNodes);

			// reset
			square = BlockIdxToPos(blockIdx = mGoalBlockIdx);
		}

		// for path adjustment (cutting corners)
		// make sure we don't match anything
		std::deque<int2> previous = {square, square};

		while (true) {
			foundPath.squares.push_back(square);
			foundPath.path.emplace_back(square.x * SQUARE_SIZE, CMoveMath::yLevel(moveDef, square.x, square.y), square.y * SQUARE_SIZE);

			// try to cut corners
			AdjustFoundPath(moveDef, foundPath, previous, square);

			previous.pop_front();
			previous.push_back(square);

			if (blockIdx == mStartBlockIdx)
				break;

			square -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS];
			blockIdx = BlockPosToIdx(square);
		}

		if (numNodes > 0) {
			foundPath.pathGoal = foundPath.path[0];
		}
	}

	// copy the path-cost
	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx];

	return IPath::Ok;
}




/** Helper function for SmoothMidWaypoint */
static inline void FixupPath3Pts(const MoveDef& moveDef, const float3 p1, float3& p2, const float3 p3)
{
#if ENABLE_PATH_DEBUG
	float3 old = p2;
#endif
	p2.x = 0.5f * (p1.x + p3.x);
	p2.z = 0.5f * (p1.z + p3.z);
	p2.y = CMoveMath::yLevel(moveDef, p2);

#if ENABLE_PATH_DEBUG
	geometricObjects->AddLine(old + UpVector * 10.0f, p2 + UpVector * 10.0f, 5, 10, 600, 0);
#endif
}


void CPathFinder::SmoothMidWaypoint(
	const int2 testSqr,
	const int2 prevSqr,
	const MoveDef& moveDef,
	IPath::Path& foundPath
) const {
	constexpr float COSTMOD = 1.39f; // (math::sqrt(2) + 1) / math::sqrt(3)

	const int tstSqrIdx = BlockPosToIdx(testSqr);
	const int prvSqrIdx = BlockPosToIdx(prevSqr);

	if (
		   ((blockStates.nodeMask[tstSqrIdx] & PATHOPT_BLOCKED) == 0)
		&& (blockStates.fCost[tstSqrIdx] <= COSTMOD * blockStates.fCost[prvSqrIdx])
	) {
		const float3& p2 = foundPath.path[foundPath.path.size() - 3];
		      float3& p1 = foundPath.path[foundPath.path.size() - 2];
		const float3& p0 = foundPath.path[foundPath.path.size() - 1];

		FixupPath3Pts(moveDef, p0, p1, p2);
	}
}

/*
 * This function takes the current & the last 2 waypoints and detects when they form
 * a "soft" curve. And if so, it takes the mid waypoint of those 3 and smooths it
 * between the one before and the current waypoint (so the soft curve gets even smoother).
 * Hint: hard curves (e.g. `move North then West`) can't and will not smoothed. Only soft ones
 *  like `move North then North-West` can.
 */
void CPathFinder::AdjustFoundPath(
	const MoveDef& moveDef,
	IPath::Path& foundPath,
	const std::deque<int2>& previous,
	int2 curSquare
) const {
	assert(previous.size() == 2);
	const int2& p1 = previous[0]; // two before curSquare
	const int2& p2 = previous[1]; // one before curSquare

	int2 dirNow = (p2 - curSquare);
	int2 dirPrv = (p1 - curSquare) - dirNow;
	assert(dirNow.x % PATH_NODE_SPACING == 0);
	assert(dirNow.y % PATH_NODE_SPACING == 0);
	assert(dirPrv.x % PATH_NODE_SPACING == 0);
	assert(dirPrv.y % PATH_NODE_SPACING == 0);
	dirNow /= PATH_NODE_SPACING;
	dirPrv /= PATH_NODE_SPACING;

	if (foundPath.path.size() < 3)
		return;

	for (unsigned pathDir = PATHDIR_LEFT; pathDir < PATH_DIRECTIONS; ++pathDir) {
		// find the pathDir
		if (dirNow != PE_DIRECTION_VECTORS[pathDir])
			continue;

		// only smooth "soft" curves (e.g. `move North-East then North`)
		if (
			   (dirPrv == PE_DIRECTION_VECTORS[(pathDir + PATH_DIRECTIONS - 1) % PATH_DIRECTIONS])
			|| (dirPrv == PE_DIRECTION_VECTORS[(pathDir                   + 1) % PATH_DIRECTIONS])
		) {
			SmoothMidWaypoint(curSquare + (dirPrv * PATH_NODE_SPACING), p2, moveDef, foundPath);
		}

		break;
	}
}
