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

void* CPathFinder::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathFinder::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }

const CMoveMath::BlockType squareMobileBlockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

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
	static const int   dirScale = 2;
	static const float dirCost  = math::sqrt(2.0f);

	directionVectors2D[PATHOPT_LEFT                ] = int2(+1 * dirScale,  0           );
	directionVectors2D[PATHOPT_RIGHT               ] = int2(-1 * dirScale,  0           );
	directionVectors2D[PATHOPT_UP                  ] = int2( 0,            +1 * dirScale);
	directionVectors2D[PATHOPT_DOWN                ] = int2( 0,            -1 * dirScale);
	directionVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ] = int2(directionVectors2D[PATHOPT_LEFT ].x, directionVectors2D[PATHOPT_UP   ].y);
	directionVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ] = int2(directionVectors2D[PATHOPT_RIGHT].x, directionVectors2D[PATHOPT_UP   ].y);
	directionVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN] = int2(directionVectors2D[PATHOPT_RIGHT].x, directionVectors2D[PATHOPT_DOWN ].y);
	directionVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN] = int2(directionVectors2D[PATHOPT_LEFT ].x, directionVectors2D[PATHOPT_DOWN ].y);

	directionVectors3D[PATHOPT_RIGHT               ].x = directionVectors2D[PATHOPT_RIGHT               ].x;
	directionVectors3D[PATHOPT_RIGHT               ].z = directionVectors2D[PATHOPT_RIGHT               ].y;
	directionVectors3D[PATHOPT_LEFT                ].x = directionVectors2D[PATHOPT_LEFT                ].x;
	directionVectors3D[PATHOPT_LEFT                ].z = directionVectors2D[PATHOPT_LEFT                ].y;
	directionVectors3D[PATHOPT_UP                  ].x = directionVectors2D[PATHOPT_UP                  ].x;
	directionVectors3D[PATHOPT_UP                  ].z = directionVectors2D[PATHOPT_UP                  ].y;
	directionVectors3D[PATHOPT_DOWN                ].x = directionVectors2D[PATHOPT_DOWN                ].x;
	directionVectors3D[PATHOPT_DOWN                ].z = directionVectors2D[PATHOPT_DOWN                ].y;
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].x = directionVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].x;
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].z = directionVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].y;
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].x = directionVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].x;
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].z = directionVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].y;
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].x = directionVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].x;
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].z = directionVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].y;
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].x = directionVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].x;
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].z = directionVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].y;

	directionVectors3D[PATHOPT_RIGHT               ].ANormalize();
	directionVectors3D[PATHOPT_LEFT                ].ANormalize();
	directionVectors3D[PATHOPT_UP                  ].ANormalize();
	directionVectors3D[PATHOPT_DOWN                ].ANormalize();
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].ANormalize();
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].ANormalize();
	directionVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].ANormalize();
	directionVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].ANormalize();

	directionCosts[PATHOPT_LEFT                ] =    1.0f; // * dirScale;
	directionCosts[PATHOPT_RIGHT               ] =    1.0f; // * dirScale;
	directionCosts[PATHOPT_UP                  ] =    1.0f; // * dirScale;
	directionCosts[PATHOPT_DOWN                ] =    1.0f; // * dirScale;
	directionCosts[PATHOPT_LEFT  | PATHOPT_UP  ] = dirCost; // * dirScale;
	directionCosts[PATHOPT_RIGHT | PATHOPT_UP  ] = dirCost; // * dirScale;
	directionCosts[PATHOPT_RIGHT | PATHOPT_DOWN] = dirCost; // * dirScale;
	directionCosts[PATHOPT_LEFT  | PATHOPT_DOWN] = dirCost; // * dirScale;
}

CPathFinder::~CPathFinder()
{
	ResetSearch();
}



