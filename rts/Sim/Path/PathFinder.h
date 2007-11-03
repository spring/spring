#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "IPath.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
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

	/**
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
	SearchResult GetPath(const MoveData& moveData, const float3 startPos,
	                     const CPathFinderDef& pfDef, Path& path,
	                     bool testMobile, bool exactPath = false,
	                     unsigned int maxSearchedNodes = 10000, bool needPath = true);

	SearchResult GetPath(const MoveData& moveData, const std::vector<float3>& startPos,
	                     const CPathFinderDef& pfDef, Path& path);
	
	//Minimum distance between two waypoints.
	enum { PATH_RESOLUTION = 2 * SQUARE_SIZE };

private:  
	enum { MAX_SEARCHED_SQUARES = 10000 };

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
	class myVector{
	public:
		typedef OpenSquare* value_type;
		typedef int size_type;
		typedef OpenSquare* reference;
		typedef const OpenSquare* const_reference;

		int bufPos;
		OpenSquare* buf[MAX_SEARCHED_SQUARES];

		myVector() {bufPos=-1;}

		inline void push_back(OpenSquare* os)			{buf[++bufPos]=os;}
		inline void pop_back()										{--bufPos;}
		inline OpenSquare* back() const						{return buf[bufPos];}
		inline const value_type& front() const		{return buf[0];}
		inline value_type& front()								{return buf[0];}
		inline bool empty() const									{return (bufPos<0);}
		inline size_type size()										{return bufPos+1;}
		inline OpenSquare** begin()								{return &buf[0];}
		inline OpenSquare** end()									{return &buf[bufPos+1];}
		inline void clear()												{bufPos=-1;}
	};

  class myPQ : public std::priority_queue<OpenSquare*,myVector,lessCost>{
	public:
		void DeleteAll();
  };
	void ResetSearch();
	SearchResult InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef);
	SearchResult DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef);
	bool TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef, OpenSquare* parentOpenSquare, unsigned int enterDirection);
	void FinishSearch(const MoveData& moveData, Path& path);

	unsigned int maxNodesToBeSearched;
	myPQ openSquares;

	SquareState* squareState;		//Map of all squares on map.
	//list<int> dirtySquares;			//Squares tested by search.
	vector<int> dirtySquares;

	int2 directionVector[16];		//Unit square-movement in given direction.
	float moveCost[16];				//The cost of moving in given direction.

	float3 start;
	int startxSqr, startzSqr;
	int startSquare;
	
	int goalSquare;					//Is sat during the search as the square closest to the goal.
	float goalHeuristic;			//The heuristic value of goalSquare.

	bool exactPath;
	bool testMobile;
	bool needPath;

	//Statistic
	unsigned int testedNodes;

	OpenSquare *openSquareBufferPointer;
	OpenSquare openSquareBuffer[MAX_SEARCHED_SQUARES];
public:
	void Draw(void);
};

class CPathFinderDef {
public:
	CPathFinderDef(float3 goalCenter, float goalRadius);
	bool IsGoal(int xSquare, int zSquare) const;
	float Heuristic(int xSquare, int zSquare) const;
	bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	virtual bool WithinConstraints(int xSquare, int Square) const {return true;}
	void Draw() const;
	int2 GoalSquareOffset(int blockSize) const;

	float3 goal;
	float sqGoalRadius;
	int goalSquareX;
	int goalSquareZ;
	virtual ~CPathFinderDef();
};

class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(float3 start, float3 goal, float goalRadius,float searchSize,int extraSize);
	virtual bool WithinConstraints(int xSquare, int zSquare) const;
	virtual ~CRangedGoalWithCircularConstraint();

private:
	int halfWayX;
	int halfWayZ;
	int searchRadiusSq;
};


#endif

