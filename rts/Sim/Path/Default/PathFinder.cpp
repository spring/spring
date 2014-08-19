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
#include "Sim/Misc/GeometricObjects.h"

#define PATHDEBUG 0

using namespace Bitwise;



const CMoveMath::BlockType squareMobileBlockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

// indexed by PATHOPT* bitmasks
static float3 PF_DIRECTION_VECTORS_3D[PATH_DIRECTIONS << 1];
static  float PF_DIRECTION_COSTS[PATH_DIRECTIONS << 1];



CPathFinder::CPathFinder()
	: IPathFinder(1)
	, exactPath(false)
	, testMobile(false)
	, needPath(false)
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



IPath::SearchResult CPathFinder::GetPath(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	const float3& startPos,
	IPath::Path& path,
	unsigned int maxNodes,
	bool testMobile,
	bool needPath,
	bool peCall,
	bool synced
) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	maxBlocksToBeSearched = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);

	// if true, factor presence of mobile objects on squares into cost
	this->testMobile = testMobile;
	// if true, neither start nor goal are allowed to be blocked squares
	this->exactPath = peCall;
	// if false, we are only interested in the cost (not the waypoints)
	this->needPath = needPath;

	// Clamp the start position
	mStartBlock = int2(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE);
	mStartBlock.x = Clamp(mStartBlock.x, 0, gs->mapxm1);
	mStartBlock.y = Clamp(mStartBlock.y, 0, gs->mapym1);

	mStartBlockIdx = mStartBlock.x + mStartBlock.y * gs->mapx;

	// Start up the search.
	const IPath::SearchResult result = InitSearch(moveDef, pfDef, owner, peCall, synced);

	// Respond to the success of the search.
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, path);

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "Path found.");
			LOG_L(L_DEBUG, "Nodes tested: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open squares: %u", openBlockBuffer.GetSize());
			LOG_L(L_DEBUG, "Path nodes: " _STPF_, path.path.size());
			LOG_L(L_DEBUG, "Path cost: %f", path.pathCost);
		}
	} else {
		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "No path found!");
			LOG_L(L_DEBUG, "Nodes tested: %u", testedBlocks);
			LOG_L(L_DEBUG, "Open squares: %u", openBlockBuffer.GetSize());
		}
	}

	return result;
}


IPath::SearchResult CPathFinder::InitSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	bool peCall,
	bool synced
) {
	if (!peCall) {
		// also need a "dead zone" around blocked goal-squares for non-exact paths
		// otherwise CPU use can become unacceptable even with circular constraint
		//
		// helps only a little because the allowed search radius in this case will
		// be much smaller as well (do not call PathFinderDef::GoalIsBlocked here
		// because that uses its own definition of "small area")
		if (owner != NULL) {
			const bool goalInRange = (pfDef.goal.SqDistance2D(owner->pos) < (owner->sqRadius + pfDef.sqGoalRadius));
			const bool goalBlocked = ((CMoveMath::IsBlocked(moveDef, pfDef.goal, owner) & CMoveMath::BLOCK_STRUCTURE) != 0);

			if (goalInRange && goalBlocked) {
				return IPath::CantGetCloser;
			}
		}
	}

	// start-position is clamped by caller
	// NOTE:
	//   if search starts on  odd-numbered square, only  odd-numbered nodes are explored
	//   if search starts on even-numbered square, only even-numbered nodes are explored
	const bool isStartGoal = pfDef.IsGoal(mStartBlock.x, mStartBlock.y);

	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && pfDef.startInGoalRadius)
		return IPath::CantGetCloser;

	// Clear the system from last search.
	ResetSearch();

	// Marks and store the start-square.
	blockStates.nodeMask[mStartBlockIdx] = (PATHOPT_START | PATHOPT_OPEN);
	blockStates.fCost[mStartBlockIdx] = 0.0f;
	blockStates.gCost[mStartBlockIdx] = 0.0f;

	blockStates.SetMaxCost(NODE_COST_F, 0.0f);
	blockStates.SetMaxCost(NODE_COST_G, 0.0f);

	dirtyBlocks.push_back(mStartBlockIdx);

	// this is updated every square we get closer to our real goal (s.t. we
	// always have waypoints to return even if we only find a partial path)
	mGoalBlockIdx = mStartBlockIdx;
	mGoalHeuristic = pfDef.Heuristic(mStartBlock.x, mStartBlock.y);

	// add start-square to the queue
	openBlockBuffer.SetSize(0);
	PathNode* os = openBlockBuffer.GetNode(openBlockBuffer.GetSize());
		os->fCost     = 0.0f;
		os->gCost     = 0.0f;
		os->nodePos.x = mStartBlock.x;
		os->nodePos.y = mStartBlock.y;
		os->nodeNum   = mStartBlockIdx;
	openBlocks.push(os);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, pfDef, owner, synced);

	// if no improvements are found, then return CantGetCloser instead
	if ((mGoalBlockIdx == mStartBlockIdx && (!isStartGoal || pfDef.startInGoalRadius)) || mGoalBlockIdx == 0)
		return IPath::CantGetCloser;

	return result;
}


