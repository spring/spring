/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHESTIMATOR_H
#define PATHESTIMATOR_H

#include <string>
#include <list>
#include <queue>

#include "IPath.h"
#include "System/float3.h"

#include <boost/thread/thread.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/cstdint.hpp>

struct MoveData;
class CPathFinder;
class CPathEstimatorDef;
class CPathFinderDef;
class CPathCache;

class CPathEstimator: public IPath
{
public:
	/**
	 * Creates a new estimator based on a couple of parameters
	 * @param pathFinder
	 *		The pathfinder to be used for exact cost-calculation of vertices.
	 *
	 * @param BLOCK_SIZE
	 *		The resolution of the estimator, given in mapsquares.
	 *
	 * @param moveMathOpt
	 *		What level of blocking the estimator should care of. Based on the
	 *		available options given in CMoveMath.
	 *
	 * @param cacheFileName
	 *		Name of the file on disk where pre-calculated data is stored.
	 *		The name given are added to the end of the filename, after the
	 *		name of the corresponding map.
	 *		Ex. PE-name "pe" + Mapname "Desert" => "Desert.pe"
	 */
	CPathEstimator(CPathFinder* pathFinder, unsigned int BLOCK_SIZE, unsigned int moveMathOpt, std::string cacheFileName, const std::string& map);
	~CPathEstimator();

#if !defined(USE_MMGR)
	// note: thread-safety (see PathFinder.cpp)?
	void* operator new(size_t size);
	void operator delete(void* p, size_t size);
#endif



	/**
	 * Returns a estimate low-resolution path from starting location to the goal defined in
	 * CPathEstimatorDef, whenever any such are available.
	 * If no complete path are found, then a path leading as "close" as possible to the goal
	 * is returned instead, together with SearchResult::OutOfRange.
	 * Only if no position closer to the goal than the starting location itself could be found
	 * no path and SearchResult::CantGetCloser is returned.
	 * @param moveData
	 *		Defining the footprint of the unit to use the path.
	 *
	 * @param start
	 *		The starting location of the search.
	 *
	 * @param peDef
	 *		Object defining the goal of the search.
	 *		Could also be used to add constraints to the search.
	 *
	 * @param path
	 *		If a path could be found, it's generated and put into this structure.
	 *
	 * @param maxSearchedBlocks
	 *		The maximum number of nodes/blocks the search are allowed to analyze.
	 *		This restriction could be used in cases where CPU-consumption are critical.
	 */
	SearchResult GetPath(
		const MoveData& moveData,
		float3 start,
		const CPathFinderDef& peDef,
		Path& path,
		unsigned int maxSearchedBlocks,
		bool synced = true
	);


	/**
	 * Whenever the ground structure of the map changes (ex. at explosions and new buildings)
	 * this function shall be called, with (x1, z1)-(x2, z2) defining the rectangular area
	 * affected. The estimator will itself decided when update of the area is needed.
	 */
	void MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);


	/**
	 * called every frame
	 */
	void Update();

	/// find the best block to use for this pos
	float3 FindBestBlockCenter(const MoveData* moveData, float3 pos);

	/// Return a checksum that can be used to check if every player has the same path data
	boost::uint32_t GetPathChecksum() const { return pathChecksum; }

	static const unsigned int MAX_SEARCHED_BLOCKS = 10000U;

private:
	void InitEstimator(const std::string& cacheFileName, const std::string& map);
	void InitVertices();
	void InitBlocks();
	void CalcOffsetsAndPathCosts(int thread);
	void CalculateBlockOffsets(int, int);
	void EstimatePathCosts(int, int);

	boost::mutex loadMsgMutex;
	std::vector<CPathFinder*> pathFinders;
	std::vector<boost::thread*> threads;



	const unsigned int BLOCK_SIZE;
	const unsigned int BLOCK_PIXEL_SIZE;
	const unsigned int BLOCKS_TO_UPDATE;


	class OpenBlock {
		public:
			float cost;
			float currentCost;
			int2 block;
			int blocknr;
			inline bool operator<  (const OpenBlock& ob) const { return cost < ob.cost; }
			inline bool operator>  (const OpenBlock& ob) const { return cost > ob.cost; }
			inline bool operator== (const OpenBlock& ob) const { return blocknr == ob.blocknr; }
	};

	struct lessCost: public std::binary_function<OpenBlock*, OpenBlock*, bool> {
		inline bool operator() (const OpenBlock* x, const OpenBlock* y) const {
			return (x->cost > y->cost);
		}
	};

	struct BlockInfo {
		int2* sqrCenter;
		float cost;
		int2 parentBlock;
		unsigned int options;
	};

	struct SingleBlock {
		int2 block;
		MoveData* moveData;
	};


	void FindOffset(const MoveData&, int, int);
	void CalculateVertices(const MoveData&, int, int, int thread = 0);
	void CalculateVertex(const MoveData&, int, int, unsigned int, int thread = 0);

	SearchResult InitSearch(const MoveData& moveData, const CPathFinderDef& peDef);
	SearchResult StartSearch(const MoveData& moveData, const CPathFinderDef& peDef);
	SearchResult DoSearch(const MoveData& moveData, const CPathFinderDef& peDef);
	void TestBlock(const MoveData& moveData, const CPathFinderDef& peDef, OpenBlock& parentOpenBlock, unsigned int direction);
	void FinishSearch(const MoveData& moveData, Path& path);
	void ResetSearch();

	bool ReadFile(std::string cacheFileName, const std::string& map);
	void WriteFile(std::string cacheFileName, const std::string& map);
	unsigned int Hash() const;

	CPathFinder* pathFinder;

	int nbrOfBlocksX, nbrOfBlocksZ, nbrOfBlocks;									// Number of blocks on map.
	BlockInfo* blockState;															// Map over all blocks and there states.
	OpenBlock openBlockBuffer[MAX_SEARCHED_BLOCKS];									// The buffer to be used in the priority-queue.
	unsigned int openBlockBufferIndex;												// index of the most recently added open block
	std::priority_queue<OpenBlock*, std::vector<OpenBlock*>, lessCost> openBlocks;	// The priority-queue used to select next block to be searched.
	std::list<int> dirtyBlocks;														// List of blocks changed in last search.
	std::list<SingleBlock> needUpdate;												// Blocks that may need an update due to map changes.

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

	CPathCache* pathCache;

	boost::uint32_t pathChecksum; ///< currently crc from the zip

	boost::barrier* pathBarrier;
	boost::detail::atomic_count offsetBlockNum, costBlockNum;

	int lastOffsetMessage, lastCostMessage;
};

#endif
