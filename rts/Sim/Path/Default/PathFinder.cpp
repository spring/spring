/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring>
#include <ostream>
#include <deque>

#include "System/mmgr.h"
#include "PathAllocator.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
#include "PathLog.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/GeometricObjects.h"

#define PATHDEBUG 0

#if !defined(USE_MMGR)
void* CPathFinder::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathFinder::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }
#endif

const CMoveMath::BlockType squareMobileBlockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

CPathFinder::CPathFinder()
	: heatMapOffset(0)
	, heatMapping(true)
	, start(ZeroVector)
	, startxSqr(0)
	, startzSqr(0)
	, startSquare(0)
	, goalSquare(0)
	, goalHeuristic(0.0f)
	, exactPath(false)
	, testMobile(false)
	, needPath(false)
	, maxSquaresToBeSearched(0)
	, testedNodes(0)
	, maxNodeCost(0.0f)
	, squareStates(int2(gs->mapx, gs->mapy) , int2(gs->mapx, gs->mapy))
{
	InitHeatMap();

	// Precalculated vectors.
	dirVectors2D[PATHOPT_RIGHT               ].x = -2;
	dirVectors2D[PATHOPT_RIGHT               ].y =  0;
	dirVectors2D[PATHOPT_LEFT                ].x =  2;
	dirVectors2D[PATHOPT_LEFT                ].y =  0;
	dirVectors2D[PATHOPT_UP                  ].x =  0;
	dirVectors2D[PATHOPT_UP                  ].y =  2;
	dirVectors2D[PATHOPT_DOWN                ].x =  0;
	dirVectors2D[PATHOPT_DOWN                ].y = -2;
	dirVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].x = dirVectors2D[PATHOPT_RIGHT].x + dirVectors2D[PATHOPT_UP  ].x;
	dirVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].y = dirVectors2D[PATHOPT_RIGHT].y + dirVectors2D[PATHOPT_UP  ].y;
	dirVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].x = dirVectors2D[PATHOPT_LEFT ].x + dirVectors2D[PATHOPT_UP  ].x;
	dirVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].y = dirVectors2D[PATHOPT_LEFT ].y + dirVectors2D[PATHOPT_UP  ].y;
	dirVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].x = dirVectors2D[PATHOPT_RIGHT].x + dirVectors2D[PATHOPT_DOWN].x;
	dirVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].y = dirVectors2D[PATHOPT_RIGHT].y + dirVectors2D[PATHOPT_DOWN].y;
	dirVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].x = dirVectors2D[PATHOPT_LEFT ].x + dirVectors2D[PATHOPT_DOWN].x;
	dirVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].y = dirVectors2D[PATHOPT_LEFT ].y + dirVectors2D[PATHOPT_DOWN].y;

	dirVectors3D[PATHOPT_RIGHT               ].x = dirVectors2D[PATHOPT_RIGHT].x;
	dirVectors3D[PATHOPT_RIGHT               ].z = dirVectors2D[PATHOPT_RIGHT].y;
	dirVectors3D[PATHOPT_LEFT                ].x = dirVectors2D[PATHOPT_LEFT ].x;
	dirVectors3D[PATHOPT_LEFT                ].z = dirVectors2D[PATHOPT_LEFT ].y;
	dirVectors3D[PATHOPT_UP                  ].x = dirVectors2D[PATHOPT_UP   ].x;
	dirVectors3D[PATHOPT_UP                  ].z = dirVectors2D[PATHOPT_UP   ].y;
	dirVectors3D[PATHOPT_DOWN                ].x = dirVectors2D[PATHOPT_DOWN ].x;
	dirVectors3D[PATHOPT_DOWN                ].z = dirVectors2D[PATHOPT_DOWN ].y;
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].x = dirVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].x;
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].z = dirVectors2D[PATHOPT_RIGHT | PATHOPT_UP  ].y;
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].x = dirVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].x;
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].z = dirVectors2D[PATHOPT_LEFT  | PATHOPT_UP  ].y;
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].x = dirVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].x;
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].z = dirVectors2D[PATHOPT_RIGHT | PATHOPT_DOWN].y;
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].x = dirVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].x;
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].z = dirVectors2D[PATHOPT_LEFT  | PATHOPT_DOWN].y;

	dirVectors3D[PATHOPT_RIGHT               ].ANormalize();
	dirVectors3D[PATHOPT_LEFT                ].ANormalize();
	dirVectors3D[PATHOPT_UP                  ].ANormalize();
	dirVectors3D[PATHOPT_DOWN                ].ANormalize();
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_UP  ].ANormalize();
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_UP  ].ANormalize();
	dirVectors3D[PATHOPT_RIGHT | PATHOPT_DOWN].ANormalize();
	dirVectors3D[PATHOPT_LEFT  | PATHOPT_DOWN].ANormalize();

	moveCost[PATHOPT_RIGHT] = 1;
	moveCost[PATHOPT_LEFT ] = 1;
	moveCost[PATHOPT_UP   ] = 1;
	moveCost[PATHOPT_DOWN ] = 1;
	moveCost[(PATHOPT_RIGHT | PATHOPT_UP  )] = 1.42f;
	moveCost[(PATHOPT_LEFT  | PATHOPT_UP  )] = 1.42f;
	moveCost[(PATHOPT_RIGHT | PATHOPT_DOWN)] = 1.42f;
	moveCost[(PATHOPT_LEFT  | PATHOPT_DOWN)] = 1.42f;
}

