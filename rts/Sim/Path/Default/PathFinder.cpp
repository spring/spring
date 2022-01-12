/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <ostream>

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
#include "System/MathConstants.h"


#define ENABLE_PATH_DEBUG 0
#define ENABLE_DIAG_TESTS 1

using namespace Bitwise;
using MMBT = CMoveMath::BlockTypes;

PFMemPool pfMemPool;


static constexpr uint32_t squareMobileBlockBits =
	uint32_t(MMBT::BLOCK_MOBILE_BUSY) |
	uint32_t(MMBT::BLOCK_MOBILE     ) |
	uint32_t(MMBT::BLOCK_MOVING     );

static constexpr CPathFinder::BlockCheckFunc blockCheckFuncs[2] = {
	CMoveMath::IsBlockedNoSpeedModCheckThreadUnsafe, // alias for RangeIsBlocked
	CMoveMath::IsBlockedNoSpeedModCheck // same as RangeIsBlocked without tempNum test
};

// both indexed by PATHOPT* bitmasks
static constexpr float PF_DIRECTION_COSTS[] = {
	0.0f       ,
	1.0f       , // PATHOPT_LEFT
	1.0f       , // PATHOPT_RIGHT
	0.0f       , // PATHOPT_LEFT | PATHOPT_RIGHT
	1.0f       , // PATHOPT_UP
	math::SQRT2, // PATHOPT_LEFT | PATHOPT_UP
	math::SQRT2, // PATHOPT_RIGHT | PATHOPT_UP
	0.0f       , // PATHOPT_LEFT | PATHOPT_RIGHT | PATHOPT_UP
	1.0f       , // PATHOPT_DOWN
	math::SQRT2, // PATHOPT_LEFT | PATHOPT_DOWN
	math::SQRT2, // PATHOPT_RIGHT | PATHOPT_DOWN
	0.0f       ,
	0.0f       ,
	0.0f       ,
	0.0f       ,
	0.0f       ,
};

//FIXME why not use PATHDIR_* consts and merge code with top one
static constexpr float3 PF_DIRECTION_VECTORS_3D[] = {
	{ 0,               0,                0},
	{+1,               0,                0}, // PATHOPT_LEFT
	{-1,               0,                0}, // PATHOPT_RIGHT
	{ 0,               0,                0}, // PATHOPT_LEFT | PATHOPT_RIGHT
	{ 0,               0,               +1}, // PATHOPT_UP
	{+math::HALFSQRT2, 0, +math::HALFSQRT2}, // PATHOPT_LEFT | PATHOPT_UP
	{-math::HALFSQRT2, 0, +math::HALFSQRT2}, // PATHOPT_RIGHT | PATHOPT_UP
	{ 0,               0,                0}, // PATHOPT_LEFT | PATHOPT_RIGHT | PATHOPT_UP
	{ 0,               0,               -1}, // PATHOPT_DOWN
	{+math::HALFSQRT2, 0, -math::HALFSQRT2}, // PATHOPT_LEFT | PATHOPT_DOWN
	{-math::HALFSQRT2, 0, -math::HALFSQRT2}, // PATHOPT_RIGHT | PATHOPT_DOWN
	{ 0,               0,                0},
	{ 0,               0,                0},
	{ 0,               0,                0},
	{ 0,               0,                0},
	{ 0,               0,                0}
};