IPath::SearchResult CPathFinder::GetPath(
	const MoveDef& moveDef,
	const float3& startPos,
	const CPathFinderDef& pfDef,
	const CSolidObject* owner,
	IPath::Path& path,
	unsigned int maxNodes,
	bool testMobile,
	bool exactPath,
	bool needPath,
	bool synced
) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	maxOpenNodes = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);

	this->testMobile = testMobile;
	this->exactPath = exactPath;
	this->needPath = needPath;

	start = startPos;

	startxSqr = start.x / SQUARE_SIZE;
	startzSqr = start.z / SQUARE_SIZE;

	// Clamp the start position
	if (startxSqr >= gs->mapx) startxSqr = gs->mapxm1;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapym1;

	mStartSquareIdx = startxSqr + startzSqr * gs->mapx;

	// Start up the search.
	IPath::SearchResult result = InitSearch(moveDef, pfDef, owner, synced);

	// Respond to the success of the search.
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveDef, path);

		if (LOG_IS_ENABLED(L_DEBUG)) {
			LOG_L(L_DEBUG, "Path found.");
			LOG_L(L_DEBUG, "Nodes tested: %u", testedNodes);
			LOG_L(L_DEBUG, "Open squares: %u", openSquareBuffer.GetSize());
			LOG_L(L_DEBUG, "Path nodes: "_STPF_, path.path.size());
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


IPath::SearchResult CPathFinder::InitSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool synced) {
	// If exact path is reqired and the goal is blocked, then no search is needed.
	if (exactPath && pfDef.GoalIsBlocked(moveDef, CMoveMath::BLOCK_STRUCTURE, owner))
		return IPath::CantGetCloser;

	// Clamp the start position
	if (startxSqr >= gs->mapx) { startxSqr = gs->mapxm1; }
	if (startzSqr >= gs->mapy) { startzSqr = gs->mapym1; }

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

	// Make the beginning the fest square found.
	mGoalSquareIdx = mStartSquareIdx;
	mGoalHeuristic = pfDef.Heuristic(startxSqr, startzSqr);

	// Adding the start-square to the queue.
	openSquareBuffer.SetSize(0);
	PathNode* os = openSquareBuffer.GetNode(openSquareBuffer.GetSize());
		os->fCost     = 0.0f;
		os->gCost     = 0.0f;
		os->nodePos.x = startxSqr;
		os->nodePos.y = startzSqr;
		os->nodeNum   = mStartSquareIdx;
	openSquares.push(os);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, pfDef, owner, synced);

	// if no improvements are found, then return CantGetCloser instead
	if ((mGoalSquareIdx == mStartSquareIdx && (!isStartGoal || pfDef.startInGoalRadius)) || mGoalSquareIdx == 0) {
		return IPath::CantGetCloser;
	}

	return result;
}


IPath::SearchResult CPathFinder::DoSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool synced) {
	bool foundGoal = false;

	while (!openSquares.empty() && (openSquareBuffer.GetSize() < maxOpenNodes)) {
		// Get the open square with lowest expected path-cost.
		PathNode* os = const_cast<PathNode*>(openSquares.top());
		openSquares.pop();

		// check if this PathNode has become obsolete
		if (squareStates.fCost[os->nodeNum] != os->fCost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(os->nodePos.x, os->nodePos.y)) {
			mGoalSquareIdx = os->nodeNum;
			mGoalHeuristic = 0.0f;
			foundGoal = true;
			break;
		}

		// Test the 8 surrounding squares.
		const bool right = TestSquare(moveDef, pfDef, os, owner, PATHOPT_RIGHT, synced);
		const bool left  = TestSquare(moveDef, pfDef, os, owner, PATHOPT_LEFT,  synced);
		const bool up    = TestSquare(moveDef, pfDef, os, owner, PATHOPT_UP,    synced);
		const bool down  = TestSquare(moveDef, pfDef, os, owner, PATHOPT_DOWN,  synced);

		if (up) {
			// we dont want to search diagonally if there is a blocking object
			// (not blocking terrain) in one of the two side squares
			if (right) { TestSquare(moveDef, pfDef, os, owner, (PATHOPT_RIGHT | PATHOPT_UP), synced); }
			if (left) { TestSquare(moveDef, pfDef, os, owner, (PATHOPT_LEFT | PATHOPT_UP), synced); }
		}
		if (down) {
			if (right) { TestSquare(moveDef, pfDef, os, owner, (PATHOPT_RIGHT | PATHOPT_DOWN), synced); }
			if (left) { TestSquare(moveDef, pfDef, os, owner, (PATHOPT_LEFT | PATHOPT_DOWN), synced); }
		}

		// Mark this square as closed.
		squareStates.nodeMask[os->nodeNum] |= PATHOPT_CLOSED;
	}

	if (foundGoal)
		return IPath::Ok;

	// Could not reach the goal.
	if (openSquareBuffer.GetSize() >= maxOpenNodes)
		return IPath::GoalOutOfRange;

	// Search could not reach the goal, due to the unit being locked in.
	if (openSquares.empty())
		return IPath::GoalOutOfRange;

	// should be unreachable
	// LogObject() << "ERROR: CPathFinder::DoSearch() - Unhandled end of search!\n";
	return IPath::Error;
}


