/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <ostream>
#include <deque>

#include "PathAllocator.h"
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
static   int2 PF_DIRECTION_VECTORS_2D[PATH_DIRECTIONS << 1];
static float3 PF_DIRECTION_VECTORS_3D[PATH_DIRECTIONS << 1];

static  float PF_DIRECTION_COSTS[PATH_DIRECTIONS << 1];



CPathFinder::CPathFinder()
	: start(ZeroVector)
	, startxSqr(0)
	, startzSqr(0)
	, mStartSquareIdx(0)
	, mGoalSquareIdx(0)
	, mGoalHeuristic(0.0f)
	, exactPath(false)
	, testMobile(false)
	, needPath(false)
	, maxOpenNodes(0)
	, testedNodes(0)
	, squareStates(int2(gs->mapx, gs->mapy), int2(gs->mapx, gs->mapy))
{
}

CPathFinder::~CPathFinder()
{
	ResetSearch();
}

void* CPathFinder::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathFinder::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }

void CPathFinder::InitDirectionVectorsTable() {
	PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT                ] = int2(+1 * PATH_NODE_SPACING,  0                    );
	PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT               ] = int2(-1 * PATH_NODE_SPACING,  0                    );
	PF_DIRECTION_VECTORS_2D[PATHOPT_UP                  ] = int2( 0,                     +1 * PATH_NODE_SPACING);
	PF_DIRECTION_VECTORS_2D[PATHOPT_DOWN                ] = int2( 0,                     -1 * PATH_NODE_SPACING);
	PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_UP  ] = int2(PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT ].x, PF_DIRECTION_VECTORS_2D[PATHOPT_UP   ].y);
	PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_UP  ] = int2(PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT].x, PF_DIRECTION_VECTORS_2D[PATHOPT_UP   ].y);
	PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_DOWN] = int2(PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT].x, PF_DIRECTION_VECTORS_2D[PATHOPT_DOWN ].y);
	PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_DOWN] = int2(PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT ].x, PF_DIRECTION_VECTORS_2D[PATHOPT_DOWN ].y);

	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT               ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT               ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT               ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT               ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT                ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT                ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT                ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT                ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_UP                  ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_UP                  ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_UP                  ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_UP                  ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_DOWN                ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_DOWN                ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_DOWN                ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_DOWN                ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_UP  ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_UP  ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_UP  ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_UP  ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_UP  ].x = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_UP  ].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_UP  ].z = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_UP  ].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_DOWN].x = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_DOWN].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_DOWN].z = PF_DIRECTION_VECTORS_2D[PATHOPT_RIGHT | PATHOPT_DOWN].y;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_DOWN].x = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_DOWN].x;
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_DOWN].z = PF_DIRECTION_VECTORS_2D[PATHOPT_LEFT  | PATHOPT_DOWN].y;

	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT               ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT                ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_UP                  ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_DOWN                ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_UP  ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_UP  ].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_RIGHT | PATHOPT_DOWN].ANormalize();
	PF_DIRECTION_VECTORS_3D[PATHOPT_LEFT  | PATHOPT_DOWN].ANormalize();
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
	bool exactPath,
	bool needPath,
	bool peCall,
	bool synced
) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	maxOpenNodes = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);

	// if true, factor presence of mobile objects on squares into cost
	this->testMobile = testMobile;
	// if true, neither start nor goal are allowed to be blocked squares
	this->exactPath = exactPath;
	// if false, we are only interested in the cost (not the waypoints)
	this->needPath = needPath;

	start = startPos;

	startxSqr = start.x / SQUARE_SIZE;
	startzSqr = start.z / SQUARE_SIZE;

	// Clamp the start position
	if (startxSqr >= gs->mapx) startxSqr = gs->mapxm1;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapym1;

	mStartSquareIdx = startxSqr + startzSqr * gs->mapx;

	// Start up the search.
	const IPath::SearchResult result = InitSearch(moveDef, pfDef, owner, peCall, synced);

	// Respond to the success of the search.
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, path);

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "Path found.");
			LOG_L(L_DEBUG, "Nodes tested: %u", testedNodes);
			LOG_L(L_DEBUG, "Open squares: %u", openSquareBuffer.GetSize());
			LOG_L(L_DEBUG, "Path nodes: " _STPF_, path.path.size());
			LOG_L(L_DEBUG, "Path cost: %f", path.pathCost);
		}
	} else {
		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "No path found!");
			LOG_L(L_DEBUG, "Nodes tested: %u", testedNodes);
			LOG_L(L_DEBUG, "Open squares: %u", openSquareBuffer.GetSize());
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
	if (exactPath) {
		assert(peCall);

		// if called from an estimator, we never want to allow searches from
		// any blocked starting positions (otherwise PE and PF can disagree)
		// note: PE itself should ensure this never happens to begin with?
		//
		// be more lenient for normal searches so players can "unstuck" units
		//
		// blocked goal positions are always early-outs (no searching needed)
		const bool strtBlocked = ((CMoveMath::IsBlocked(moveDef, start, owner) & CMoveMath::BLOCK_STRUCTURE) != 0);
		const bool goalBlocked = pfDef.GoalIsBlocked(moveDef, CMoveMath::BLOCK_STRUCTURE, owner);

 		if (strtBlocked || goalBlocked) {
			return IPath::CantGetCloser;
		}
	} else {
		assert(!peCall);

		// also need a "dead zone" around blocked goal-squares for non-exact paths
		// otherwise CPU use can become unacceptable even with circular constraint
		//
		// helps only a little because the allowed search radius in this case will
		// be much smaller as well (do not call PathFinderDef::GoalIsBlocked here
		// because that uses its own definition of "small area")
		if (owner != NULL) {
			const bool goalInRange = ((owner->pos - pfDef.goal).SqLength2D() < (owner->sqRadius + pfDef.sqGoalRadius));
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
	const bool isStartGoal = pfDef.IsGoal(startxSqr, startzSqr);

	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && pfDef.startInGoalRadius)
		return IPath::CantGetCloser;

	// Clear the system from last search.
	ResetSearch();

	// Marks and store the start-square.
	squareStates.nodeMask[mStartSquareIdx] = (PATHOPT_START | PATHOPT_OPEN);
	squareStates.fCost[mStartSquareIdx] = 0.0f;
	squareStates.gCost[mStartSquareIdx] = 0.0f;

	squareStates.SetMaxCost(NODE_COST_F, 0.0f);
	squareStates.SetMaxCost(NODE_COST_G, 0.0f);

	dirtySquares.push_back(mStartSquareIdx);

	// this is updated every square we get closer to our real goal (s.t. we
	// always have waypoints to return even if we only find a partial path)
	mGoalSquareIdx = mStartSquareIdx;
	mGoalHeuristic = pfDef.Heuristic(startxSqr, startzSqr);

	// add start-square to the queue
	openSquareBuffer.SetSize(0);
	PathNode* os = openSquareBuffer.GetNode(openSquareBuffer.GetSize());
		os->fCost     = 0.0f;
		os->gCost     = 0.0f;
		os->nodePos.x = startxSqr;
		os->nodePos.y = startzSqr;
		os->nodeNum   = mStartSquareIdx;
	openSquares.push(os);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, pfDef, owner, peCall, synced);

	// if no improvements are found, then return CantGetCloser instead
	if ((mGoalSquareIdx == mStartSquareIdx && (!isStartGoal || pfDef.startInGoalRadius)) || mGoalSquareIdx == 0)
		return IPath::CantGetCloser;

	return result;
}


