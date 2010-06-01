/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <ostream>
#include <deque>
#include "mmgr.h"

#include "PathFinder.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Misc/GeometricObjects.h"
#include "System/LogOutput.h"

#define PATHDEBUG 0



// Cost constants.
const float PATHCOST_INFINITY = 10000000.0f;

void* pfAlloc(size_t n)
{
	char* ret=new char[n];
	for(int a=0;a<n;++a)
		ret[a]=0;

	return ret;
}

void pfDealloc(void *p,size_t n)
{
	delete[] ((char*)p);
}

/**
 * Constructor.
 * Building tables and precalculating data.
 */
CPathFinder::CPathFinder()
: openSquareBufferPointer(openSquareBuffer)
{
	heatMapping = false;
	InitHeatMap();

	// Creates and init all square states.
	squareState = new SquareState[gs->mapSquares];
	for(int a = 0; a < gs->mapSquares; ++a){
		squareState[a].status = 0;
		squareState[a].cost = PATHCOST_INFINITY;
	}
	for(int a=0;a<MAX_SEARCHED_SQUARES;++a){
		openSquareBuffer[a].cost=0;
		openSquareBuffer[a].currentCost=0;
		openSquareBuffer[a].sqr=0;
		openSquareBuffer[a].square.x=0;
		openSquareBuffer[a].square.y=0;
	}

/*	//Create border-constraints all around the map.
	//Need to be 2 squares thick.
	for(int x = 0; x < gs->mapx; ++x) {
		for(int y = 0; y < 2; ++y)
			squareState[y*gs->mapx+x].status |= PATHOPT_FORBIDDEN;
		for(int y = gs->mapy-2; y < gs->mapy; ++y)
			squareState[y*gs->mapx+x].status |= PATHOPT_FORBIDDEN;
	}
	for(int y = 0; y < gs->mapy; ++y){
		for(int x = 0; x < 2; ++x)
			squareState[y*gs->mapx+x].status |= PATHOPT_FORBIDDEN;
		for(int x = gs->mapx-2; x < gs->mapx; ++x)
			squareState[y*gs->mapx+x].status |= PATHOPT_FORBIDDEN;
	}
*/
	// Precalculated vectors.
	directionVector[PATHOPT_RIGHT].x = -2;
	directionVector[PATHOPT_LEFT].x = 2;
	directionVector[PATHOPT_UP].x = 0;
	directionVector[PATHOPT_DOWN].x = 0;
	directionVector[(PATHOPT_RIGHT | PATHOPT_UP)].x = directionVector[PATHOPT_RIGHT].x + directionVector[PATHOPT_UP].x;
	directionVector[(PATHOPT_LEFT | PATHOPT_UP)].x = directionVector[PATHOPT_LEFT].x + directionVector[PATHOPT_UP].x;
	directionVector[(PATHOPT_RIGHT | PATHOPT_DOWN)].x = directionVector[PATHOPT_RIGHT].x + directionVector[PATHOPT_DOWN].x;
	directionVector[(PATHOPT_LEFT | PATHOPT_DOWN)].x = directionVector[PATHOPT_LEFT].x + directionVector[PATHOPT_DOWN].x;

	directionVector[PATHOPT_RIGHT].y = 0;
	directionVector[PATHOPT_LEFT].y = 0;
	directionVector[PATHOPT_UP].y = 2;
	directionVector[PATHOPT_DOWN].y = -2;
	directionVector[(PATHOPT_RIGHT | PATHOPT_UP)].y = directionVector[PATHOPT_RIGHT].y + directionVector[PATHOPT_UP].y;
	directionVector[(PATHOPT_LEFT | PATHOPT_UP)].y = directionVector[PATHOPT_LEFT].y + directionVector[PATHOPT_UP].y;
	directionVector[(PATHOPT_RIGHT | PATHOPT_DOWN)].y = directionVector[PATHOPT_RIGHT].y + directionVector[PATHOPT_DOWN].y;
	directionVector[(PATHOPT_LEFT | PATHOPT_DOWN)].y = directionVector[PATHOPT_LEFT].y + directionVector[PATHOPT_DOWN].y;

	moveCost[PATHOPT_RIGHT] = 1;
	moveCost[PATHOPT_LEFT] = 1;
	moveCost[PATHOPT_UP] = 1;
	moveCost[PATHOPT_DOWN] = 1;
	moveCost[(PATHOPT_RIGHT | PATHOPT_UP)] = 1.42f;
	moveCost[(PATHOPT_LEFT | PATHOPT_UP)] = 1.42f;
	moveCost[(PATHOPT_RIGHT | PATHOPT_DOWN)] = 1.42f;
	moveCost[(PATHOPT_LEFT | PATHOPT_DOWN)] = 1.42f;
}


