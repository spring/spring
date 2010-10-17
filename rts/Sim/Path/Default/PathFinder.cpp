/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cstring>
#include <ostream>
#include <deque>

#include "mmgr.h"
#include "PathAllocator.h"
#include "PathFinder.h"
#include "PathFinderDef.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Misc/GeometricObjects.h"
#include "System/LogOutput.h"

#define PATHDEBUG 0

#if !defined(USE_MMGR)
void* CPathFinder::operator new(size_t size) { return PathAllocator::Alloc(size); }
void CPathFinder::operator delete(void* p, size_t size) { PathAllocator::Free(p, size); }
#endif



CPathFinder::CPathFinder(): squareStates(int2(gs->mapx, gs->mapy), int2(gs->mapx, gs->mapy))
{
	heatMapping = true;
	InitHeatMap();

	// Precalculated vectors.
	directionVector[PATHOPT_RIGHT].x = -2;
	directionVector[PATHOPT_LEFT ].x =  2;
	directionVector[PATHOPT_UP   ].x =  0;
	directionVector[PATHOPT_DOWN ].x =  0;
	directionVector[(PATHOPT_RIGHT | PATHOPT_UP  )].x = directionVector[PATHOPT_RIGHT].x + directionVector[PATHOPT_UP  ].x;
	directionVector[(PATHOPT_LEFT  | PATHOPT_UP  )].x = directionVector[PATHOPT_LEFT ].x + directionVector[PATHOPT_UP  ].x;
	directionVector[(PATHOPT_RIGHT | PATHOPT_DOWN)].x = directionVector[PATHOPT_RIGHT].x + directionVector[PATHOPT_DOWN].x;
	directionVector[(PATHOPT_LEFT  | PATHOPT_DOWN)].x = directionVector[PATHOPT_LEFT ].x + directionVector[PATHOPT_DOWN].x;

	directionVector[PATHOPT_RIGHT].y =  0;
	directionVector[PATHOPT_LEFT ].y =  0;
	directionVector[PATHOPT_UP   ].y =  2;
	directionVector[PATHOPT_DOWN ].y = -2;
	directionVector[(PATHOPT_RIGHT | PATHOPT_UP  )].y = directionVector[PATHOPT_RIGHT].y + directionVector[PATHOPT_UP  ].y;
	directionVector[(PATHOPT_LEFT  | PATHOPT_UP  )].y = directionVector[PATHOPT_LEFT ].y + directionVector[PATHOPT_UP  ].y;
	directionVector[(PATHOPT_RIGHT | PATHOPT_DOWN)].y = directionVector[PATHOPT_RIGHT].y + directionVector[PATHOPT_DOWN].y;
	directionVector[(PATHOPT_LEFT  | PATHOPT_DOWN)].y = directionVector[PATHOPT_LEFT ].y + directionVector[PATHOPT_DOWN].y;

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
	squareStates.Clear();
}


/**
 * Search with several start positions
 */