IPath::SearchResult CPathFinder::DoSearch(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	bool peCall,
	bool synced
) {
	bool foundGoal = false;

	while (!openSquares.empty() && (openSquareBuffer.GetSize() < maxOpenNodes)) {
		// Get the open square with lowest expected path-cost.
		PathNode* openSquare = const_cast<PathNode*>(openSquares.top());
		openSquares.pop();

		// check if this PathNode has become obsolete
		if (squareStates.fCost[openSquare->nodeNum] != openSquare->fCost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(openSquare->nodePos.x, openSquare->nodePos.y)) {
			mGoalSquareIdx = openSquare->nodeNum;
			mGoalHeuristic = 0.0f;
			foundGoal = true;
			break;
		}

		TestNeighborSquares(moveDef, pfDef, openSquare, owner, synced);
	}

	if (foundGoal)
		return IPath::Ok;

	// could not reach goal within <maxOpenNodes> exploration limit
	if (openSquareBuffer.GetSize() >= maxOpenNodes)
		return IPath::GoalOutOfRange;

	// could not reach goal from this starting position if nothing to left to explore
	if (openSquares.empty())
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
	unsigned int ngbAllowedState[PATH_DIRECTIONS];

	float ngbPosSpeedMod[PATH_DIRECTIONS];
	float ngbDirSpeedMod[PATH_DIRECTIONS];

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
		ngbAllowedState[dir] = pfDef.WithinConstraints(ngbSquareCoors.x, ngbSquareCoors.y);

		ngbPosSpeedMod[dir] = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y);
		ngbDirSpeedMod[dir] = CMoveMath::GetPosSpeedMod(moveDef, ngbSquareCoors.x, ngbSquareCoors.y, PF_DIRECTION_VECTORS_3D[ PathDir2PathOpt(dir) ]);
	}

	// first test squares along the cardinal directions
	for (unsigned int n = 0; n < (sizeof(PATHDIR_CARDINALS) / sizeof(PATHDIR_CARDINALS[0])); n++) {
		const unsigned int dir = PATHDIR_CARDINALS[n];
		const unsigned int opt = PathDir2PathOpt(dir);

		if ((ngbBlockedState[dir] & CMoveMath::BLOCK_STRUCTURE) != 0)
			continue;
		if (!ngbAllowedState[dir])
			continue;

		TestSquare(moveDef, pfDef, square, owner, opt, ngbBlockedState[dir], ngbAllowedState[dir], ngbPosSpeedMod[dir], ngbDirSpeedMod[dir], synced);
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
			if ((ngbAllowedState[BASE_DIR_X] && ngbAllowedState[BASE_DIR_Y]) || ngbAllowedState[BASE_DIR_XY]) {          \
				const unsigned int ngbOpt = PathDir2PathOpt(BASE_DIR_XY);                                                \
				const unsigned int ngbBlk = ngbBlockedState[BASE_DIR_XY];                                                \
				const unsigned int ngbVis = ngbAllowedState[BASE_DIR_XY];                                                \
                                                                                                                         \
				const float ngbPosSpdMod = ngbPosSpeedMod[BASE_DIR_XY];                                                  \
				const float ngbDirSpdMod = ngbDirSpeedMod[BASE_DIR_XY];                                                  \
                                                                                                                         \
				TestSquare(moveDef, pfDef, square, owner, ngbOpt, ngbBlk, ngbVis, ngbPosSpdMod, ngbDirSpdMod, synced);   \
			}                                                                                                            \
		}

	TEST_DIAG_SQUARE(PATHDIR_LEFT,  PATHDIR_UP,   PATHDIR_LEFT_UP   )
	TEST_DIAG_SQUARE(PATHDIR_RIGHT, PATHDIR_UP,   PATHDIR_RIGHT_UP  )
	TEST_DIAG_SQUARE(PATHDIR_LEFT,  PATHDIR_DOWN, PATHDIR_LEFT_DOWN )
	TEST_DIAG_SQUARE(PATHDIR_RIGHT, PATHDIR_DOWN, PATHDIR_RIGHT_DOWN)

	#undef TEST_DIAG_SQUARE
	#undef CAN_TEST_SQUARE

	// mark this square as closed
	squareStates.nodeMask[square->nodeNum] |= PATHOPT_CLOSED;
}