void CPathFinder::InitStatic() {
	static_assert(PF_DIRECTION_COSTS[PATHOPT_LEFT                ] ==        1.0f, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_RIGHT               ] ==        1.0f, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_UP                  ] ==        1.0f, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_DOWN                ] ==        1.0f, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_LEFT  | PATHOPT_UP  ] == math::SQRT2, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_RIGHT | PATHOPT_UP  ] == math::SQRT2, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_RIGHT | PATHOPT_DOWN] == math::SQRT2, "");
	static_assert(PF_DIRECTION_COSTS[PATHOPT_LEFT  | PATHOPT_DOWN] == math::SQRT2, "");

	static_assert(PathDir2PathOpt(PATHDIR_LEFT      ) ==                  PATHOPT_LEFT , "");
	static_assert(PathDir2PathOpt(PATHDIR_RIGHT     ) ==                  PATHOPT_RIGHT, "");
	static_assert(PathDir2PathOpt(PATHDIR_UP        ) ==                  PATHOPT_UP   , "");
	static_assert(PathDir2PathOpt(PATHDIR_DOWN      ) ==                  PATHOPT_DOWN , "");
	static_assert(PathDir2PathOpt(PATHDIR_LEFT_UP   ) == (PATHOPT_LEFT  | PATHOPT_UP  ), "");
	static_assert(PathDir2PathOpt(PATHDIR_RIGHT_UP  ) == (PATHOPT_RIGHT | PATHOPT_UP  ), "");
	static_assert(PathDir2PathOpt(PATHDIR_RIGHT_DOWN) == (PATHOPT_RIGHT | PATHOPT_DOWN), "");
	static_assert(PathDir2PathOpt(PATHDIR_LEFT_DOWN ) == (PATHOPT_LEFT  | PATHOPT_DOWN), "");

	static_assert(PATHDIR_LEFT       == PathOpt2PathDir(                 PATHOPT_LEFT ), "");
	static_assert(PATHDIR_RIGHT      == PathOpt2PathDir(                 PATHOPT_RIGHT), "");
	static_assert(PATHDIR_UP         == PathOpt2PathDir(                 PATHOPT_UP   ), "");
	static_assert(PATHDIR_DOWN       == PathOpt2PathDir(                 PATHOPT_DOWN ), "");
	static_assert(PATHDIR_LEFT_UP    == PathOpt2PathDir((PATHOPT_LEFT  | PATHOPT_UP  )), "");
	static_assert(PATHDIR_RIGHT_UP   == PathOpt2PathDir((PATHOPT_RIGHT | PATHOPT_UP  )), "");
	static_assert(PATHDIR_RIGHT_DOWN == PathOpt2PathDir((PATHOPT_RIGHT | PATHOPT_DOWN)), "");
	static_assert(PATHDIR_LEFT_DOWN  == PathOpt2PathDir((PATHOPT_LEFT  | PATHOPT_DOWN)), "");

	static_assert(DIR2OPT[PATHDIR_LEFT] == PATHOPT_LEFT, "");
	static_assert(DIR2OPT[PATHDIR_LEFT] == PATHOPT_LEFT, "");

	// initialize direction-vectors table
	for (int i = 0; i < (PATH_DIRECTIONS << 1); ++i) {
		float3 temp(PF_DIRECTION_VECTORS_2D[i].x, 0.0f, PF_DIRECTION_VECTORS_2D[i].y);
		temp.SafeNormalize();
		assert(temp == PF_DIRECTION_VECTORS_3D[i]);
	}
}


void CPathFinder::Init(bool threadSafe)
{
	IPathFinder::Init(1);

	blockCheckFunc = blockCheckFuncs[threadSafe];
	dummyCacheItem = CPathCache::CacheItem{IPath::Error, {}, {-1, -1}, {-1, -1}, -1.0f, -1};
}