IPath::SearchResult CPathFinder::GetPath(
	const MoveData& moveData,
	const std::vector<float3>& startPos,
	const CPathFinderDef& pfDef,
	IPath::Path& path,
	int ownerId,
	bool synced
) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// Store som basic data.
	maxSquaresToBeSearched = MAX_SEARCHED_NODES_PF - 8U;
	testMobile = false;
	exactPath = true;
	needPath = true;

	// If exact path is reqired and the goal is blocked, then no search is needed.
	if (exactPath && pfDef.GoalIsBlocked(moveData, (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN)))
		return IPath::CantGetCloser;

	// If the starting position is a goal position, then no search need to be performed.
	if (pfDef.IsGoal(startxSqr, startzSqr))
		return IPath::CantGetCloser;

	// Clearing the system from last search.
	ResetSearch();

	openSquareBuffer.SetSize(0);

	for (std::vector<float3>::const_iterator si = startPos.begin(); si != startPos.end(); ++si) {
		start = *si;
		startxSqr = (int(start.x) / SQUARE_SIZE) | 1;
		startzSqr = (int(start.z) / SQUARE_SIZE) | 1;
		startSquare = startxSqr + startzSqr * gs->mapx;
		goalSquare = startSquare;

		squareStates[startSquare].nodeMask = (PATHOPT_START | PATHOPT_OPEN);
		squareStates[startSquare].fCost = 0.0f;
		squareStates[startSquare].gCost = 0.0f;
		dirtySquares.push_back(startSquare);

		if (openSquareBuffer.GetSize() >= MAX_SEARCHED_NODES_PF) {
			continue;
		}

		PathNode* os = openSquareBuffer.GetNode(openSquareBuffer.GetSize());
			os->fCost     = 0.0f;
			os->gCost     = 0.0f;
			os->nodePos.x = startxSqr;
			os->nodePos.y = startzSqr;
			os->nodeNum   = startSquare;
		openSquareBuffer.SetSize(openSquareBuffer.GetSize() + 1);
		openSquares.push(os);
	}

	// note: DoSearch, not InitSearch
	IPath::SearchResult result = DoSearch(moveData, pfDef, ownerId, synced);

	// Respond to the success of the search.
	if (result == IPath::Ok) {
		FinishSearch(moveData, path);
		if (PATHDEBUG) {
			LogObject() << "Path found.\n";
			LogObject() << "Nodes tested: " << testedNodes << "\n";
			LogObject() << "Open squares: " << openSquareBuffer.GetSize() << "\n";
			LogObject() << "Path nodes: " << path.path.size() << "\n";
			LogObject() << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if (PATHDEBUG) {
			LogObject() << "No path found!\n";
			LogObject() << "Nodes tested: " << testedNodes << "\n";
			LogObject() << "Open squares: " << openSquareBuffer.GetSize() << "\n";
		}
	}
	return result;
}

IPath::SearchResult CPathFinder::GetPath(
	const MoveData& moveData,
	const float3& startPos,
	const CPathFinderDef& pfDef,
	IPath::Path& path,
	bool testMobile,
	bool exactPath,
	unsigned int maxNodes,
	bool needPath,
	int ownerId,
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
	startxSqr = (int(start.x) / SQUARE_SIZE)|1;
	startzSqr = (int(start.z) / SQUARE_SIZE)|1;

	// Clamp the start position
	if (startxSqr <         0) startxSqr =            0;
	if (startxSqr >= gs->mapx) startxSqr = gs->mapx - 1;
	if (startzSqr <         0) startzSqr =            0;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapy - 1;

	startSquare = startxSqr + startzSqr * gs->mapx;

	// Start up the search.
	IPath::SearchResult result = InitSearch(moveData, pfDef, ownerId, synced);

	// Respond to the success of the search.
	if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
		FinishSearch(moveData, path);

		if (PATHDEBUG) {
			LogObject() << "Path found.\n";
			LogObject() << "Nodes tested: " << testedNodes << "\n";
			LogObject() << "Open squares: " << openSquareBuffer.GetSize() << "\n";
			LogObject() << "Path nodes: " << path.path.size() << "\n";
			LogObject() << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if (PATHDEBUG) {
			LogObject() << "No path found!\n";
			LogObject() << "Nodes tested: " << testedNodes << "\n";
			LogObject() << "Open squares: " << openSquareBuffer.GetSize() << "\n";
		}
	}
	return result;
}