bool CPathFinder::TestSquare(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* parentSquare,
	const CSolidObject* owner,
	const unsigned int pathOptDir,
	const unsigned int blockStatus,
	const float posSpeedMod,
	const float dirSpeedMod,
	bool withinConstraints,
	bool synced
) {
	testedNodes++;

	const int2 square = parentSquare->nodePos + PF_DIRECTION_VECTORS_2D[pathOptDir];
	const unsigned int sqrIdx = square.x + square.y * gs->mapx;

	// bounds-check
	if (square.x <         0 || square.y <         0) return false;
	if (square.x >= gs->mapx || square.y >= gs->mapy) return false;

	// check if the square is inaccessable
	if (squareStates.nodeMask[sqrIdx] & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED))
		return false;

	// caller has already tested for this
	assert((blockStatus & CMoveMath::BLOCK_STRUCTURE) == 0);

	// check if square is outside search-constraint
	// (this has already been done for open squares)
	if ((squareStates.nodeMask[sqrIdx] & PATHOPT_OPEN) == 0 && !withinConstraints) {
		squareStates.nodeMask[sqrIdx] |= PATHOPT_BLOCKED;
		dirtySquares.push_back(sqrIdx);
		return false;
	}

	// evaluate this square
	//
	// use the minimum of positional and directional speed-modifiers
	// because this agrees more with current assumptions in movetype
	// code and the estimators have no directional information
	float squareSpeedMod = std::min(posSpeedMod, dirSpeedMod);

	if (squareSpeedMod == 0.0f) {
		squareStates.nodeMask[sqrIdx] |= PATHOPT_FORBIDDEN;
		dirtySquares.push_back(sqrIdx);
		return false;
	}

	if (testMobile && moveDef.avoidMobilesOnPath && (blockStatus & squareMobileBlockBits)) {
		if (blockStatus & CMoveMath::BLOCK_MOBILE_BUSY) {
			squareSpeedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_BUSY_MULT];
		} else if (blockStatus & CMoveMath::BLOCK_MOBILE) {
			squareSpeedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_IDLE_MULT];
		} else { // (blockStatus & CMoveMath::BLOCK_MOVING)
			squareSpeedMod *= moveDef.speedModMults[MoveDef::SPEEDMOD_MOBILE_MOVE_MULT];
		}
	}

	const float heatCost = (PathHeatMap::GetInstance())->GetHeatCost(square.x, square.y, moveDef, ((owner != NULL)? owner->id: -1U));
	const float flowCost = (PathFlowMap::GetInstance())->GetFlowCost(square.x, square.y, moveDef, pathOptDir);

	const float dirMoveCost = (1.0f + heatCost + flowCost) * PF_DIRECTION_COSTS[pathOptDir];
	const float extraCost = squareStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = (dirMoveCost / squareSpeedMod) + extraCost;

	const float gCost = parentSquare->gCost + nodeCost;      // g
	const float hCost = pfDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f


	if (squareStates.nodeMask[sqrIdx] & PATHOPT_OPEN) {
		// already in the open set, look for a cost-improvement
		if (squareStates.fCost[sqrIdx] <= fCost)
			return true;

		squareStates.nodeMask[sqrIdx] &= ~PATHOPT_CARDINALS;
	}

	// if heuristic says this node is closer to goal than previous h-estimate, keep it
	if (!exactPath && hCost < mGoalHeuristic) {
		mGoalSquareIdx = sqrIdx;
		mGoalHeuristic = hCost;
	}

	// store and mark this square as open (expanded, but not yet pulled from pqueue)
	openSquareBuffer.SetSize(openSquareBuffer.GetSize() + 1);
	assert(openSquareBuffer.GetSize() < MAX_SEARCHED_NODES_PF);

	PathNode* os = openSquareBuffer.GetNode(openSquareBuffer.GetSize());
		os->fCost   = fCost;
		os->gCost   = gCost;
		os->nodePos = square;
		os->nodeNum = sqrIdx;
	openSquares.push(os);

	squareStates.SetMaxCost(NODE_COST_F, std::max(squareStates.GetMaxCost(NODE_COST_F), fCost));
	squareStates.SetMaxCost(NODE_COST_G, std::max(squareStates.GetMaxCost(NODE_COST_G), gCost));

	squareStates.fCost[sqrIdx] = os->fCost;
	squareStates.gCost[sqrIdx] = os->gCost;
	squareStates.nodeMask[sqrIdx] |= (PATHOPT_OPEN | pathOptDir);

	dirtySquares.push_back(sqrIdx);
	return true;
}