IPath::SearchResult CPathFinder::DoSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	bool synced
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

		TestNeighborSquares(moveDef, pfDef, openSquare, owner, synced);
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
	const CSolidObject* owner,
	bool synced
) {
	unsigned int ngbBlockedState[PATH_DIRECTIONS];
	bool ngbInSearchRadius[PATH_DIRECTIONS];
	float ngbPosSpeedMod[PATH_DIRECTIONS];
	float ngbSpeedMod[PATH_DIRECTIONS];

	// note: because the spacing between nodes is 2 (not 1) we
	// must also make sure not to skip across any intermediate
	// impassable squares (!) but without reducing the spacing
	// factor which would drop performance four-fold --> messy
	//
	assert(PATH_NODE_SPACING == 2);

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
		ngbSpeedMod[dir] = std::min(posSpeedMod, dirSpeedMod);
	}

	// first test squares along the cardinal directions
	for (unsigned int dir: PATHDIR_CARDINALS) {
		const unsigned int opt = PathDir2PathOpt(dir);

		if ((ngbBlockedState[dir] & CMoveMath::BLOCK_STRUCTURE) != 0)
			continue;
		if (!ngbInSearchRadius[dir])
			continue;

		TestBlock(moveDef, pfDef, square, owner, opt, ngbBlockedState[dir], ngbSpeedMod[dir], ngbInSearchRadius[dir], synced);
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
				TestBlock(moveDef, pfDef, square, owner, ngbOpt, ngbBlk, ngbSpeedMod[BASE_DIR_XY], ngbVis, synced);   \
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
	bool withinConstraints,
	bool synced
) {
	testedBlocks++;

	const int2 square = parentSquare->nodePos + PF_DIRECTION_VECTORS_2D[pathOptDir];
	const unsigned int sqrIdx = square.x + square.y * gs->mapx;

	// bounds-check
	if (square.x <         0 || square.y <         0) return false;
	if (square.x >= gs->mapx || square.y >= gs->mapy) return false;

	// check if the square is inaccessable
	if (blockStates.nodeMask[sqrIdx] & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED))
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
		blockStates.nodeMask[sqrIdx] |= PATHOPT_FORBIDDEN;
		dirtyBlocks.push_back(sqrIdx);
		return false;
	}

	if (testMobile && moveDef.avoidMobilesOnPath && (blockStatus & squareMobileBlockBits)) {
		if (blockStatus & CMoveMath::BLOCK_MOBILE_BUSY) {
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_BUSY_MULT];
		} else if (blockStatus & CMoveMath::BLOCK_MOBILE) {
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_IDLE_MULT];
		} else { // (blockStatus & CMoveMath::BLOCK_MOVING)
			speedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_MOVE_MULT];
		}
	}

	const float heatCost = (PathHeatMap::GetInstance())->GetHeatCost(square.x, square.y, moveDef, ((owner != NULL)? owner->id: -1U));
	const float flowCost = (PathFlowMap::GetInstance())->GetFlowCost(square.x, square.y, moveDef, pathOptDir);

	const float dirMoveCost = (1.0f + heatCost + flowCost) * PF_DIRECTION_COSTS[pathOptDir];
	const float extraCost = blockStates.GetNodeExtraCost(square.x, square.y, synced);
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
	if (!exactPath && hCost < mGoalHeuristic) {
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


void CPathFinder::FinishSearch(const MoveDef& moveDef, IPath::Path& foundPath) const {
	// backtrack
	if (needPath) {
		int2 square;
			square.x = mGoalBlockIdx % gs->mapx;
			square.y = mGoalBlockIdx / gs->mapx;

		// for path adjustment (cutting corners)
		std::deque<int2> previous;

		// make sure we don't match anything
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));

		while (true) {
			const int sqrIdx = square.y * gs->mapx + square.x;

			if (blockStates.nodeMask[sqrIdx] & PATHOPT_START)
				break;

			float3 cs;
				cs.x = (square.x/2/* + 0.5f*/) * SQUARE_SIZE * 2 + SQUARE_SIZE;
				cs.z = (square.y/2/* + 0.5f*/) * SQUARE_SIZE * 2 + SQUARE_SIZE;
				cs.y = CMoveMath::yLevel(moveDef, square.x, square.y);

			// try to cut corners
			AdjustFoundPath(moveDef, foundPath, /* inout */ cs, previous, square);

			foundPath.path.push_back(cs);
			foundPath.squares.push_back(square);

			previous.pop_front();
			previous.push_back(square);

			square -= PF_DIRECTION_VECTORS_2D[blockStates.nodeMask[sqrIdx] & PATHOPT_CARDINALS];
		}

		if (!foundPath.path.empty()) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}

	// Adds the cost of the path.
	foundPath.pathCost = blockStates.fCost[mGoalBlockIdx];
}