/**
 * Destructor.
 * Free used memory.
 */
CPathFinder::~CPathFinder()
{
	ResetSearch();
	delete[] squareState;
}


/**
 * Search with several start positions
 */
IPath::SearchResult CPathFinder::GetPath(const MoveData& moveData, const std::vector<float3>& startPos,
		const CPathFinderDef& pfDef, Path& path, int ownerId) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// Store som basic data.
	maxNodesToBeSearched = MAX_SEARCHED_SQUARES;
	testMobile = false;
	exactPath = true;
	needPath = true;

	// If exact path is reqired and the goal is blocked, then no search is needed.
	if (exactPath && pfDef.GoalIsBlocked(moveData, (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN)))
		return CantGetCloser;

	// If the starting position is a goal position, then no search need to be performed.
	if (pfDef.IsGoal(startxSqr, startzSqr))
		return CantGetCloser;

	// Clearing the system from last search.
	ResetSearch();

	openSquareBufferPointer = &openSquareBuffer[0];

	for (std::vector<float3>::const_iterator si = startPos.begin(); si != startPos.end(); ++si) {
		start = *si;
		startxSqr = (int(start.x) / SQUARE_SIZE) | 1;
		startzSqr = (int(start.z) / SQUARE_SIZE) | 1;
		startSquare = startxSqr + startzSqr * gs->mapx;

		squareState[startSquare].status = (PATHOPT_START | PATHOPT_OPEN);
		squareState[startSquare].cost = 0;
		dirtySquares.push_back(startSquare);

		goalSquare = startSquare;

		OpenSquare *os = ++openSquareBufferPointer; // Taking first OpenSquare in buffer.
		os->currentCost = 0;
		os->cost = 0;
		os->square.x = startxSqr;
		os->square.y = startzSqr;
		os->sqr = startSquare;
		openSquares.push(os);
	}

	// Performs the search.
	SearchResult result = DoSearch(moveData, pfDef, ownerId);

	// Respond to the success of the search.
	if(result == Ok) {
		FinishSearch(moveData, path);
		if(PATHDEBUG) {
			LogObject() << "Path found.\n";
			LogObject() << "Nodes tested: " << (int)testedNodes << "\n";
			LogObject() << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
			LogObject() << "Path steps: " << (int)(path.path.size()) << "\n";
			LogObject() << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			LogObject() << "Path not found!\n";
			LogObject() << "Nodes tested: " << (int)testedNodes << "\n";
			LogObject() << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
		}
	}
	return result;
}

/**
 * Store som data and doing some basic top-administration.
 */