IPath::SearchResult CPathFinder::DoRawSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner
) {
	if (!moveDef.allowRawMovement)
		return IPath::Error;

	const int2 strtBlk = BlockIdxToPos(mStartBlockIdx);
	const int2 goalBlk = {int(pfDef.goalSquareX), int(pfDef.goalSquareZ)};
	const int2 diffBlk = {std::abs(goalBlk.x - strtBlk.x), std::abs(goalBlk.y - strtBlk.y)};
	// has not been set yet, DoSearch is called after us
	// const int2 goalBlk = BlockIdxToPos(mGoalBlockIdx);

	if ((Square(diffBlk.x) + Square(diffBlk.y)) > Square(pfDef.maxRawPathLen))
		return IPath::Error;


	const/*expr*/ auto StepFunc = [](const int2& dir, const int2& dif, int2& pos, int2& err) {
		pos.x += (dir.x * (err.y >= 0));
		pos.y += (dir.y * (err.y <= 0));
		err.x -= (dif.y * (err.y >= 0));
		err.x += (dif.x * (err.y <= 0));
	};

	const int2 fwdStepDir = int2{(goalBlk.x > strtBlk.x), (goalBlk.y > strtBlk.y)} * 2 - int2{1, 1};
	const int2 revStepDir = int2{(strtBlk.x > goalBlk.x), (strtBlk.y > goalBlk.y)} * 2 - int2{1, 1};

	int2 blkStepCtr = {diffBlk.x + diffBlk.y, diffBlk.x + diffBlk.y};
	int2 fwdStepErr = {diffBlk.x - diffBlk.y, diffBlk.x - diffBlk.y};
	int2 revStepErr = fwdStepErr;
	int2 fwdTestBlk = strtBlk;
	int2 revTestBlk = goalBlk;

	// test bidirectionally so bad goal-squares cause early exits
	// NOTE:
	//   no need for integration with backtracking in FinishSearch
	//   the final "path" only contains startPos which is consumed
	//   immediately, after which NextWayPoint keeps returning the
	//   goal until owner reaches it
	for (blkStepCtr += int2{1, 1}; (blkStepCtr.x > 0 && blkStepCtr.y > 0); blkStepCtr -= int2{1, 1}) {
		{
			if ((blockCheckFunc(moveDef, fwdTestBlk.x, fwdTestBlk.y, owner) & MMBT::BLOCK_STRUCTURE) != 0)
				return IPath::Error;
			if (CMoveMath::GetPosSpeedMod(moveDef, fwdTestBlk.x, fwdTestBlk.y) <= pfDef.minRawSpeedMod)
				return IPath::Error;
		}

		{
			if ((blockCheckFunc(moveDef, revTestBlk.x, revTestBlk.y, owner) & MMBT::BLOCK_STRUCTURE) != 0)
				return IPath::Error;
			if (CMoveMath::GetPosSpeedMod(moveDef, revTestBlk.x, revTestBlk.y) <= pfDef.minRawSpeedMod)
				return IPath::Error;
		}

		// NOTE: for odd-length paths, center square is tested twice
		if ((std::abs(fwdTestBlk.x - revTestBlk.x) <= 1) && (std::abs(fwdTestBlk.y - revTestBlk.y) <= 1))
			break;

		StepFunc(fwdStepDir, diffBlk * 2, fwdTestBlk, fwdStepErr);
		StepFunc(revStepDir, diffBlk * 2, revTestBlk, revStepErr);

		// skip if exactly crossing a vertex (in either direction)
		blkStepCtr.x -= (fwdStepErr.y == 0);
		blkStepCtr.y -= (revStepErr.y == 0);
		fwdStepErr.y  = fwdStepErr.x;
		revStepErr.y  = revStepErr.x;
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
		// get the open square with lowest expected path-cost
		const PathNode* openSquare = openBlocks.top();
		openBlocks.pop();

		// check if this PathNode has become obsolete
		if (blockStates.fCost[openSquare->nodeNum] != openSquare->fCost)
			continue;

		// check if the goal has been reached
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
		CMoveMath::BlockType blockMask = MMBT::BLOCK_IMPASSABLE;
		float speedMod = 0.0f;
		bool insideMap = true;
		bool insideDef = false;
	};

	SquareState ngbStates[PATH_DIRECTIONS];

	const int2 squarePos = square->nodePos;

	const bool startSquareExpanded = (openBlocks.empty() && testedBlocks < 8);
	const bool startSquareBlocked = (startSquareExpanded && (blockCheckFunc(moveDef, squarePos.x, squarePos.y, owner) & MMBT::BLOCK_STRUCTURE) != 0);

	// precompute structure-blocked state and speedmod for all neighbors
	for (SquareState& sqState: ngbStates) {
		const unsigned int dirIdx = &sqState - &ngbStates[0];
		const unsigned int optDir = PathDir2PathOpt(dirIdx);
		const int2 ngbSquareCoors = squarePos + PF_DIRECTION_VECTORS_2D[optDir];
		const unsigned int ngbSquareIdx = BlockPosToIdx(ngbSquareCoors);

		sqState.insideMap &= (static_cast<unsigned int>(ngbSquareCoors.x) < nbrOfBlocks.x);
		sqState.insideMap &= (static_cast<unsigned int>(ngbSquareCoors.y) < nbrOfBlocks.y);

		if (!sqState.insideMap)
			continue;

		if (blockStates.nodeMask[ngbSquareIdx] & (PATHOPT_CLOSED | PATHOPT_BLOCKED)) //FIXME
			continue;

		// IsBlockedNoSpeedModCheck; very expensive call but with a ~20% (?) chance of early-out
		if ((sqState.blockMask = blockCheckFunc(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, owner)) & MMBT::BLOCK_STRUCTURE) {
			blockStates.nodeMask[ngbSquareIdx] |= PATHOPT_CLOSED;
			dirtyBlocks.push_back(ngbSquareIdx);
			continue;
		}


		if (!pfDef.dirIndependent) {
			sqState.speedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, PF_DIRECTION_VECTORS_3D[optDir]);
		} else {
			// PE search; use positional speed-mods since PE assumes path-costs
			// are bidirectionally symmetric between parent and child vertices
			// no gain placing this in front of the above code, only has a ~2%
			// chance (heavily depending on the map) to early-out
			//
			// only close node if search is directionally independent, since it
			// might still be entered from another (better) direction otherwise
			if ((sqState.speedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y)) == 0.0f) {
				blockStates.nodeMask[ngbSquareIdx] |= PATHOPT_CLOSED;
				dirtyBlocks.push_back(ngbSquareIdx);
			}
		}

		// LHS is only here to save some cycles
		sqState.insideDef = (sqState.speedMod != 0.0f && pfDef.WithinConstraints(ngbSquareCoors.x, ngbSquareCoors.y));
	}


	const auto CanTestSquareSM = [&](const int dir) { return (ngbStates[dir].speedMod != 0.0f); };
	const auto CanTestSquareIS = [&](const int dir) { return (ngbStates[dir].insideDef       ); };

	#if ENABLE_DIAG_TESTS
	const auto TestDiagSquare = [&](const int dirX, const int dirY, const int dirXY) {
		if (!CanTestSquareSM(dirXY) || (!startSquareBlocked && (!CanTestSquareSM(dirX) || !CanTestSquareSM(dirY))))
			return;
		if (!CanTestSquareIS(dirXY) && (                        !CanTestSquareIS(dirX) || !CanTestSquareIS(dirY)))
			return;

		TestBlock(moveDef, pfDef, square, owner, PathDir2PathOpt(dirXY), ngbStates[dirXY].blockMask, ngbStates[dirXY].speedMod);
	};
	#endif

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
	// blocked here:
	//
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
	// make another exception if the starting square is inside
	// a concave corner s.t. all non-diagonal ngbs are blocked
	// (according to blockCheckFunc) and the MoveDef footprint
	// overlaps at least one impassable square, which would be
	// the case for a 5x5 footprint centered on S below
	//
	//   [.][.][.][.][X][X]
	//   [.][.][.][.][X][X]
	//   [.][.][S][.][.][ ]
	//   [.][.][.][.][.][ ]
	//   [X][X][.][.][X][X]
	//   [X][X][ ][ ][X][X]
	//
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
	assert((blockStatus & MMBT::BLOCK_STRUCTURE) == 0);
	assert(speedMod != 0.0f);

	if (pfDef.testMobile && moveDef.avoidMobilesOnPath) {
		switch (blockStatus & squareMobileBlockBits) {
			case (uint32_t(MMBT::BLOCK_MOBILE_BUSY) | uint32_t(MMBT::BLOCK_MOBILE) | uint32_t(MMBT::BLOCK_MOVING)):   // 111
			case (uint32_t(MMBT::BLOCK_MOBILE_BUSY) | uint32_t(MMBT::BLOCK_MOBILE) | uint32_t(MMBT::BLOCK_NONE  )):   // 110
			case (uint32_t(MMBT::BLOCK_MOBILE_BUSY) | uint32_t(MMBT::BLOCK_NONE  ) | uint32_t(MMBT::BLOCK_MOVING)):   // 101
			case (uint32_t(MMBT::BLOCK_MOBILE_BUSY) | uint32_t(MMBT::BLOCK_NONE  ) | uint32_t(MMBT::BLOCK_NONE  )): { // 100
				speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_BUSY_MULT];
			} break;

			case (uint32_t(MMBT::BLOCK_NONE       ) | uint32_t(MMBT::BLOCK_MOBILE) | uint32_t(MMBT::BLOCK_MOVING)):   // 011
			case (uint32_t(MMBT::BLOCK_NONE       ) | uint32_t(MMBT::BLOCK_MOBILE) | uint32_t(MMBT::BLOCK_NONE  )): { // 010
				speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_IDLE_MULT];
			} break;

			case (uint32_t(MMBT::BLOCK_NONE       ) | uint32_t(MMBT::BLOCK_NONE  ) | uint32_t(MMBT::BLOCK_MOVING)): { // 001
				speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_MOVE_MULT];
			} break;

			default: {
			} break;
		}
	}

	const float heatCost  = (pfDef.testMobile) ? (PathHeatMap::GetInstance())->GetHeatCost(square.x, square.y, moveDef, ((owner != nullptr)? owner->id: -1U)) : 0.0f;
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