bool CPathFinder::TestSquare(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* parentOpenSquare,
	const CSolidObject* owner,
	unsigned int pathOptDir,
	bool synced
) {
	testedNodes++;

	const int2& dirVec2D = directionVectors2D[pathOptDir];
	const float3& dirVec3D = directionVectors3D[pathOptDir];

	// Calculate the new square.
	int2 square;
		square.x = parentOpenSquare->nodePos.x + dirVec2D.x;
		square.y = parentOpenSquare->nodePos.y + dirVec2D.y;

	// Inside map?
	if (square.x < 0 || square.y < 0 || square.x >= gs->mapx || square.y >= gs->mapy) {
		return false;
	}

	const unsigned int sqrIdx = square.x + square.y * gs->mapx;
	const unsigned int sqrStatus = squareStates.nodeMask[sqrIdx];

	// Check if the square is unaccessable or used.
	if (sqrStatus & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED)) {
		return false;
	}

	const CMoveMath::BlockType blockStatus = CMoveMath::IsBlockedNoSpeedModCheck(moveDef, square.x, square.y, owner);

	// Check if square are out of constraints or blocked by something.
	// Doesn't need to be done on open squares, as those are already tested.
	if (!(sqrStatus & PATHOPT_OPEN) &&
		((blockStatus & CMoveMath::BLOCK_STRUCTURE) || !pfDef.WithinConstraints(square.x, square.y))
	) {
		squareStates.nodeMask[sqrIdx] |= PATHOPT_BLOCKED;
		dirtySquares.push_back(sqrIdx);
		return false;
	}

	// Evaluate this square.
	float squareSpeedMod = CMoveMath::GetPosSpeedMod(moveDef, square.x, square.y, dirVec3D);

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

	const float dirMoveCost = (1.0f + heatCost + flowCost) * directionCosts[pathOptDir];
	const float extraCost = squareStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = (dirMoveCost / squareSpeedMod) + extraCost;

	const float gCost = parentOpenSquare->gCost + nodeCost;  // g
	const float hCost = pfDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f


	if (squareStates.nodeMask[sqrIdx] & PATHOPT_OPEN) {
		// already in the open set
		if (squareStates.fCost[sqrIdx] <= fCost)
			return true;

		squareStates.nodeMask[sqrIdx] &= ~PATHOPT_AXIS_DIRS;
	}

	// Look for improvements.
	if (!exactPath && hCost < mGoalHeuristic) {
		mGoalSquareIdx = sqrIdx;
		mGoalHeuristic = hCost;
	}

	// Store this square as open.
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

	// mark this square as open
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

			int2 oldSquare;
				oldSquare.x = square.x;
				oldSquare.y = square.y;

			square.x -= directionVectors2D[squareStates.nodeMask[sqrIdx] & PATHOPT_AXIS_DIRS].x;
			square.y -= directionVectors2D[squareStates.nodeMask[sqrIdx] & PATHOPT_AXIS_DIRS].y;
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
