#ifndef PATHESTIMATOR_H
#define PATHESTIMATOR_H

#include "IPath.h"
#include "PathFinder.h"
#include "float3.h"
#include "MoveInfo.h"
#include <string>
#include <queue>
#include <list>
using namespace std;

class CPathEstimatorDef;


class CPathEstimator : public IPath {
public:
	/*
	Constructor.
	Creates a new estimator based on a couple of parameters.
	Param:
		pathFinder
			The pathfinder to be used for exact cost-calculation of vertices.

		BLOCK_SIZE
			The resolution of the estimator, given in mapsquares.

		moveMathOpt
			What level of blocking the estimator should care of. Based on the
			available options given in CMoveMath.

		name
			Name of the file on disk where pre-calculated data is stored.
			The name given are added to the end of the filename, after the name
			of the corresponding map.
			Ex. PE-name "pe" + Mapname "Desert" => "Desert.pe"
	*/
	CPathEstimator(CPathFinder* pathFinder, unsigned int BLOCK_SIZE, unsigned int moveMathOpt, string name);
	~CPathEstimator();

	void* operator new(size_t size){return pfAlloc(size);};
	inline void operator delete(void* p,size_t size){pfDealloc(p,size);};


	/*
	Returns a estimate low-resolution path from starting location to the goal defined in
	CPathEstimatorDef, whenever any such are available.
	If no complete path are found, then a path leading as "close" as possible to the goal
	is returned instead, together with SearchResult::OutOfRange.
	Only if no position closer to the goal than the starting location itself could be found
	no path and SearchResult::CantGetCloser is returned.
	Param:
		moveData
			Defining the footprint of the unit to use the path.

		start
			The starting location of the search.

		peDef
			Object defining the goal of the search.
			Could also be used to add constraints to the search.

		path
			If a path could be found, it's generated and put into this structure.

		maxSearchedBlocks
			The maximum number of nodes/blocks the search are allowed to analyze.
			This restriction could be used in cases where CPU-consumption are critical.
	*/
	SearchResult GetPath(const MoveData& moveData, float3 start, const CPathEstimatorDef& peDef, Path& path, unsigned int maxSearchedBlocks = 10000);


	/*
	Whenever the ground structure of the map changes (ex. at explosions and new buildings)
	this function shall be called, with (x1,z1)-(x2,z2) defining the rectangular area affected.
	The estimator will itself decided when update of the area is needed.
	*/
	void MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);


	/*
	This function shall be called every 1/30sec during game runtime.
	*/
	void Update();

private:
	static const unsigned int MAX_SEARCHED_BLOCKS=10000;
	const unsigned int BLOCK_SIZE;
	const unsigned int BLOCK_PIXEL_SIZE;
	const unsigned int BLOCKS_TO_UPDATE;


	class OpenBlock {
	public:
		float cost;
		float currentCost;
		int2 block;
		int blocknr;
		inline bool operator< (const OpenBlock& ob){return cost < ob.cost;};
		inline bool operator> (const OpenBlock& ob){return cost > ob.cost;};
		inline bool operator==(const OpenBlock& ob){return blocknr == ob.blocknr;};
	};

	struct lessCost : public binary_function<OpenBlock*, OpenBlock*, bool> {
		inline bool operator()(const OpenBlock* x, const OpenBlock* y) const {
			return x->cost > y->cost;
		}
	};

	struct BlockInfo {
		int2* sqrOffset;
		float cost;
		int2 parentBlock;
		unsigned int options;
	};

	struct SingleBlock {
		int2 block;
		MoveData* moveData;
	};
	

	void FindOffset(const MoveData& moveData, int blockX, int blockZ);
	void CalculateVertices(const MoveData& moveData, int blockX, int blockZ);
	void CalculateVertex(const MoveData& moveData, int parentBlockX, int parentBlockZ, unsigned int direction);
	
	SearchResult InitSearch(const MoveData& moveData, const CPathEstimatorDef& peDef);
	SearchResult StartSearch(const MoveData& moveData, const CPathEstimatorDef& peDef);
	SearchResult DoSearch(const MoveData& moveData, const CPathEstimatorDef& peDef);
	void TestBlock(const MoveData& moveData, const CPathEstimatorDef& peDef, OpenBlock& parentOpenBlock, unsigned int direction);
	void FinishSearch(const MoveData& moveData, Path& path);
	void ResetSearch();

	bool ReadFile(string name);
	void WriteFile(string name);
	unsigned int Hash();

	CPathFinder* pathFinder;

	int nbrOfBlocksX, nbrOfBlocksZ, nbrOfBlocks;				//Number of blocks on map.
	BlockInfo* blockState;										//Map over all blocks and there states.
	OpenBlock openBlockBuffer[MAX_SEARCHED_BLOCKS];				//The buffer to be used in the priority-queue.
	OpenBlock *openBlockBufferPointer;							//Pointer to the current position in the buffer.
	priority_queue<OpenBlock*, vector<OpenBlock*>, lessCost> openBlocks;	//The priority-queue used to select next block to be searched.
	list<int> dirtyBlocks;										//List of blocks changed in last search.
	list<SingleBlock> needUpdate;								//Blocks that may need an update due to map changes.

	static const int PATH_DIRECTIONS = 8;
	static const int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS / 2;
	int2 directionVector[PATH_DIRECTIONS];
	int directionVertex[PATH_DIRECTIONS];

	unsigned int nbrOfVertices;
	float* vertex;

	unsigned int maxBlocksToBeSearched;
	unsigned int moveMathOptions;
	float3 start;
	int2 startBlock, goalBlock;
	int startBlocknr;
	float goalHeuristic;
	int2 goalSqrOffset;

	int testedBlocks;
};


/*
While CPathEstimator is generic, CPathEstimatorDef is used to
define the target and restrictions of the search.
CPathEstimatorDef is abstract and need to be inherited.
*/
class CPathEstimatorDef : public CPathFinderDef {
public:
	virtual int2 GoalSquareOffset(int blockSize) const=0;
};


/*
CRangedGoalPED is simply a extended estimator-version of the
CRangedGoalPFD, using the same idea (and code).
*/
class CRangedGoalPED : public CPathEstimatorDef, public CRangedGoalPFD {
public:
	CRangedGoalPED(float3 goalCenter, float goalRadius) : CRangedGoalPFD(goalCenter, goalRadius) {}
	virtual bool IsGoal(int xSquare, int zSquare) const {return CRangedGoalPFD::IsGoal(xSquare, zSquare);}
	virtual float Heuristic(int xSquare, int zSquare) const {return CRangedGoalPFD::Heuristic(xSquare, zSquare);}
	virtual bool WithinConstraints(int xSquare, int zSquare) const {return CRangedGoalPFD::WithinConstraints(xSquare, zSquare);}
	virtual bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const {return CRangedGoalPFD::GoalIsBlocked(moveData, moveMathOptions);}
	virtual void Draw() const {CRangedGoalPFD::Draw();}
	virtual int2 GoalSquareOffset(int blockSize) const;
};


/*
Using a ranged goal, but are also limiting the search area into a circular
area with it's center in the middle point between start and goal location
and with a minimum radius still including both start and goal location.
Note: This is a strong restriction, that probits the search to go "backwards".
*/
class CRangedGoalWithCircularConstraintPFD : public CRangedGoalPFD {
public:
	CRangedGoalWithCircularConstraintPFD(float3 start, float3 goal, float goalRadius);
	virtual bool WithinConstraints(int xSquare, int zSquare) const;

private:
	float3 start, halfWay;
	float searchRadius;
};


#endif