void CPathFinder::FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& foundPath) const
{
	if (pfDef.needPath) {
		// backtrack
		int2 square = BlockIdxToPos(mGoalBlockIdx);

		unsigned int blockIdx = mGoalBlockIdx;
		unsigned int numNodes = 0;

		{
			while (blockIdx != mStartBlockIdx) {
				assert(PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS] != int2(0, 0));

				square   -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS];
				blockIdx  = BlockPosToIdx(square);
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
		int2 prvSquares[2] = {square, square};

		while (true) {
			foundPath.squares.push_back(square);
			foundPath.path.emplace_back(square.x * SQUARE_SIZE, CMoveMath::yLevel(moveDef, square.x, square.y), square.y * SQUARE_SIZE);

			// try to cut corners
			AdjustFoundPath(moveDef, foundPath, prvSquares[0], prvSquares[1], square);

			prvSquares[0] = prvSquares[1];
			prvSquares[1] = square;

			if (blockIdx == mStartBlockIdx)
				break;

			square -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS];
			blockIdx = BlockPosToIdx(square);
		}

		if (!foundPath.path.empty())
			foundPath.pathGoal = foundPath.path[0];
	}

	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx];
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

	if ((blockStates.nodeMask[tstSqrIdx] & PATHOPT_BLOCKED) != 0)
		return;
	if (blockStates.fCost[tstSqrIdx] > (COSTMOD * blockStates.fCost[prvSqrIdx]))
		return;

	const float3& p2 = foundPath.path[foundPath.path.size() - 3];
	      float3& p1 = foundPath.path[foundPath.path.size() - 2];
	const float3& p0 = foundPath.path[foundPath.path.size() - 1];

	FixupPath3Pts(moveDef, p0, p1, p2);
}

