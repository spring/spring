#include "PathFinder.h"
#include "MoveMath.h"
#include "ReadMap.h"
#include "InfoConsole.h"
#include "glExtra.h"
#include <ostream>
#include "MoveInfo.h"

#define PATHDEBUG false


//Option constants.
const unsigned int PATHOPT_RIGHT = 1;		//-x
const unsigned int PATHOPT_LEFT = 2;		//+x
const unsigned int PATHOPT_UP = 4;			//+z
const unsigned int PATHOPT_DOWN = 8;		//-z
const unsigned int PATHOPT_DIRECTION = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN);
const unsigned int PATHOPT_START = 16;
const unsigned int PATHOPT_OPEN = 32;
const unsigned int PATHOPT_CLOSED = 64;
const unsigned int PATHOPT_FORBIDDEN = 128;
const unsigned int PATHOPT_BLOCKED = 256;

//Cost constants.
const float PATHCOST_INFINITY = 1e9;

void* pfAlloc(size_t n)
{
	char* ret=new char[n];
	for(int a=0;a<n;++a)
		ret[a]=0;

	return ret;
}

void pfDealloc(void *p,size_t n)
{
	delete[] p;
}

const unsigned int CPathFinder::MAX_SEARCHED_SQARES;

/*
Constructor.
Building tables and precalculating data.
*/
CPathFinder::CPathFinder() 
: openSquareBufferPointer(openSquareBuffer)
{
	//Creates and init all square states.
	squareState = new SquareState[gs->mapSquares];
	for(int a = 0; a < gs->mapSquares; ++a){
		squareState[a].status = 0;
		squareState[a].cost = PATHCOST_INFINITY;
	}
	for(int a=0;a<MAX_SEARCHED_SQARES;++a){
		openSquareBuffer[a].cost=0;
		openSquareBuffer[a].currentCost=0;
		openSquareBuffer[a].sqr=0;
		openSquareBuffer[a].square.x=0;
		openSquareBuffer[a].square.y=0;
	}

	//Create border-constraints all around the map.
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

	//Precalculated vectors.
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
	moveCost[(PATHOPT_RIGHT | PATHOPT_UP)] = 1.42;
	moveCost[(PATHOPT_LEFT | PATHOPT_UP)] = 1.42;
	moveCost[(PATHOPT_RIGHT | PATHOPT_DOWN)] = 1.42;
	moveCost[(PATHOPT_LEFT | PATHOPT_DOWN)] = 1.42;
}


/*
Destructor.
Free used memory.
*/
CPathFinder::~CPathFinder()
{
	ResetSearch();
	delete[] squareState;
}


/*
Store som data and doing some basic top-administration.
*/
IPath::SearchResult CPathFinder::GetPath(const MoveData& moveData, const float3 startPos, const CPathFinderDef& pfDef, Path& path, unsigned int moveMathOpt, bool exactPath, unsigned int maxNodes) {
	//Clear the given path.
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	//Store som basic data.
	maxNodesToBeSearched = min(MAX_SEARCHED_SQARES, maxNodes);
	blockOpt = moveMathOpt;
	this->exactPath = exactPath;
	start = startPos;
	startxSqr = int(start.x+SQUARE_SIZE/2) / SQUARE_SIZE;
	startzSqr = int(start.z+SQUARE_SIZE/2) / SQUARE_SIZE;
	startSquare = startxSqr + startzSqr * gs->mapx;

	//Start up the search.
	SearchResult result = InitSearch(moveData, pfDef);

	//Respond to the success of the search.
	if(result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
		if(PATHDEBUG) {
			*info << "Path found.\n";
			*info << "Nodes tested: " << (int)testedNodes << "\n";
			*info << "Open squares: " << (openSquareBufferPointer - openSquareBuffer) << "\n";
			*info << "Path steps: " << (int)(path.path.size()) << "\n";
			*info << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			*info << "Path not found!\n";
			*info << "Nodes tested: " << (int)testedNodes << "\n";
			*info << "Open squares: " << (openSquareBufferPointer - openSquareBuffer) << "\n";
		}
	}
	return result;
}