CPathFinder::~CPathFinder()
{
	ResetSearch();
}



IPath::SearchResult CPathFinder::GetPath(
	const MoveDef& moveDef,
	const float3& startPos,
	const CPathFinderDef& pfDef,
	IPath::Path& path,
	bool testMobile,
	bool exactPath,
	unsigned int maxNodes,
	bool needPath,
	const CSolidObject* owner,
	bool synced
) {

	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// Store som basic data.
	maxSquaresToBeSearched = std::min(MAX_SEARCHED_NODES_PF - 8U, maxNodes);
	this->testMobile = testMobile;
	this->exactPath = exactPath;
	this->needPath = needPath;
	start = startPos;
	startxSqr = (int(start.x) / SQUARE_SIZE);
	startzSqr = (int(start.z) / SQUARE_SIZE);

	// Clamp the start position
	if (startxSqr <         0) startxSqr =            0;
	if (startxSqr >= gs->mapx) startxSqr = gs->mapxm1;
	if (startzSqr <         0) startzSqr =            0;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapym1;

	startSquare = startxSqr + startzSqr * gs->mapx;

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
	if (startxSqr <         0) { startxSqr =            0; }
	if (startxSqr >= gs->mapx) { startxSqr = gs->mapxm1; }
	if (startzSqr <         0) { startzSqr =            0; }
	if (startzSqr >= gs->mapy) { startzSqr = gs->mapym1; }

	const bool isStartGoal = pfDef.IsGoal(startxSqr, startzSqr);
	// although our starting square may be inside the goal radius, the starting coordinate may be outside.
	// in this case we do not want to return CantGetCloser, but instead a path to our starting square.
	if (isStartGoal && pfDef.startInGoalRadius)
		return IPath::CantGetCloser;

	// Clear the system from last search.
	ResetSearch();

	// Marks and store the start-square.
	squareStates.nodeMask[startSquare] = (PATHOPT_START | PATHOPT_OPEN);
	squareStates.fCost[startSquare] = 0.0f;
	squareStates.gCost[startSquare] = 0.0f;
	squareStates.SetMaxFCost(0.0f);
	squareStates.SetMaxGCost(0.0f);

	dirtySquares.push_back(startSquare);

	// Make the beginning the fest square found.
	goalSquare = startSquare;
	goalHeuristic = pfDef.Heuristic(startxSqr, startzSqr);

	// Adding the start-square to the queue.
	openSquareBuffer.SetSize(0);
	PathNode* os = openSquareBuffer.GetNode(openSquareBuffer.GetSize());
		os->fCost     = 0.0f;
		os->gCost     = 0.0f;
		os->nodePos.x = startxSqr;
		os->nodePos.y = startzSqr;
		os->nodeNum   = startSquare;
	openSquares.push(os);

	// perform the search
	IPath::SearchResult result = DoSearch(moveDef, pfDef, owner, synced);

	// if no improvements are found, then return CantGetCloser instead
	if ((goalSquare == startSquare && (!isStartGoal || pfDef.startInGoalRadius)) || goalSquare == 0) {
		return IPath::CantGetCloser;
	}

	return result;
}