/** Helper function for AdjustFoundPath */
static inline void FixupPath3Pts(const MoveDef& moveDef, float3& p1, float3& p2, float3& p3, int2 sqr)
{
	float3 old = p2;
	old.y += 10;
	p2.x = 0.5f * (p1.x + p3.x);
	p2.z = 0.5f * (p1.z + p3.z);
	p2.y = CMoveMath::yLevel(moveDef, sqr.x, sqr.y);

#if PATHDEBUG
	geometricObjects->AddLine(p3 + float3(0, 5, 0), p2 + float3(0, 10, 0), 5, 10, 600, 0);
	geometricObjects->AddLine(p3 + float3(0, 5, 0), old,                   5, 10, 600, 0);
#endif
}


void CPathFinder::AdjustFoundPath(const MoveDef& moveDef, IPath::Path& foundPath, float3& nextPoint,
	std::deque<int2>& previous, int2 square) const
{
#define COSTMOD 1.39f	// (math::sqrt(2) + 1)/math::sqrt(3)
#define TRYFIX3POINTS(dxtest, dytest)                                                            \
	do {                                                                                         \
		int testsqr = square.x + (dxtest) + (square.y + (dytest)) * gs->mapx;                    \
		int p2sqr = previous[2].x + previous[2].y * gs->mapx;                                    \
		if (!(blockStates.nodeMask[testsqr] & (PATHOPT_BLOCKED | PATHOPT_FORBIDDEN)) &&         \
			 blockStates.fCost[testsqr] <= (COSTMOD) * blockStates.fCost[p2sqr]) {             \
			float3& p2 = foundPath.path[foundPath.path.size() - 2];                              \
			float3& p1 = foundPath.path.back();                                                  \
			float3& p0 = nextPoint;                                                              \
			FixupPath3Pts(moveDef, p0, p1, p2, int2(square.x + (dxtest), square.y + (dytest))); \
		}                                                                                        \
	} while (false)

	if (previous[2].x == square.x) {
		if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y-4) {
				LOG_L(L_DEBUG, "case N, NW");
				TRYFIX3POINTS(-2, -2);
			}
			else if (previous[1].x == square.x+2 && previous[1].y == square.y-4) {
				LOG_L(L_DEBUG, "case N, NE");
				TRYFIX3POINTS(2, -2);
			}
		}
		else if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y+4) {
				LOG_L(L_DEBUG, "case S, SE");
				TRYFIX3POINTS(2, 2);
			}
			else if (previous[1].x == square.x-2 && previous[1].y == square.y+4) {
				LOG_L(L_DEBUG, "case S, SW");
				TRYFIX3POINTS(-2, 2);
			}
		}
	}
	else if (previous[2].x == square.x-2) {
		if (previous[2].y == square.y) {
			if (previous[1].x == square.x-4 && previous[1].y == square.y-2) {
				LOG_L(L_DEBUG, "case W, NW");
				TRYFIX3POINTS(-2, -2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y+2) {
				LOG_L(L_DEBUG, "case W, SW");
				TRYFIX3POINTS(-2, 2);
			}
		}
		else if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y-4) {
				LOG_L(L_DEBUG, "case NW, N");
				TRYFIX3POINTS(0, -2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y-2) {
				LOG_L(L_DEBUG, "case NW, W");
				TRYFIX3POINTS(-2, 0);
			}
		}
		else if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y+4) {
				LOG_L(L_DEBUG, "case SW, S");
				TRYFIX3POINTS(0, 2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y+2) {
				LOG_L(L_DEBUG, "case SW, W");
				TRYFIX3POINTS(-2, 0);
			}
		}
	}
	else if (previous[2].x == square.x+2) {
		if (previous[2].y == square.y) {
			if (previous[1].x == square.x+4 && previous[1].y == square.y-2) {
				LOG_L(L_DEBUG, "case NE, E");
				TRYFIX3POINTS(2, -2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y+2) {
				LOG_L(L_DEBUG, "case SE, E");
				TRYFIX3POINTS(2, 2);
			}
		}
		if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y+4) {
				LOG_L(L_DEBUG, "case SE, S");
				TRYFIX3POINTS(0, 2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y+2) {
				LOG_L(L_DEBUG, "case SE, E");
				TRYFIX3POINTS(2, 0);
			}

		}
		else if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y-4) {
				LOG_L(L_DEBUG, "case NE, N");
				TRYFIX3POINTS(0, -2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y-2) {
				LOG_L(L_DEBUG, "case NE, E");
				TRYFIX3POINTS(0, -2);
			}
		}
	}
#undef TRYFIX3POINTS
#undef COSTMOD
}
