#include "StdAfx.h"
#include "PathFinder.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Map/ReadMap.h"
#include "LogOutput.h"
#include "Rendering/GL/glExtra.h"
#include <ostream>
#include "Sim/MoveTypes/MoveInfo.h"
#include "Map/Ground.h"
#include "mmgr.h"

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
const float PATHCOST_INFINITY = 10000000;

void* pfAlloc(size_t n)
{
	char* ret=SAFE_NEW char[n];
	for(int a=0;a<n;++a)
		ret[a]=0;

	return ret;
}

void pfDealloc(void *p,size_t n)
{
	delete[] ((char*)p);
}

/**
Constructor.
Building tables and precalculating data.
*/
CPathFinder::CPathFinder()
: openSquareBufferPointer(openSquareBuffer)
{
	//Creates and init all square states.
	squareState = SAFE_NEW SquareState[gs->mapSquares];
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
	moveCost[(PATHOPT_RIGHT | PATHOPT_UP)] = 1.42f;
	moveCost[(PATHOPT_LEFT | PATHOPT_UP)] = 1.42f;
	moveCost[(PATHOPT_RIGHT | PATHOPT_DOWN)] = 1.42f;
	moveCost[(PATHOPT_LEFT | PATHOPT_DOWN)] = 1.42f;
}


/**
Destructor.
Free used memory.
*/
CPathFinder::~CPathFinder()
{
	ResetSearch();
	delete[] squareState;
}


/**
Search with several start positions
*/
IPath::SearchResult CPathFinder::GetPath(const MoveData& moveData, const std::vector<float3>& startPos, const CPathFinderDef& pfDef, Path& path) {
	// Clear the given path.
	path.path.clear();
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

	//Clearing the system from last search.
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

		OpenSquare *os = ++openSquareBufferPointer;	//Taking first OpenSquare in buffer.
		os->currentCost = 0;
		os->cost = 0;
		os->square.x = startxSqr;
		os->square.y = startzSqr;
		os->sqr = startSquare;
		openSquares.push(os);
	}

	//Performs the search.
	SearchResult result = DoSearch(moveData, pfDef);

	//Respond to the success of the search.
	if(result == Ok) {
		FinishSearch(moveData, path);
		if(PATHDEBUG) {
			logOutput << "Path found.\n";
			logOutput << "Nodes tested: " << (int)testedNodes << "\n";
			logOutput << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
			logOutput << "Path steps: " << (int)(path.path.size()) << "\n";
			logOutput << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			logOutput << "Path not found!\n";
			logOutput << "Nodes tested: " << (int)testedNodes << "\n";
			logOutput << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
		}
	}
	return result;
}

/**
Store som data and doing some basic top-administration.
*/
IPath::SearchResult CPathFinder::GetPath(const MoveData& moveData, const float3 startPos, const CPathFinderDef& pfDef, Path& path, bool testMobile, bool exactPath, unsigned int maxNodes,bool needPath) {
	//Clear the given path.
	path.path.clear();
	path.pathCost = PATHCOST_INFINITY;

	//Store som basic data.
	maxNodesToBeSearched = std::min((unsigned int)MAX_SEARCHED_SQUARES, maxNodes);
	this->testMobile=testMobile;
	this->exactPath = exactPath;
	this->needPath=needPath;
	start = startPos;
	startxSqr = (int(start.x) / SQUARE_SIZE)|1;
	startzSqr = (int(start.z) / SQUARE_SIZE)|1;

	//Clamp the start position
	if (startxSqr < 0) startxSqr=0;
	if (startxSqr >= gs->mapx) startxSqr = gs->mapx-1;
	if (startzSqr < 0) startzSqr =0;
	if (startzSqr >= gs->mapy) startzSqr = gs->mapy-1;

	startSquare = startxSqr + startzSqr * gs->mapx;

	//Start up the search.
	SearchResult result = InitSearch(moveData, pfDef);

	//Respond to the success of the search.
	if(result == Ok || result == GoalOutOfRange) {
		FinishSearch(moveData, path);
		if(PATHDEBUG) {
			logOutput << "Path found.\n";
			logOutput << "Nodes tested: " << (int)testedNodes << "\n";
			logOutput << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
			logOutput << "Path steps: " << (int)(path.path.size()) << "\n";
			logOutput << "Path cost: " << path.pathCost << "\n";
		}
	} else {
		if(PATHDEBUG) {
			logOutput << "Path not found!\n";
			logOutput << "Nodes tested: " << (int)testedNodes << "\n";
			logOutput << "Open squares: " << (float)(openSquareBufferPointer - openSquareBuffer) << "\n";
		}
	}
	return result;
}