/*
Setting up the starting point of the search.
*/
IPath::SearchResult CPathFinder::InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef) {
	//If exact path is reqired and the goal is blocked, then no search is needed.
	if(exactPath && pfDef.GoalIsBlocked(moveData, blockOpt))
		return CantGetCloser;
	
	//If the starting position is a goal position, then no search need to be performed.
	if(pfDef.IsGoal(startxSqr, startzSqr))
		return CantGetCloser;
	
	//Clearing the system from last search.
	ResetSearch();

	//Marks and store the start-square.
	squareState[startSquare].status = (PATHOPT_START | PATHOPT_OPEN);
	squareState[startSquare].cost = 0;
	dirtySquares.push_back(startSquare);

	//Make the beginning the fest square found.
	goalSquare = startSquare;
	goalHeuristic = pfDef.Heuristic(startxSqr, startzSqr);

	//Adding the start-square to the queue.
	openSquareBufferPointer = &openSquareBuffer[0];
	OpenSquare *os = openSquareBufferPointer;	//Taking first OpenSquare in buffer.
	os->currentCost = 0;
	os->cost = 0;
	os->square.x = startxSqr;
	os->square.y = startzSqr;
	os->sqr = startSquare;
	openSquares.push(os);

	//Performs the search.
	SearchResult result = DoSearch(moveData, pfDef);
	
	//If no improvement has been found then return CantGetCloser instead.
	if(goalSquare == startSquare || goalSquare == 0) {
		return CantGetCloser;
	} else
		return result;
}


/*
Performs the actual search.
*/
IPath::SearchResult CPathFinder::DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef) {
	bool foundGoal = false;
	while(!openSquares.empty()
	&& openSquareBufferPointer - openSquareBuffer < (maxNodesToBeSearched - 8)){
		//Gets the open square with lowest expected path-cost.
		OpenSquare* os = openSquares.top();
		openSquares.pop();

		//Check if the square have been marked not to be used during it's time in que.
		if(squareState[os->sqr].status & (PATHOPT_FORBIDDEN | PATHOPT_CLOSED | PATHOPT_BLOCKED)) {
			continue;
		}

		//Check if this OpenSquare-holder have become obsolete.
		if(squareState[os->sqr].cost != os->cost)
			continue;

		//Check if the goal is reached.
		if(pfDef.IsGoal(os->square.x, os->square.y)) {
			goalSquare = os->sqr;
			goalHeuristic = 0;
			foundGoal = true;
			break;
		}

		//Testing the 8 surrounding squares.
		TestSquare(moveData, pfDef, os, PATHOPT_RIGHT);
		TestSquare(moveData, pfDef, os, PATHOPT_LEFT);
		TestSquare(moveData, pfDef, os, PATHOPT_UP);
		TestSquare(moveData, pfDef, os, PATHOPT_DOWN);
		TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_UP));
		TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_UP));
		TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_DOWN));
		TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_DOWN));

		//Mark this square as closed.
		squareState[os->sqr].status |= PATHOPT_CLOSED;
	}

	//Returning search-result.
	if(foundGoal)
		return Ok;

	//Could not reach the goal.
	if(openSquareBufferPointer - openSquareBuffer >= (maxNodesToBeSearched - 8))
		return GoalOutOfRange;

	//Search could not reach the goal, due to the unit being locked in.
	if(openSquares.empty())
		return GoalOutOfRange;

	//Below shall never be runned.
	*info << "ERROR: CPathFinder::DoSearch() - Unhandled end of search!\n";
	return Error;
}