void CPathFinder::FinishSearch(const MoveDef& moveDef, IPath::Path& foundPath) {
	// backtrack
	if (needPath) {
		int2 square;
			square.x = mGoalSquareIdx % gs->mapx;
			square.y = mGoalSquareIdx / gs->mapx;

		// for path adjustment (cutting corners)
		std::deque<int2> previous;

		// make sure we don't match anything
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));

		while (true) {
			const int sqrIdx = square.y * gs->mapx + square.x;

			if (squareStates.nodeMask[sqrIdx] & PATHOPT_START)
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

			square.x -= PF_DIRECTION_VECTORS_2D[squareStates.nodeMask[sqrIdx] & PATHOPT_CARDINALS].x;
			square.y -= PF_DIRECTION_VECTORS_2D[squareStates.nodeMask[sqrIdx] & PATHOPT_CARDINALS].y;
		}

		if (!foundPath.path.empty()) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}

	// Adds the cost of the path.
	foundPath.pathCost = squareStates.fCost[mGoalSquareIdx];
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
	std::deque<int2>& previous, int2 square)
{
#define COSTMOD 1.39f	// (math::sqrt(2) + 1)/math::sqrt(3)
#define TRYFIX3POINTS(dxtest, dytest)                                                            \
	do {                                                                                         \
		int testsqr = square.x + (dxtest) + (square.y + (dytest)) * gs->mapx;                    \
		int p2sqr = previous[2].x + previous[2].y * gs->mapx;                                    \
		if (!(squareStates.nodeMask[testsqr] & (PATHOPT_BLOCKED | PATHOPT_FORBIDDEN)) &&         \
			 squareStates.fCost[testsqr] <= (COSTMOD) * squareStates.fCost[p2sqr]) {             \
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


void CPathFinder::ResetSearch()
{
	openSquares.Clear();

	while (!dirtySquares.empty()) {
		const unsigned int lsquare = dirtySquares.back();

		squareStates.nodeMask[lsquare] = 0;
		squareStates.fCost[lsquare] = PATHCOST_INFINITY;
		squareStates.gCost[lsquare] = PATHCOST_INFINITY;

		dirtySquares.pop_back();
	}

	testedNodes = 0;
}
