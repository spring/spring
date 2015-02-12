/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <ostream>
#include <deque>

#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathFlowMap.hpp"
#include "PathHeatMap.hpp"
#include "PathLog.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/GeometricObjects.h"


#define PATHDEBUG 0

using namespace Bitwise;



const CMoveMath::BlockType squareMobileBlockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

// indexed by PATHOPT* bitmasks
static float3 PF_DIRECTION_VECTORS_3D[PATH_DIRECTIONS << 1];
static  float PF_DIRECTION_COSTS[PATH_DIRECTIONS << 1];



CPathFinder::CPathFinder()
	: IPathFinder(1)
{
}


void CPathFinder::InitDirectionVectorsTable() {
	for (int i = 0; i < (PATH_DIRECTIONS << 1); ++i) {
		PF_DIRECTION_VECTORS_3D[i].x = PF_DIRECTION_VECTORS_2D[i].x;
		PF_DIRECTION_VECTORS_3D[i].z = PF_DIRECTION_VECTORS_2D[i].y;
		PF_DIRECTION_VECTORS_3D[i].Normalize();
	}
}

void CPathFinder::InitDirectionCostsTable() {
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
	unsigned int ngbBlockedState[PATH_DIRECTIONS];
	bool ngbInSearchRadius[PATH_DIRECTIONS];
	float ngbPosSpeedMod[PATH_DIRECTIONS];
	float ngbSpeedMod[PATH_DIRECTIONS];

	// precompute structure-blocked and within-constraint states for all neighbors
	for (unsigned int dir = 0; dir < PATH_DIRECTIONS; dir++) {
		const int2 ngbSquareCoors = square->nodePos + PF_DIRECTION_VECTORS_2D[ PathDir2PathOpt(dir) ];

		ngbBlockedState[dir] = CMoveMath::IsBlockedNoSpeedModCheck(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, owner);
		ngbInSearchRadius[dir] = pfDef.WithinConstraints(ngbSquareCoors.x, ngbSquareCoors.y);

		// use the minimum of positional and directional speed-modifiers
		// because this agrees more with current assumptions in movetype
		// code and the estimators have no directional information
		const float posSpeedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y);
		const float dirSpeedMod = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, PF_DIRECTION_VECTORS_3D[ PathDir2PathOpt(dir) ]);
		ngbPosSpeedMod[dir] = posSpeedMod;
		// hint: use posSpeedMod for PE! cause it assumes path costs are bidirectional and so it only saves one `cost` for left & right movement
		ngbSpeedMod[dir] = (pfDef.dirIndependent) ? posSpeedMod : std::min(posSpeedMod, dirSpeedMod);
	}

	// first test squares along the cardinal directions
	for (unsigned int dir: PATHDIR_CARDINALS) {
		const unsigned int opt = PathDir2PathOpt(dir);

		if ((ngbBlockedState[dir] & CMoveMath::BLOCK_STRUCTURE) != 0)
			continue;
		if (!ngbInSearchRadius[dir])
			continue;

		TestBlock(moveDef, pfDef, square, owner, opt, ngbBlockedState[dir], ngbSpeedMod[dir], ngbInSearchRadius[dir]);
	}


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
	// if either side-square is merely outside the constrained
	// area but the diagonal square is not, we do consider the
	// edge passable since we still need to be able to jump to
	// diagonally adjacent PE-blocks
	//
	#define CAN_TEST_SQUARE(dir) ((ngbBlockedState[dir] & CMoveMath::BLOCK_STRUCTURE) == 0 && ngbPosSpeedMod[dir] != 0.0f)
	#define TEST_DIAG_SQUARE(BASE_DIR_X, BASE_DIR_Y, BASE_DIR_XY)                                                        \
		if (CAN_TEST_SQUARE(BASE_DIR_X) && CAN_TEST_SQUARE(BASE_DIR_Y) && CAN_TEST_SQUARE(BASE_DIR_XY)) {                \
			if ((ngbInSearchRadius[BASE_DIR_X] && ngbInSearchRadius[BASE_DIR_Y]) || ngbInSearchRadius[BASE_DIR_XY]) {          \
				const unsigned int ngbOpt = PathDir2PathOpt(BASE_DIR_XY);                                                \
				const unsigned int ngbBlk = ngbBlockedState[BASE_DIR_XY];                                                \
				const unsigned int ngbVis = ngbInSearchRadius[BASE_DIR_XY];                                                \
                                                                                                                         \
				TestBlock(moveDef, pfDef, square, owner, ngbOpt, ngbBlk, ngbSpeedMod[BASE_DIR_XY], ngbVis);   \
			}                                                                                                            \
		}

	TEST_DIAG_SQUARE(PATHDIR_LEFT,  PATHDIR_UP,   PATHDIR_LEFT_UP   )
	TEST_DIAG_SQUARE(PATHDIR_RIGHT, PATHDIR_UP,   PATHDIR_RIGHT_UP  )
	TEST_DIAG_SQUARE(PATHDIR_LEFT,  PATHDIR_DOWN, PATHDIR_LEFT_DOWN )
	TEST_DIAG_SQUARE(PATHDIR_RIGHT, PATHDIR_DOWN, PATHDIR_RIGHT_DOWN)

	#undef TEST_DIAG_SQUARE
	#undef CAN_TEST_SQUARE

	// mark this square as closed
	blockStates.nodeMask[square->nodeNum] |= PATHOPT_CLOSED;
}

