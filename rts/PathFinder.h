#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "IPath.h"
#include "ReadMap.h"
#include "MoveMath.h"
#include <queue>
#include <list>
using namespace std;

class CPathFinderDef;

void* pfAlloc(size_t n);
void pfDealloc(void *p,size_t n);

class CPathFinder : public IPath {
public:
	CPathFinder();
	virtual ~CPathFinder();

	void* operator new(size_t size){return pfAlloc(size);};
	inline void operator delete(void* p,size_t size){pfDealloc(p,size);};

	/*
	Gives a detailed path from given starting location to target defined in CPathFinderDef,
	whenever any such are available.
	If no complete path was found, any path leading as "close" to target as possible will be
	created, and SearchResult::OutOfRange will be returned.
	Only when no "closer" position than the	given starting location could be found no path
	is created, and SearchResult::CantGetCloser	is returned.
	Path resolution: 2*SQUARE_SIZE
	Params:
		moveData
			MoveData defining the footprint of the unit requesting the path.

		startPos
			The starting location of the path. (Projected onto (x,z).)

		pfDef
			Object defining the target/goal of the search.
			Could also be used to put constraints on the searchspace used.

		path
			If any path could be found, it will be generated and put into this structure.

		moveMathOpt
			MoveMath options. Used to tell what types of objects that could be considered
			as blocking objects.

		exactPath
			Overrides the return of the "closest" path. If this option is true, a path is
			returned only if it's completed all the way to the goal defined in pfDef.
			All SearchResult::OutOfRange are then turned into SearchResult::CantGetCloser.

		maxSearchedNodes
			The maximum number of nodes/squares the search are allowed to analyze.
			This restriction could be used in cases where CPU-consumption are critical.
	*/
	SearchResult GetPath(const MoveData& moveData, const float3 startPos, const CPathFinderDef& pfDef, Path& path, unsigned int moveMathOpt = (CMoveMath::CHECK_STRUCTURES | CMoveMath::CHECK_MOBILE), bool exactPath = false, unsigned int maxSearchedNodes = 10000);

	
	//Minimum distance between two waypoints.
	static const unsigned int PATH_RESOLUTION = 2*SQUARE_SIZE;

private:  
	const static unsigned int MAX_SEARCHED_SQARES = 10000;

	class OpenSquare {
	public:
		float cost;
		float currentCost;
		int2 square;
		int sqr;
		inline bool operator< (const OpenSquare& os){return cost < os.cost;};
		inline bool operator> (const OpenSquare& os){return cost > os.cost;};
		inline bool operator==(const OpenSquare& os){return sqr == os.sqr;};
	};

	struct lessCost : public std::binary_function<OpenSquare*, OpenSquare*, bool> {
		inline bool operator()(const OpenSquare* x, const OpenSquare* y) const {
			return x->cost > y->cost;
		}
	};

	struct SquareState {
		unsigned int status;
		float cost;
	};

	void ResetSearch();
	SearchResult InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef);
	SearchResult DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef);
	void TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef, OpenSquare* parentOpenSquare, unsigned int enterDirection);
	void FinishSearch(const MoveData& moveData, Path& path);

	unsigned int maxNodesToBeSearched;
	OpenSquare openSquareBuffer[MAX_SEARCHED_SQARES];
	OpenSquare *openSquareBufferPointer;
	priority_queue<OpenSquare*, vector<OpenSquare*>, lessCost> openSquares;

	SquareState* squareState;		//Map of all squares on map.
	list<int> dirtySquares;			//Squares tested by search.

	int2 directionVector[16];		//Unit square-movement in given direction.
	float moveCost[16];				//The cost of moving in given direction.

	float3 start;
	int startxSqr, startzSqr;
	int startSquare;
	
	int goalSquare;					//Is sat during the search as the square closest to the goal.
	float goalHeuristic;			//The heuristic value of goalSquare.

	bool exactPath;
	unsigned int blockOpt;			//Used in calls of MoveMath::IsBlocked().

	//Statistic
	unsigned int testedNodes;
};


/*
While CPathFinder are a generic searcher, CPathFinderDef are used to define the
actual goal and restrictions of the search. CPathFinderDef itself are abstract
and need to be inherited.
*/
class CPathFinderDef {
public:
	virtual bool IsGoal(int xSquare, int zSquare) const = 0;
	virtual float Heuristic(int xSquare, int zSquare) const = 0;	//Number of mapsquares to goal.
	virtual bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const = 0;
	virtual bool WithinConstraints(int xSquare, int zSquare) const = 0;
	virtual void Draw() const = 0;
};


/*
CRangedGoalPFD are using a circular area defining the goal of the search.
With a heuristic pointing to the center of this area.
Note: A singularity-goal could be created by setting goalRadius = 0.
*/
class CRangedGoalPFD : public CPathFinderDef {
public:
	CRangedGoalPFD(float3 goalCenter, float goalRadius);
	virtual bool IsGoal(int xSquare, int zSquare) const;
	virtual float Heuristic(int xSquare, int zSquare) const;
	virtual bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	virtual bool WithinConstraints(int xSquare, int Square) const {return true;}
	virtual void Draw() const;

protected:
	float3 goal;
	float goalRadius;
};


#endif