IPath::SearchResult CPathFinder::GetPath(const MoveData& moveData, const float3 startPos,
		const CPathFinderDef& pfDef, Path& path, bool testMobile, bool exactPath,
		unsigned int maxNodes, bool needPath, int ownerId) {
	// Clear the given path.
	path.path.clear();
	path.squares.clear();
	path.pathCost = PATHCOST_INFINITY;

	// Store som basic data.
	maxNodesToBeSearched = std::min((unsigned int)MAX_SEARCHED_SQUARES, maxNodes);
	this->testMobile=testMobile;
	this->exactPath = exactPath;
	this->needPath=needPath;
	start = startPos;
	startxSqr = (int(start.x) / SQUARE_SIZE)|1;
	startzSqr = (int(start.z) / SQUARE_SIZE)|1;

	// Clamp the start position
	if (startxSqr < 0) startxSqr=0;
	if (startxSqr >= gs->mapx) startxSqr = gs->mapx-1;
	if (startzSqr < 0) startzSqr =0;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapy-1;

	startSquare = startxSqr + startzSqr * gs->mapx;

	// Start up the search.
	SearchResult result = InitSearch(moveData, pfDef, ownerId);

	// Respond to the success of the search.
	if(result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
		if(PATHDEBUG) {
			LogObject() << "Path found.\n";
			LogObject() << "Nodes tested: " << (int)testedNodes << "\n";
			LogObject() << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
			LogObject() << "Path steps: " << (int)(path.path.size()) << "\n";
			LogObject() << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			LogObject() << "Path not found!\n";
			LogObject() << "Nodes tested: " << (int)testedNodes << "\n";
			LogObject() << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
		}
	}
	return result;
}


/**
 * Setting up the starting point of the search.
 */
IPath::SearchResult CPathFinder::InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef,
		int ownerId) {
	// If exact path is reqired and the goal is blocked, then no search is needed.
	if (exactPath && pfDef.GoalIsBlocked(moveData, (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN)))
		return CantGetCloser;

	// Clamp the start position
	if (startxSqr < 0) startxSqr = 0;
	if (startxSqr >= gs->mapx) startxSqr = gs->mapx - 1;
	if (startzSqr < 0) startzSqr = 0;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapy - 1;

	// If the starting position is a goal position, then no search need to be performed.
	if(pfDef.IsGoal(startxSqr, startzSqr))
		return CantGetCloser;

	// Clear the system from last search.
	ResetSearch();

	// Marks and store the start-square.
	squareState[startSquare].status = (PATHOPT_START | PATHOPT_OPEN);
	squareState[startSquare].cost = 0;
	dirtySquares.push_back(startSquare);

	// Make the beginning the fest square found.
	goalSquare = startSquare;
	goalHeuristic = pfDef.Heuristic(startxSqr, startzSqr);

	// Adding the start-square to the queue.
	openSquareBufferPointer = &openSquareBuffer[0];
	OpenSquare *os = openSquareBufferPointer; // Taking first OpenSquare in buffer.
	os->currentCost = 0;
	os->cost = 0;
	os->square.x = startxSqr;
	os->square.y = startzSqr;
	os->sqr = startSquare;
	openSquares.push(os);

	// Performs the search.
	SearchResult result = DoSearch(moveData, pfDef, ownerId);

	// If no improvement has been found then return CantGetCloser instead.
	if(goalSquare == startSquare || goalSquare == 0) {
		return CantGetCloser;
	} else
		return result;
}


/**
 * Performs the actual search.
 */
IPath::SearchResult CPathFinder::DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef,
		int ownerId) {
	bool foundGoal = false;
	while (!openSquares.empty() && openSquareBufferPointer - openSquareBuffer < (maxNodesToBeSearched - 8)) {
		// Get the open square with lowest expected path-cost.
		OpenSquare* os = (OpenSquare*) openSquares.top();
		openSquares.pop();

		// Check if this OpenSquare-holder have become obsolete.
		if (squareState[os->sqr].cost != os->cost)
			continue;

		// Check if the goal is reached.
		if (pfDef.IsGoal(os->square.x, os->square.y)) {
			goalSquare = os->sqr;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		// Test the 8 surrounding squares.
		bool right = TestSquare(moveData, pfDef, os, PATHOPT_RIGHT, ownerId);
		bool left = TestSquare(moveData, pfDef, os, PATHOPT_LEFT, ownerId);
		bool up = TestSquare(moveData, pfDef, os, PATHOPT_UP, ownerId);
		bool down = TestSquare(moveData, pfDef, os, PATHOPT_DOWN, ownerId);

		if (up) {
			// we dont want to search diagonally if there is a blocking object
			// (not blocking terrain) in one of the two side squares
			if (right)
				TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_UP), ownerId);
			if (left)
				TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_UP), ownerId);
		}
		if (down) {
			if (right)
				TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_DOWN), ownerId);
			if (left)
				TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_DOWN), ownerId);
		}

		// Mark this square as closed.
		squareState[os->sqr].status |= PATHOPT_CLOSED;
	}

	// Returning search-result.
	if(foundGoal)
		return Ok;

	// Could not reach the goal.
	if(openSquareBufferPointer - openSquareBuffer >= (maxNodesToBeSearched - 8))
		return GoalOutOfRange;

	// Search could not reach the goal, due to the unit being locked in.
	if(openSquares.empty())
		return GoalOutOfRange;

	// Below shall never be runned.
	LogObject() << "ERROR: CPathFinder::DoSearch() - Unhandled end of search!\n";
	return Error;
}