bool CPathFinder::TestBlock(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* parentSquare,
	const CSolidObject* owner,
	const unsigned int pathOptDir,
	const unsigned int blockStatus,
	float speedMod,
	bool withinConstraints
) {
	testedBlocks++;

	// initial calculations of the new block
	const int2 square = parentSquare->nodePos + PF_DIRECTION_VECTORS_2D[pathOptDir];
	const unsigned int sqrIdx = BlockPosToIdx(square);

	// bounds-check
	if ((unsigned)square.x >= nbrOfBlocks.x) return false;
	if ((unsigned)square.y >= nbrOfBlocks.y) return false;

	// check if the square is inaccessable
	if (blockStates.nodeMask[sqrIdx] & (PATHOPT_CLOSED | PATHOPT_BLOCKED))
		return false;

	// caller has already tested for this
	assert((blockStatus & CMoveMath::BLOCK_STRUCTURE) == 0);

	// check if square is outside search-constraint
	// (this has already been done for open squares)
	if ((blockStates.nodeMask[sqrIdx] & PATHOPT_OPEN) == 0 && !withinConstraints) {
		blockStates.nodeMask[sqrIdx] |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(sqrIdx);
		return false;
	}

	// evaluate this square
	//

	if (speedMod == 0.0f) {
		blockStates.nodeMask[sqrIdx] |= PATHOPT_BLOCKED;
		dirtyBlocks.push_back(sqrIdx);
		return false;
	}

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
	const float flowCost  = (pfDef.testMobile) ? (PathFlowMap::GetInstance())->GetFlowCost(square.x, square.y, moveDef, pathOptDir) : 0.0f;
	const float extraCost = blockStates.GetNodeExtraCost(square.x, square.y, pfDef.synced);

	const float dirMoveCost = (1.0f + heatCost + flowCost) * PF_DIRECTION_COSTS[pathOptDir];
	const float nodeCost = (dirMoveCost / speedMod) + extraCost;

	const float gCost = parentSquare->gCost + nodeCost;      // g
	const float hCost = pfDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f

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

		// for path adjustment (cutting corners)
		std::deque<int2> previous;

		// make sure we don't match anything
		previous.push_back(square);
		previous.push_back(square);

		while (true) {
			float3 pos(square.x * SQUARE_SIZE, 0.0f, square.y * SQUARE_SIZE);
			pos.y = CMoveMath::yLevel(moveDef, square.x, square.y);

			// try to cut corners
			AdjustFoundPath(moveDef, foundPath, pos, previous, square);

			foundPath.path.push_back(pos);
			foundPath.squares.push_back(square);

			previous.pop_front();
			previous.push_back(square);

			if (blockIdx == mStartBlockIdx)
				break;

			square -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[blockIdx] & PATHOPT_CARDINALS];
			blockIdx = BlockPosToIdx(square);
		}

		if (!foundPath.path.empty()) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}

	// Adds the cost of the path.
	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx];

	return IPath::Ok;
}

/** Helper function for AdjustFoundPath */
static inline void FixupPath3Pts(const MoveDef& moveDef, const float3 p1, float3& p2, const float3 p3)
{
#if PATHDEBUG
	float3 old = p2;
#endif
	p2.x = 0.5f * (p1.x + p3.x);
	p2.z = 0.5f * (p1.z + p3.z);
	p2.y = CMoveMath::yLevel(moveDef, p2);

#if PATHDEBUG
	geometricObjects->AddLine(old + float3(0, 10, 0), p2 + float3(0, 10, 0), 5, 10, 600, 0);
#endif
}


void CPathFinder::SmoothMidWaypoint(const int2 testsqr, const int2 prevsqr, const MoveDef& moveDef, IPath::Path& foundPath, const float3 nextPoint) const
{
	static const float COSTMOD = 1.39f; // (math::sqrt(2) + 1) / math::sqrt(3)
	const int tstsqr = BlockPosToIdx(testsqr);
	const int prvsqr = BlockPosToIdx(prevsqr);
	if (
		   ((blockStates.nodeMask[tstsqr] & PATHOPT_BLOCKED) == 0)
		&& (blockStates.fCost[tstsqr] <= COSTMOD * blockStates.fCost[prvsqr])
	) {
		const float3& p2 = foundPath.path[foundPath.path.size() - 2];
		      float3& p1 = foundPath.path.back();
		const float3& p0 = nextPoint;
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
void CPathFinder::AdjustFoundPath(const MoveDef& moveDef, IPath::Path& foundPath, const float3 nextPoint,
	std::deque<int2>& previous, int2 curquare) const
{
	assert(previous.size() == 2);
	const int2& p1 = previous[0]; // two before curquare
	const int2& p2 = previous[1]; // one before curquare

	int2 dirNow = (p2 - curquare);
	int2 dirPrv = (p1 - curquare) - dirNow;
	assert(dirNow.x % PATH_NODE_SPACING == 0);
	assert(dirNow.y % PATH_NODE_SPACING == 0);
	assert(dirPrv.x % PATH_NODE_SPACING == 0);
	assert(dirPrv.y % PATH_NODE_SPACING == 0);
	dirNow /= PATH_NODE_SPACING;
	dirPrv /= PATH_NODE_SPACING;

	for (unsigned pathDir = PATHDIR_LEFT; pathDir < PATH_DIRECTIONS; ++pathDir) {
		// find the pathDir
		if (dirNow != PE_DIRECTION_VECTORS[pathDir])
			continue;

		// only smooth "soft" curves (e.g. `move North-East then North`)
		if (
			   (dirPrv == PE_DIRECTION_VECTORS[(pathDir-1) % PATH_DIRECTIONS])
			|| (dirPrv == PE_DIRECTION_VECTORS[(pathDir+1) % PATH_DIRECTIONS])
		) {
			SmoothMidWaypoint(curquare + (dirPrv * PATH_NODE_SPACING), p2, moveDef, foundPath, nextPoint);
		}
		break;
	}
}