// set up the starting point of the search
IPath::SearchResult CPathFinder::InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId, bool synced) {
	// If exact path is reqired and the goal is blocked, then no search is needed.
	if (exactPath && pfDef.GoalIsBlocked(moveData, (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN)))
		return IPath::CantGetCloser;

	// Clamp the start position
	if (startxSqr <         0) { startxSqr =            0; }
	if (startxSqr >= gs->mapx) { startxSqr = gs->mapx - 1; }
	if (startzSqr <         0) { startzSqr =            0; }
	if (startzSqr >= gs->mapy) { startzSqr = gs->mapy - 1; }

	// If the starting position is a goal position, then no search need to be performed.
	if (pfDef.IsGoal(startxSqr, startzSqr))
		return IPath::CantGetCloser;

	// Clear the system from last search.
	ResetSearch();

	// Marks and store the start-square.
	squareStates[startSquare].nodeMask = (PATHOPT_START | PATHOPT_OPEN);
	squareStates[startSquare].fCost = 0.0f;
	squareStates[startSquare].gCost = 0.0f;
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
	IPath::SearchResult result = DoSearch(moveData, pfDef, ownerId, synced);

	// if no improvements are found, then return CantGetCloser instead
	if (goalSquare == startSquare || goalSquare == 0) {
		return IPath::CantGetCloser;
	}

	return result;
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathFinder::DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId, bool synced) {
	bool foundGoal = false;

	while (!openSquares.empty() && (openSquareBuffer.GetSize() < maxSquaresToBeSearched)) {
		// Get the open square with lowest expected path-cost.
		PathNode* os = const_cast<PathNode*>(openSquares.top());
		openSquares.pop();

		// check if this PathNode has become obsolete
		if (squareStates[os->nodeNum].fCost != os->fCost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(os->nodePos.x, os->nodePos.y)) {
			goalSquare = os->nodeNum;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		// Test the 8 surrounding squares.
		const bool right = TestSquare(moveData, pfDef, os, PATHOPT_RIGHT, ownerId, synced);
		const bool left  = TestSquare(moveData, pfDef, os, PATHOPT_LEFT,  ownerId, synced);
		const bool up    = TestSquare(moveData, pfDef, os, PATHOPT_UP,    ownerId, synced);
		const bool down  = TestSquare(moveData, pfDef, os, PATHOPT_DOWN,  ownerId, synced);

		if (up) {
			// we dont want to search diagonally if there is a blocking object
			// (not blocking terrain) in one of the two side squares
			if (right) { TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_UP), ownerId, synced); }
			if (left) { TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_UP), ownerId, synced); }
		}
		if (down) {
			if (right) { TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_DOWN), ownerId, synced); }
			if (left) { TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_DOWN), ownerId, synced); }
		}

		// Mark this square as closed.
		squareStates[os->nodeNum].nodeMask |= PATHOPT_CLOSED;
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
	LogObject() << "ERROR: CPathFinder::DoSearch() - Unhandled end of search!\n";
	return IPath::Error;
}


/**
 * Test the availability and value of a square,
 * and possibly add it to the queue of open squares.
 */
bool CPathFinder::TestSquare(
	const MoveData& moveData,
	const CPathFinderDef& pfDef,
	const PathNode* parentOpenSquare,
	unsigned int enterDirection,
	int ownerId,
	bool synced
) {
	testedNodes++;

	// Calculate the new square.
	int2 square;
		square.x = parentOpenSquare->nodePos.x + directionVector[enterDirection].x;
		square.y = parentOpenSquare->nodePos.y + directionVector[enterDirection].y;

	// Inside map?
	if (square.x < 0 || square.y < 0 || square.x >= gs->mapx || square.y >= gs->mapy) {
		return false;
	}

	const int sqrIdx = square.x + square.y * gs->mapx;
	const int sqrStatus = squareStates[sqrIdx].nodeMask;

	// Check if the square is unaccessable or used.
	if (sqrStatus & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED)) {
		return false;
	}

	const int blockStatus = moveData.moveMath->IsBlocked2(moveData, square.x, square.y);
	int blockBits = (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN);

	// Check if square are out of constraints or blocked by something.
	// Doesn't need to be done on open squares, as those are already tested.
	if ((!pfDef.WithinConstraints(square.x, square.y) || (blockStatus & blockBits)) &&
		!(sqrStatus & PATHOPT_OPEN)) {

		squareStates[sqrIdx].nodeMask |= PATHOPT_BLOCKED;
		dirtySquares.push_back(sqrIdx);
		return false;
	}

	// Evaluate this square.
	float squareSpeedMod = moveData.moveMath->SpeedMod(moveData, square.x, square.y);
	blockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

	if (squareSpeedMod == 0) {
		squareStates[sqrIdx].nodeMask |= PATHOPT_FORBIDDEN;
		dirtySquares.push_back(sqrIdx);
		return false;
	}

	if (testMobile && (blockStatus & blockBits)) {
		if (blockStatus & CMoveMath::BLOCK_MOVING)
			squareSpeedMod *= 0.65f;
		else if (blockStatus & CMoveMath::BLOCK_MOBILE)
			squareSpeedMod *= 0.35f;
		else
			squareSpeedMod *= 0.10f;
	}

	// Include heatmap cost adjustment.
	float heatCostMod = 1.0f;
	if (heatMapping && moveData.heatMapping && GetHeatOwner(square.x, square.y) != ownerId) {
		heatCostMod += (moveData.heatMod * GetHeatValue(square.x,square.y));
	}



	const float dirMoveCost = (heatCostMod * moveCost[enterDirection]);
	const float extraCost = squareStates.GetNodeExtraCost(square.x, square.y, synced);
	const float nodeCost = (dirMoveCost / squareSpeedMod) + extraCost;

	const float gCost = parentOpenSquare->gCost + nodeCost;  // g
	const float hCost = pfDef.Heuristic(square.x, square.y); // h
	const float fCost = gCost + hCost;                       // f


	if (squareStates[sqrIdx].nodeMask & PATHOPT_OPEN) {
		// already in the open set
		if (squareStates[sqrIdx].fCost <= fCost)
			return true;

		squareStates[sqrIdx].nodeMask &= ~PATHOPT_DIRECTION;
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
	squareStates[sqrIdx].fCost = os->fCost;
	squareStates[sqrIdx].gCost = os->gCost;
	squareStates[sqrIdx].nodeMask |= (PATHOPT_OPEN | enterDirection);
	dirtySquares.push_back(sqrIdx);
	return true;
}