/*
Test the availability and value of a square,
and possibly add it to the queue of open squares.
*/
void CPathFinder::TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef, OpenSquare* parentOpenSquare, unsigned int enterDirection) {
	testedNodes++;
	
	//Calculate the new square.
	int2 square;
	square.x = parentOpenSquare->square.x + directionVector[enterDirection].x;
	square.y = parentOpenSquare->square.y + directionVector[enterDirection].y;
	
	//Inside map?
	if(square.x < 0 || square.y < 0
	|| square.x >= gs->mapx || square.y >= gs->mapy)
		return;
	int sqr = square.x + square.y * gs->mapx;

	//Check if the square is unaccessable or used.
	if(squareState[sqr].status & (PATHOPT_CLOSED | PATHOPT_FORBIDDEN | PATHOPT_BLOCKED))
		return;

	//Check if square are out of constraints or blocked by something.
	//Don't need to be done on open squares, as whose are already tested.
	if(!(squareState[sqr].status & PATHOPT_OPEN) &&
	(!pfDef.WithinConstraints(square.x, square.y) || moveData.moveMath->IsBlocked(moveData, blockOpt, square.x, square.y))) {
		squareState[sqr].status |= PATHOPT_BLOCKED;
		dirtySquares.push_back(sqr);
		return;
	}

	//Evaluate this node.
	float squareSpeedMod = moveData.moveMath->SpeedMod(moveData, square.x, square.y);
	float squareCost = (float)moveCost[enterDirection] / squareSpeedMod;
	float heuristicCost = pfDef.Heuristic(square.x, square.y);

	//Adds a punishment for turning.
	if(enterDirection != (squareState[parentOpenSquare->sqr].status & PATHOPT_DIRECTION))
		squareCost *= 1.4f;

	//Summarize cost.
	float currentCost = parentOpenSquare->currentCost + squareCost;
	float cost = currentCost + heuristicCost;

	//Checks if this square are in que already.
	//If the old one is better then keep it, else change it.
	if(squareState[sqr].status & PATHOPT_OPEN) {
		if(squareState[sqr].cost <= cost)
			return;
		squareState[sqr].status &= ~PATHOPT_DIRECTION;
	}

	//Looking for improvements.
	if(!exactPath
	&& heuristicCost + moveCost[enterDirection]/2 < goalHeuristic) {
		goalSquare = sqr;
		goalHeuristic = heuristicCost;
	}

	//Store this square as open.
	OpenSquare *os = ++openSquareBufferPointer;		//Take the next OpenSquare in buffer.
	os->square = square;
	os->sqr = sqr;
	os->currentCost = currentCost;
	os->cost = cost;
	openSquares.push(os);

	//Set this one as open and the direction from which it was reached.
	squareState[sqr].cost = os->cost;
	squareState[sqr].status |= (PATHOPT_OPEN | enterDirection);
	dirtySquares.push_back(sqr);
}


/*
Recreates the path found by pathfinder.
Starting at goalSquare and tracking backwards.
*/
void CPathFinder::FinishSearch(const MoveData& moveData, Path& foundPath) {
	//Backtracking the path.
	int2 square;
	square.x = goalSquare % gs->mapx;
	square.y = goalSquare / gs->mapx;
	do {
		int sqr = square.x + square.y * gs->mapx;
		if(squareState[sqr].status & PATHOPT_START)
			break;
		float3 cs;
		cs.x = (square.x/* + 0.5*/) * SQUARE_SIZE;
		cs.z = (square.y/* + 0.5*/) * SQUARE_SIZE;
		cs.y = moveData.moveMath->yLevel(square.x, square.y);
		foundPath.path.push_back(cs);
		int2 oldSquare;
		oldSquare.x = square.x;
		oldSquare.y = square.y;
		square.x -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].x;
		square.y -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].y;
	} while(true);
	
	//Adds the cost of the path.
	foundPath.pathCost = squareState[goalSquare].cost - goalHeuristic;
	foundPath.pathGoal = foundPath.path.front();
}


/*
Clear things up from last search.
*/
void CPathFinder::ResetSearch() {
	while(!openSquares.empty())
		openSquares.pop();
	while(!dirtySquares.empty()){
		squareState[dirtySquares.back()].status = 0;
		squareState[dirtySquares.back()].cost = PATHCOST_INFINITY;
		dirtySquares.pop_back();
	}
	testedNodes = 0;
}



////////////////////
// CRangedGoalPFD //
////////////////////

/*
Constructor.
*/
CRangedGoalPFD::CRangedGoalPFD(float3 goalCenter, float goalRadius) :
goal(goalCenter),
goalRadius(goalRadius)
{
	//Makes sure that the goal could be reached with 2-square resolution.
	if(this->goalRadius < SQUARE_SIZE)
		this->goalRadius = SQUARE_SIZE;
}


/*
Tells whenever the goal is in range.
*/
bool CRangedGoalPFD::IsGoal(int xSquare, int zSquare) const {
	 return (SquareToFloat3(xSquare, zSquare).distance2D(goal) <= goalRadius);
}


/*
Distance to goal center in mapsquares.
*/
float CRangedGoalPFD::Heuristic(int xSquare, int zSquare) const {
	return (SquareToFloat3(xSquare, zSquare).distance2D(goal) / SQUARE_SIZE);
}


/*
Tells if the goal are is unaccessable.
If the goal area is small and blocked then it's considered blocked, else not.
*/
bool CRangedGoalPFD::GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const {
	if((goalRadius < SQUARE_SIZE*4 || goalRadius <= (moveData.size/2)*1.5*SQUARE_SIZE)
	&& moveData.moveMath->IsBlocked(moveData, moveMathOptions, goal))
		return true;
	else
		return false;
}


/*
Draw a circle around the goal, indicating the goal area.
*/
void CRangedGoalPFD::Draw() const {
	glColor4f(0, 1, 1, 1);
	glSurfaceCircle(goal, goalRadius);
}

