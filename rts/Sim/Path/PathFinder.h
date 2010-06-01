/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <cstdlib>
#include "IPath.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include <queue>
#include <list>

class CPathFinderDef;

void* pfAlloc(size_t n);
void pfDealloc(void *p,size_t n);

class CPathFinder : public IPath {
public:
	CPathFinder();
	~CPathFinder();

#if !defined(USE_MMGR)
	void* operator new(size_t size){return pfAlloc(size);};
	inline void operator delete(void* p,size_t size){pfDealloc(p,size);};
#endif

	/**
	Gives a detailed path from given starting location to target defined in CPathFinderDef,
	whenever any such are available.
	If no complete path was found, any path leading as "close" to target as possible will be
	created, and SearchResult::OutOfRange will be returned.
	Only when no "closer" position than the given starting location could be found no path
	is created, and SearchResult::CantGetCloser is returned.
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
	                     unsigned int maxSearchedNodes = 10000, bool needPath = true,
	                     int ownerId = 0);

	SearchResult GetPath(const MoveData& moveData, const std::vector<float3>& startPos,
	                     const CPathFinderDef& pfDef, Path& path, int ownerId = 0);

	// Minimum distance between two waypoints.
	enum { PATH_RESOLUTION = 2 * SQUARE_SIZE };

	enum PATH_OPTIONS {
		PATHOPT_RIGHT     =   1,      //-x
		PATHOPT_LEFT      =   2,      //+x
		PATHOPT_UP        =   4,      //+z
		PATHOPT_DOWN      =   8,      //-z
		PATHOPT_DIRECTION = (PATHOPT_RIGHT | PATHOPT_LEFT | PATHOPT_UP | PATHOPT_DOWN),
		PATHOPT_START     =  16,
		PATHOPT_OPEN      =  32,
		PATHOPT_CLOSED    =  64,
		PATHOPT_FORBIDDEN = 128,
		PATHOPT_BLOCKED   = 256,
	};

	/** Enable/disable heat mapping.

	Heat mapping makes the pathfinder value unused paths more. Less path overlap should
	make units behave more intelligently.

	*/
	void InitHeatMap();
	void SetHeatMapState(bool enabled);
	bool GetHeatMapState() { return heatMapping; }
	void UpdateHeatMap();

	void UpdateHeatValue(int x, int y, int value, int ownerId)
	{
		assert(!heatmap.empty());
		x >>= 1;
		y >>= 1;
		if (heatmap[x][y].value < value + heatMapOffset) {
			heatmap[x][y].value = value + heatMapOffset;
			heatmap[x][y].ownerId = ownerId;
		}
	}

	const int GetHeatOwner(int x, int y)
	{
		assert(!heatmap.empty());
		x >>= 1;
		y >>= 1;
		return heatmap[x][y].ownerId;
	}

	const int GetHeatValue(int x, int y)
	{
		assert(!heatmap.empty());
		x >>= 1;
		y >>= 1;
		return std::max(0, heatmap[x][y].value - heatMapOffset);
	}

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
		typedef OpenSquare** iterator;
		typedef const OpenSquare* const* const_iterator;

		// gcc 4.3 requires concepts, so give them to it
		value_type& operator[](size_type idx) { return buf[idx]; }
		const value_type& operator[](size_type idx) const { return buf[idx]; }

		typedef iterator pointer;
		typedef const_iterator const_pointer;
		typedef int difference_type;

		// XXX don't use this
		// FIXME write proper versions of those
		typedef OpenSquare** reverse_iterator;
		typedef const OpenSquare* const* const_reverse_iterator;
		reverse_iterator rbegin() { return 0; }
		reverse_iterator rend() { return 0; }
		const_reverse_iterator rbegin() const { return 0; }
		const_reverse_iterator rend() const { return 0; }
		myVector(int, const value_type&) { abort(); }
		myVector(iterator, iterator) { abort(); }
		void insert(iterator, const value_type&) { abort(); }
		void insert(iterator, const size_type&, const value_type&) { abort(); }
		void insert(iterator, iterator, iterator) { abort(); }
		void erase(iterator, iterator) { abort(); }
		void erase(iterator) { abort(); }
		void erase(iterator, iterator, iterator) { abort(); }
		void swap(myVector&) { abort(); }
		// end of concept hax

		int bufPos;
		OpenSquare* buf[MAX_SEARCHED_SQUARES];

		myVector() {bufPos=-1;}

		inline void push_back(OpenSquare* os)	{buf[++bufPos]=os;}
		inline void pop_back()			{--bufPos;}
		inline OpenSquare* back() const		{return buf[bufPos];}
		inline const value_type& front() const	{return buf[0];}
		inline value_type& front()		{return buf[0];}
		inline bool empty() const		{return (bufPos<0);}
		inline size_type size() const		{return bufPos+1;}
		inline size_type max_size() const	{return 1<<30;}
		inline iterator begin()			{return &buf[0];}
		inline iterator end()			{return &buf[bufPos+1];}
		inline const_iterator begin() const	{return &buf[0];}
		inline const_iterator end() const	{return &buf[bufPos+1];}
		inline void clear()			{bufPos=-1;}
	};

	class myPQ : public std::priority_queue<OpenSquare*,myVector,lessCost>{
	public:
		void DeleteAll();
	};

	void ResetSearch();
	SearchResult InitSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId);
	SearchResult DoSearch(const MoveData& moveData, const CPathFinderDef& pfDef, int ownerId);
	bool TestSquare(const MoveData& moveData, const CPathFinderDef& pfDef, OpenSquare* parentOpenSquare,
			unsigned int enterDirection, int ownerId);
	void FinishSearch(const MoveData& moveData, Path& path);
	void AdjustFoundPath(const MoveData& moveData, Path& path, float3& nextPoint,
			std::deque<int2>& previous, int2 square);

	unsigned int maxNodesToBeSearched;
	myPQ openSquares;

	SquareState* squareState;			///< Map of all squares on map.
	// std::list<int> dirtySquares;		///< Squares tested by search.
	std::vector<int> dirtySquares;

	int2 directionVector[16];		///< Unit square-movement in given direction.
	float moveCost[16];				///< The cost of moving in given direction.

	float3 start;
	int startxSqr, startzSqr;
	int startSquare;

	int goalSquare;					///< Is sat during the search as the square closest to the goal.
	float goalHeuristic;			///< The heuristic value of goalSquare.

	bool exactPath;
	bool testMobile;
	bool needPath;

	// Statistic
	unsigned int testedNodes;

	OpenSquare *openSquareBufferPointer;
	OpenSquare openSquareBuffer[MAX_SEARCHED_SQUARES];

	// Heat mapping
	struct HeatMapValue {
		HeatMapValue(): value(0), ownerId(0) {
		}
		int value;
		int ownerId;
	};

	std::vector<std::vector<HeatMapValue> > heatmap; //! resolution is hmapx*hmapy
	int heatMapOffset;  //! heatmap values are relative to this
	bool heatMapping;
};

class CPathFinderDef {
public:
	CPathFinderDef(float3 goalCenter, float goalRadius);
	bool IsGoal(int xSquare, int zSquare) const;
	float Heuristic(int xSquare, int zSquare) const;
	bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	virtual bool WithinConstraints(int xSquare, int Square) const {return true;}
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

#endif // PATHFINDER_H