IPath::SearchResult CPathFinder::DoSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, const CSolidObject* owner, bool synced) {
	bool foundGoal = false;

	while (!openSquares.empty() && (openSquareBuffer.GetSize() < maxSquaresToBeSearched)) {
		// Get the open square with lowest expected path-cost.
		PathNode* os = const_cast<PathNode*>(openSquares.top());
		openSquares.pop();

		// check if this PathNode has become obsolete
		if (squareStates.fCost[os->nodeNum] != os->fCost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(os->nodePos.x, os->nodePos.y)) {
			goalSquare = os->nodeNum;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		// Test the 8 surrounding squares.
		const bool right = TestSquare(moveDef, pfDef, os, PATHOPT_RIGHT, owner, synced);
		const bool left  = TestSquare(moveDef, pfDef, os, PATHOPT_LEFT,  owner, synced);
		const bool up    = TestSquare(moveDef, pfDef, os, PATHOPT_UP,    owner, synced);
		const bool down  = TestSquare(moveDef, pfDef, os, PATHOPT_DOWN,  owner, synced);

		if (up) {
			// we dont want to search diagonally if there is a blocking object
			// (not blocking terrain) in one of the two side squares
			if (right) { TestSquare(moveDef, pfDef, os, (PATHOPT_RIGHT | PATHOPT_UP), owner, synced); }
			if (left) { TestSquare(moveDef, pfDef, os, (PATHOPT_LEFT | PATHOPT_UP), owner, synced); }
		}
		if (down) {
			if (right) { TestSquare(moveDef, pfDef, os, (PATHOPT_RIGHT | PATHOPT_DOWN), owner, synced); }
			if (left) { TestSquare(moveDef, pfDef, os, (PATHOPT_LEFT | PATHOPT_DOWN), owner, synced); }
		}

		// Mark this square as closed.
		squareStates.nodeMask[os->nodeNum] |= PATHOPT_CLOSED;
	}

	if (foundGoal)
		return IPath::Ok;

	// Could not reach the goal.
	if (openSquareBuffer.GetSize() >= maxSquaresToBeSearched)
		return IPath::GoalOutOfRange;

	// Search could not reach the goal, due to the unit being locked in.
	if (openSquares.empty())
		return IPath::GoalOutOfRange;

	// Below shall never be runned.
	LOG_L(L_ERROR, "%s - Unhandled end of search!", __FUNCTION__);
	return IPath::Error;
}


bool CPathFinder::TestSquare(
	const MoveDef& moveDef,
	const CPathFinderDef& pfDef,
	const PathNode* parentOpenSquare,
	unsigned int enterDirection,
	const CSolidObject* owner,
	bool synced
) {
	testedNodes++;

	const int2& dirVec2D = dirVectors2D[enterDirection];
	const float3& dirVec3D = dirVectors3D[enterDirection];

	// Calculate the new square.
	int2 square;
		square.x = parentOpenSquare->nodePos.x + dirVec2D.x;
		square.y = parentOpenSquare->nodePos.y + dirVec2D.y;

	// Inside map?
	if (square.x < 0 || square.y < 0 || square.x >= gs->mapx || square.y >= gs->mapy) {
		return false;
	}

	const int sqrIdx = square.x + square.y * gs->mapx;
	const int sqrStatus = squareStates.nodeMask[sqrIdx];

	// Check if the square is unaccessable or used.
	if (sqrStatus & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED)) {
		return false;
	}

	const CMoveMath::BlockType blockStatus = CMoveMath::IsBlocked(moveDef, square.x, square.y, owner);

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
	float heatCostMod = 1.0f;

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

	// Include heatmap cost adjustment.
	if (heatMapping && moveDef.heatMapping && GetHeatOwner(square.x, square.y) != ((owner != NULL) ? owner->id : -1)) {
		heatCostMod += (moveDef.heatMod * GetHeatValue(square.x, square.y));
	}



	const float dirMoveCost = (heatCostMod * moveCost[enterDirection]);
	const float extraCost = squareStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = (dirMoveCost / squareSpeedMod) + extraCost;

	const float gCost = parentOpenSquare->gCost + nodeCost;  // g
	const float hCost = pfDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f


	if (squareStates.nodeMask[sqrIdx] & PATHOPT_OPEN) {
		// already in the open set
		if (squareStates.fCost[sqrIdx] <= fCost)
			return true;

		squareStates.nodeMask[sqrIdx] &= ~PATHOPT_DIRECTION;
	}

	// Look for improvements.
	if (!exactPath && hCost < goalHeuristic) {
		goalSquare = sqrIdx;
		goalHeuristic = hCost;
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

	squareStates.SetMaxFCost(std::max(squareStates.GetMaxFCost(), fCost));
	squareStates.SetMaxGCost(std::max(squareStates.GetMaxGCost(), gCost));

	// mark this square as open
	squareStates.fCost[sqrIdx] = os->fCost;
	squareStates.gCost[sqrIdx] = os->gCost;
	squareStates.nodeMask[sqrIdx] |= (PATHOPT_OPEN | enterDirection);
	dirtySquares.push_back(sqrIdx);
	return true;
}


void CPathFinder::FinishSearch(const MoveDef& moveDef, IPath::Path& foundPath) {
	// backtrack
	if (needPath) {
		int2 square;
			square.x = goalSquare % gs->mapx;
			square.y = goalSquare / gs->mapx;

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

			square.x -= dirVectors2D[squareStates.nodeMask[sqrIdx] & PATHOPT_DIRECTION].x;
			square.y -= dirVectors2D[squareStates.nodeMask[sqrIdx] & PATHOPT_DIRECTION].y;
		}

		if (!foundPath.path.empty()) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}

	// Adds the cost of the path.
	foundPath.pathCost = squareStates.fCost[goalSquare];
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
		const int lsquare = dirtySquares.back();
		dirtySquares.pop_back();

		squareStates.nodeMask[lsquare] = 0;
		squareStates.fCost[lsquare] = PATHCOST_INFINITY;
		squareStates.gCost[lsquare] = PATHCOST_INFINITY;
	}
	testedNodes = 0;
}






// heat mapping

void CPathFinder::SetHeatMapState(bool enabled)
{
	heatMapping = enabled;
}

void CPathFinder::InitHeatMap()
{
	heatmap.resize(gs->hmapx * gs->hmapy, HeatMapValue());
	heatMapOffset = 0;
}

void CPathFinder::UpdateHeatMap()
{
	++heatMapOffset;
}

int CPathFinder::GetHeatMapIndex(int x, int y)
{
	assert(!heatmap.empty());

	//! x & y are given in gs->mapi coords (:= gs->hmapi * 2)
	x >>= 1;
	y >>= 1;

	return y * gs->hmapx + x;
}