/*
 * This function takes the current and previous two waypoints and detects when they form a
 * "soft" (45 degrees, i.e. north-then-northwest) curve. If so, it then smooths the middle
 * waypoint to decrease the angle between p1-p2 and p2-p0. Hard turns like north-then-west
 * can and will not be smoothed.
 */
void CPathFinder::AdjustFoundPath(
	const MoveDef& moveDef,
	IPath::Path& foundPath,
	const int2& p1, // two squares before p0 (current)
	const int2& p2, // one square before p0 (current)
	const int2& p0
) const {
	int2 curDir = (p2 - p0);
	int2 prvDir = (p1 - p0) - curDir; // FIXME?
	assert((curDir.x % PATH_NODE_SPACING) == 0);
	assert((curDir.y % PATH_NODE_SPACING) == 0);
	assert((prvDir.x % PATH_NODE_SPACING) == 0);
	assert((prvDir.y % PATH_NODE_SPACING) == 0);
	curDir /= PATH_NODE_SPACING;
	prvDir /= PATH_NODE_SPACING;

	if (foundPath.path.size() < 3)
		return;

	for (unsigned pathDir = PATHDIR_LEFT; pathDir < PATH_DIRECTIONS; ++pathDir) {
		// find the pathDir matching the p2-p0 segment
		if (curDir != PE_DIRECTION_VECTORS[pathDir])
			continue;

		const bool lhTurn = (prvDir == PE_DIRECTION_VECTORS[(pathDir + PATH_DIRECTIONS - 1) % PATH_DIRECTIONS]);
		const bool rhTurn = (prvDir == PE_DIRECTION_VECTORS[(pathDir                   + 1) % PATH_DIRECTIONS]);

		if (rhTurn || lhTurn)
			SmoothMidWaypoint(p0 + (prvDir * PATH_NODE_SPACING), p2, moveDef, foundPath);

		break;
	}
}