/**
 * Test the availability and value of a square,
 * and possibly add it to the queue of open squares.
 */
bool CPathFinder::TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef,
		OpenSquare* parentOpenSquare, unsigned int enterDirection, int ownerId) {
	testedNodes++;

	// Calculate the new square.
	int2 square;
	square.x = parentOpenSquare->square.x + directionVector[enterDirection].x;
	square.y = parentOpenSquare->square.y + directionVector[enterDirection].y;

	// Inside map?
	if (square.x < 0 || square.y < 0 || square.x >= gs->mapx || square.y >= gs->mapy) {
		return false;
	}

	int sqr = square.x + square.y * gs->mapx;
	int sqrStatus = squareState[sqr].status;

	// Check if the square is unaccessable or used.
	if (sqrStatus & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED)) {
		return false;
	}

	int blockStatus = moveData.moveMath->IsBlocked2(moveData, square.x, square.y);
	int blockBits = (CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN);

	// Check if square are out of constraints or blocked by something.
	// Doesn't need to be done on open squares, as those are already tested.
	if ((!pfDef.WithinConstraints(square.x, square.y) || (blockStatus & blockBits)) &&
		!(sqrStatus & PATHOPT_OPEN)) {

		squareState[sqr].status |= PATHOPT_BLOCKED;
		dirtySquares.push_back(sqr);
		return false;
	}

	// Evaluate this square.
	float squareSpeedMod = moveData.moveMath->SpeedMod(moveData, square.x, square.y);
	blockBits = (CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOVING | CMoveMath::BLOCK_MOBILE_BUSY);

	if (squareSpeedMod == 0) {
		squareState[sqr].status |= PATHOPT_FORBIDDEN;
		dirtySquares.push_back(sqr);
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
		heatCostMod += moveData.heatMod * GetHeatValue(square.x,square.y);
	}

	float squareCost = heatCostMod * moveCost[enterDirection] / squareSpeedMod;
	float heuristicCost = pfDef.Heuristic(square.x, square.y);

	// Summarize cost.
	float currentCost = parentOpenSquare->currentCost + squareCost;
	float cost = currentCost + heuristicCost;

	// Checks if this square is in open queue already.
	// If the old one is better then keep it, else change it.
	if (squareState[sqr].status & PATHOPT_OPEN) {
		if (squareState[sqr].cost <= cost)
			return true;
		squareState[sqr].status &= ~PATHOPT_DIRECTION;
	}

	// Look for improvements.
	if (!exactPath && heuristicCost < goalHeuristic) {
		goalSquare = sqr;
		goalHeuristic = heuristicCost;
	}

	// Store this square as open.
	OpenSquare* os = ++openSquareBufferPointer;		//Take the next OpenSquare in buffer.
	os->square = square;
	os->sqr = sqr;
	os->currentCost = currentCost;
	os->cost = cost;
	openSquares.push(os);

	// Set this one as open and the direction from which it was reached.
	squareState[sqr].cost = os->cost;
	squareState[sqr].status |= (PATHOPT_OPEN | enterDirection);
	dirtySquares.push_back(sqr);
	return true;
}