/**
Setting up the starting point of the search.
*/
IPath::SearchResult CPathFinder::InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef) {
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


/**
Performs the actual search.
*/
IPath::SearchResult CPathFinder::DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef) {
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
		bool right = TestSquare(moveData, pfDef, os, PATHOPT_RIGHT);
		bool left = TestSquare(moveData, pfDef, os, PATHOPT_LEFT);
		bool up = TestSquare(moveData, pfDef, os, PATHOPT_UP);
		bool down = TestSquare(moveData, pfDef, os, PATHOPT_DOWN);

		if (up) {
			// we dont want to search diagonally if there is a blocking object
			// (not blocking terrain) in one of the two side squares
			if (right)
				TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_UP));
			if (left)
				TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_UP));
		}
		if (down) {
			if (right)
				TestSquare(moveData, pfDef, os, (PATHOPT_RIGHT | PATHOPT_DOWN));
			if (left)
				TestSquare(moveData, pfDef, os, (PATHOPT_LEFT | PATHOPT_DOWN));
		}

		// Mark this square as closed.
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
	logOutput << "ERROR: CPathFinder::DoSearch() - Unhandled end of search!\n";
	return Error;
}


/**
Test the availability and value of a square,
and possibly add it to the queue of open squares.
*/
bool CPathFinder::TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef, OpenSquare* parentOpenSquare, unsigned int enterDirection) {
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

	float squareCost = moveCost[enterDirection] / squareSpeedMod;
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
Recreates the path found by pathfinder.
Starting at goalSquare and tracking backwards.
*/
void CPathFinder::FinishSearch(const MoveData& moveData, Path& foundPath) {
	//Backtracking the path.
	if(needPath)
		{
		int2 square;
		square.x = goalSquare % gs->mapx;
		square.y = goalSquare / gs->mapx;
		do {
			int sqr = square.x + square.y * gs->mapx;
			if(squareState[sqr].status & PATHOPT_START)
				break;
			float3 cs;
				cs.x = (square.x/2/* + 0.5f*/) * SQUARE_SIZE*2+SQUARE_SIZE;
				cs.z = (square.y/2/* + 0.5f*/) * SQUARE_SIZE*2+SQUARE_SIZE;
			cs.y = moveData.moveMath->yLevel(square.x, square.y);
			foundPath.path.push_back(cs);
			int2 oldSquare;
			oldSquare.x = square.x;
			oldSquare.y = square.y;
			square.x -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].x;
			square.y -= directionVector[squareState[sqr].status & PATHOPT_DIRECTION].y;
			} 
		while(true);
		if (foundPath.path.size() > 0)
			{
			foundPath.pathGoal = foundPath.path.front();
			}
		}
	//Adds the cost of the path.
	foundPath.pathCost = squareState[goalSquare].cost;
}


/**
Clear things up from last search.
*/
void CPathFinder::ResetSearch() {
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



////////////////////
// CPathFinderDef //
////////////////////

/**
Constructor.
*/
CPathFinderDef::CPathFinderDef(float3 goalCenter, float goalRadius) :
goal(goalCenter),
sqGoalRadius(goalRadius)
{
	//Makes sure that the goal could be reached with 2-square resolution.
	if(sqGoalRadius < SQUARE_SIZE*SQUARE_SIZE*2)
		sqGoalRadius = SQUARE_SIZE*SQUARE_SIZE*2;
	goalSquareX=(int)(goalCenter.x/SQUARE_SIZE);
	goalSquareZ=(int)(goalCenter.z/SQUARE_SIZE);
}


/**
Tells whenever the goal is in range.
*/
bool CPathFinderDef::IsGoal(int xSquare, int zSquare) const {
	return ((SquareToFloat3(xSquare, zSquare)-goal).SqLength2D() <= sqGoalRadius);
}


/**
Distance to goal center in mapsquares.
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
Tells if the goal are is unaccessable.
If the goal area is small and blocked then it's considered blocked, else not.
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

/**
Draw a circle around the goal, indicating the goal area.
*/
void CPathFinderDef::Draw() const {
	glColor4f(0, 1, 1, 1);
	glSurfaceCircle(goal, sqrt(sqGoalRadius), 20);
}

void CPathFinder::Draw(void)
{
	glColor3f(0.7f,0.2f,0.2f);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	for(OpenSquare* os=openSquareBuffer;os!=openSquareBufferPointer;++os){
		int2 sqr=os->square;
		int square = os->sqr;
		if(squareState[square].status & PATHOPT_START)
			continue;
		float3 p1;
		p1.x=sqr.x*SQUARE_SIZE;
		p1.z=sqr.y*SQUARE_SIZE;
		p1.y=ground->GetHeight(p1.x,p1.z)+15;
		float3 p2;
		int obx=sqr.x-directionVector[squareState[square].status & PATHOPT_DIRECTION].x;
		int obz=sqr.y-directionVector[squareState[square].status & PATHOPT_DIRECTION].y;
		int obsquare =  obz * gs->mapx + obx;

		if(obsquare>=0){
			p2.x=obx*SQUARE_SIZE;
			p2.z=obz*SQUARE_SIZE;
			p2.y=ground->GetHeight(p2.x,p2.z)+15;

			glVertexf3(p1);
			glVertexf3(p2);
		}
	}
	glEnd();
}

//////////////////////////////////////////
// CRangedGoalWithCircularConstraintPFD //
//////////////////////////////////////////

/**
Constructor
Calculating the center and radius of the constrainted area.
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
Tests if a square is inside is the circular constrainted area.
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