/**
 * Recreates the path found by pathfinder.
 * Starting at goalSquare and tracking backwards.
 *
 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
 */
void CPathFinder::FinishSearch(const MoveData& moveData, IPath::Path& foundPath) {
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

			if (squareStates[sqrIdx].nodeMask & PATHOPT_START)
				break;

			float3 cs;
				cs.x = (square.x/2/* + 0.5f*/) * SQUARE_SIZE * 2 + SQUARE_SIZE;
				cs.z = (square.y/2/* + 0.5f*/) * SQUARE_SIZE * 2 + SQUARE_SIZE;
				cs.y = moveData.moveMath->yLevel(square.x, square.y);

			// try to cut corners
			AdjustFoundPath(moveData, foundPath, /* inout */ cs, previous, square);

			foundPath.path.push_back(cs);
			foundPath.squares.push_back(square);

			previous.pop_front();
			previous.push_back(square);

			int2 oldSquare;
				oldSquare.x = square.x;
				oldSquare.y = square.y;

			square.x -= directionVector[squareStates[sqrIdx].nodeMask & PATHOPT_DIRECTION].x;
			square.y -= directionVector[squareStates[sqrIdx].nodeMask & PATHOPT_DIRECTION].y;
		}

		if (!foundPath.path.empty()) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}

	// Adds the cost of the path.
	foundPath.pathCost = squareStates[goalSquare].fCost;
}

/** Helper function for AdjustFoundPath */
static inline void FixupPath3Pts(const MoveData& moveData, float3& p1, float3& p2, float3& p3, int2 sqr)
{
	float3 old = p2;
	old.y += 10;
	p2.x = 0.5f * (p1.x + p3.x);
	p2.z = 0.5f * (p1.z + p3.z);
	p2.y = moveData.moveMath->yLevel(sqr.x, sqr.y);

#if PATHDEBUG
	geometricObjects->AddLine(p3 + float3(0, 5, 0), p2 + float3(0, 10, 0), 5, 10, 600, 0);
	geometricObjects->AddLine(p3 + float3(0, 5, 0), old,                   5, 10, 600, 0);
#endif
}


/**
 * Adjusts the found path to cut corners where possible.
 */