/**
 * Recreates the path found by pathfinder.
 * Starting at goalSquare and tracking backwards.
 *
 * Perform adjustment of waypoints so not all turns are 90 or 45 degrees.
 */
void CPathFinder::FinishSearch(const MoveData& moveData, Path& foundPath) {
	// Backtracking the path.
	if(needPath) {
		int2 square;
		square.x = goalSquare % gs->mapx;
		square.y = goalSquare / gs->mapx;

		// for path adjustment (cutting corners)
		std::deque<int2> previous;

		// make sure we don't match anything
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));
		previous.push_back(int2(-100, -100));

		do {
			int sqr = square.x + square.y * gs->mapx;
			if(squareState[sqr].status & PATHOPT_START)
				break;
			float3 cs;
			cs.x = (square.x/2/* + 0.5f*/) * SQUARE_SIZE*2+SQUARE_SIZE;
			cs.z = (square.y/2/* + 0.5f*/) * SQUARE_SIZE*2+SQUARE_SIZE;
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
			square.x -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].x;
			square.y -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].y;
		} while(true);

		if (foundPath.path.size() > 0) {
			foundPath.pathGoal = foundPath.path.front();
		}
	}
	// Adds the cost of the path.
	foundPath.pathCost = squareState[goalSquare].cost;
}

/** Helper function for AdjustFoundPath */
static inline void FixupPath3Pts(const MoveData& moveData, float3& p1, float3& p2, float3& p3, int2 testsquare)
{
	float3 old = p2;
	old.y += 10;
	p2.x = 0.5f * (p1.x + p3.x);
	p2.z = 0.5f * (p1.z + p3.z);
	p2.y = moveData.moveMath->yLevel(testsquare.x, testsquare.y);
#if PATHDEBUG
	geometricObjects->AddLine(p3+float3(0, 5, 0), p2+float3(0, 10, 0), 5, 10, 600, 0);
	geometricObjects->AddLine(p3+float3(0, 5, 0), old, 5, 10, 600, 0);
#endif
}


/**
 * Adjusts the found path to cut corners where possible.
 */
void CPathFinder::AdjustFoundPath(const MoveData& moveData, Path& foundPath, float3& nextPoint,
	std::deque<int2>& previous, int2 square)
{
#define COSTMOD 1.39f	// (sqrt(2) + 1)/sqrt(3)
#define TRYFIX3POINTS(dxtest, dytest) \
	do { \
		int testsqr = square.x + (dxtest) + (square.y + (dytest)) * gs->mapx; \
		int p2sqr = previous[2].x + previous[2].y * gs->mapx; \
		if (!(squareState[testsqr].status & (PATHOPT_BLOCKED | PATHOPT_FORBIDDEN)) \
				&& squareState[testsqr].cost <= (COSTMOD) * squareState[p2sqr].cost) { \
			float3& p2 = foundPath.path[foundPath.path.size() - 2]; \
			float3& p1 = foundPath.path.back(); \
			float3& p0 = nextPoint; \
			FixupPath3Pts(moveData, p0, p1, p2, int2(square.x + (dxtest), square.y + (dytest))); \
		} \
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
	openSquares.DeleteAll();
//	while(!openSquares.empty())
//		openSquares.pop();
	while(!dirtySquares.empty()){
		int lsquare = dirtySquares.back();
		squareState[lsquare].status = 0;
		squareState[lsquare].cost = PATHCOST_INFINITY;
		dirtySquares.pop_back();
	}
	testedNodes = 0;
}


/////////////////
// heat mapping

void CPathFinder::SetHeatMapState(bool enabled)
{
	heatMapping = enabled;
}

void CPathFinder::InitHeatMap()
{
	heatmap.resize(gs->hmapx);
	for (int i = 0; i<gs->hmapx; ++i) {
		heatmap[i].resize(gs->hmapy, HeatMapValue());
	}
	heatMapOffset = 0;
}


void CPathFinder::UpdateHeatMap()
{
	++heatMapOffset;
}


////////////////////
// CPathFinderDef //
////////////////////

CPathFinderDef::CPathFinderDef(float3 goalCenter, float goalRadius) :
goal(goalCenter),
sqGoalRadius(goalRadius)
{
	// Makes sure that the goal could be reached with 2-square resolution.
	if(sqGoalRadius < SQUARE_SIZE*SQUARE_SIZE*2)
		sqGoalRadius = SQUARE_SIZE*SQUARE_SIZE*2;
	goalSquareX=(int)(goalCenter.x/SQUARE_SIZE);
	goalSquareZ=(int)(goalCenter.z/SQUARE_SIZE);
}


/**
 * Tells whenever the goal is in range.
 */
bool CPathFinderDef::IsGoal(int xSquare, int zSquare) const {
	return ((SquareToFloat3(xSquare, zSquare)-goal).SqLength2D() <= sqGoalRadius);
}


/**
 * Distance to goal center in mapsquares.
 */
float CPathFinderDef::Heuristic(int xSquare, int zSquare) const
{
	int min=abs(xSquare-goalSquareX);
	int max=abs(zSquare-goalSquareZ);
	if(min>max){
		int temp=min;
		min=max;
		max=temp;
	}
	return max*0.5f+min*0.2f;
}


/**
 * Tells if the goal are is unaccessable.
 * If the goal area is small and blocked then it's considered blocked, else not.
 */
bool CPathFinderDef::GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const {
	const float r0 = SQUARE_SIZE * SQUARE_SIZE * 4;
	const float r1 = (moveData.size / 2) * (moveData.size / 2) * 1.5f * SQUARE_SIZE * SQUARE_SIZE;

	return
		((sqGoalRadius < r0 || sqGoalRadius <= r1) &&
		(moveData.moveMath->IsBlocked(moveData, goal) & moveMathOptions));
}

int2 CPathFinderDef::GoalSquareOffset(int blockSize) const {
	int blockPixelSize = blockSize * SQUARE_SIZE;
	int2 offset;
	offset.x = ((int) goal.x % blockPixelSize) / SQUARE_SIZE;
	offset.y = ((int) goal.z % blockPixelSize) / SQUARE_SIZE;
	return offset;
}



//////////////////////////////////////////
// CRangedGoalWithCircularConstraintPFD //
//////////////////////////////////////////

/**
 * Constructor
 * Calculating the center and radius of the constrainted area.
 */
CRangedGoalWithCircularConstraint::CRangedGoalWithCircularConstraint(float3 start, float3 goal, float goalRadius,float searchSize,int extraSize) :
CPathFinderDef(goal, goalRadius)
{
	int startX=(int)(start.x/SQUARE_SIZE);
	int startZ=(int)(start.z/SQUARE_SIZE);
	float3 halfWay = (start + goal) / 2;

	halfWayX = (int)(halfWay.x/SQUARE_SIZE);
	halfWayZ = (int)(halfWay.z/SQUARE_SIZE);

	int dx=startX-halfWayX;
	int dz=startZ-halfWayZ;

	searchRadiusSq=dx*dx+dz*dz;
	searchRadiusSq*=(int)(searchSize*searchSize);
	searchRadiusSq+=extraSize;
}


/**
 * Tests if a square is inside is the circular constrainted area.
 */
bool CRangedGoalWithCircularConstraint::WithinConstraints(int xSquare, int zSquare) const
{
	int dx = halfWayX - xSquare;
	int dz = halfWayZ - zSquare;

	return ((dx * dx + dz * dz) <= searchRadiusSq);
}

void CPathFinder::myPQ::DeleteAll()
{
//	while(!c.empty())
//		c.pop_back();
	c.clear();
//	c.reserve(1000);
}

CPathFinderDef::~CPathFinderDef() {
}
CRangedGoalWithCircularConstraint::~CRangedGoalWithCircularConstraint() {
}