void CPathFinder::AdjustFoundPath(const MoveData& moveData, IPath::Path& foundPath, float3& nextPoint,
	std::deque<int2>& previous, int2 square)
{
#define COSTMOD 1.39f	// (sqrt(2) + 1)/sqrt(3)
#define TRYFIX3POINTS(dxtest, dytest)                                                            \
	do {                                                                                         \
		int testsqr = square.x + (dxtest) + (square.y + (dytest)) * gs->mapx;                    \
		int p2sqr = previous[2].x + previous[2].y * gs->mapx;                                    \
		if (!(squareStates[testsqr].nodeMask & (PATHOPT_BLOCKED | PATHOPT_FORBIDDEN)) &&         \
			 squareStates[testsqr].fCost <= (COSTMOD) * squareStates[p2sqr].fCost) {             \
			float3& p2 = foundPath.path[foundPath.path.size() - 2];                              \
			float3& p1 = foundPath.path.back();                                                  \
			float3& p0 = nextPoint;                                                              \
			FixupPath3Pts(moveData, p0, p1, p2, int2(square.x + (dxtest), square.y + (dytest))); \
		}                                                                                        \
	} while (false)

	if (previous[2].x == square.x) {
		if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y-4) {
				if (PATHDEBUG) logOutput.Print("case N, NW");
				TRYFIX3POINTS(-2, -2);
			}
			else if (previous[1].x == square.x+2 && previous[1].y == square.y-4) {
				if (PATHDEBUG) logOutput.Print("case N, NE");
				TRYFIX3POINTS(2, -2);
			}
		}
		else if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y+4) {
				if (PATHDEBUG) logOutput.Print("case S, SE");
				TRYFIX3POINTS(2, 2);
			}
			else if (previous[1].x == square.x-2 && previous[1].y == square.y+4) {
				if (PATHDEBUG) logOutput.Print("case S, SW");
				TRYFIX3POINTS(-2, 2);
			}
		}
	}
	else if (previous[2].x == square.x-2) {
		if (previous[2].y == square.y) {
			if (previous[1].x == square.x-4 && previous[1].y == square.y-2) {
				if (PATHDEBUG) logOutput.Print("case W, NW");
				TRYFIX3POINTS(-2, -2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y+2) {
				if (PATHDEBUG) logOutput.Print("case W, SW");
				TRYFIX3POINTS(-2, 2);
			}
		}
		else if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y-4) {
				if (PATHDEBUG) logOutput.Print("case NW, N");
				TRYFIX3POINTS(0, -2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y-2) {
				if (PATHDEBUG) logOutput.Print("case NW, W");
				TRYFIX3POINTS(-2, 0);
			}
		}
		else if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x-2 && previous[1].y == square.y+4) {
				if (PATHDEBUG) logOutput.Print("case SW, S");
				TRYFIX3POINTS(0, 2);
			}
			else if (previous[1].x == square.x-4 && previous[1].y == square.y+2) {
				if (PATHDEBUG) logOutput.Print("case SW, W");
				TRYFIX3POINTS(-2, 0);
			}
		}
	}
	else if (previous[2].x == square.x+2) {
		if (previous[2].y == square.y) {
			if (previous[1].x == square.x+4 && previous[1].y == square.y-2) {
				if (PATHDEBUG) logOutput.Print("case NE, E");
				TRYFIX3POINTS(2, -2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y+2) {
				if (PATHDEBUG) logOutput.Print("case SE, E");
				TRYFIX3POINTS(2, 2);
			}
		}
		if (previous[2].y == square.y+2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y+4) {
				if (PATHDEBUG) logOutput.Print("case SE, S");
				TRYFIX3POINTS(0, 2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y+2) {
				if (PATHDEBUG) logOutput.Print("case SE, E");
				TRYFIX3POINTS(2, 0);
			}

		}
		else if (previous[2].y == square.y-2) {
			if (previous[1].x == square.x+2 && previous[1].y == square.y-4) {
				if (PATHDEBUG) logOutput.Print("case NE, N");
				TRYFIX3POINTS(0, -2);
			}
			else if (previous[1].x == square.x+4 && previous[1].y == square.y-2) {
				if (PATHDEBUG) logOutput.Print("case NE, E");
				TRYFIX3POINTS(0, -2);
			}
		}
	}
#undef TRYFIX3POINTS
#undef COSTMOD
}


/**
 * Clear things up from last search.
 */
void CPathFinder::ResetSearch()
{
	openSquares.Clear();

	while (!dirtySquares.empty()) {
		const int lsquare = dirtySquares.back();
		dirtySquares.pop_back();

		squareStates[lsquare].nodeMask = 0;
		squareStates[lsquare].fCost = PATHCOST_INFINITY;
		squareStates[lsquare].gCost = PATHCOST_INFINITY;
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
