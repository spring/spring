/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHESTIMATOR_H
#define PATHESTIMATOR_H

#include <string>
#include <list>
#include <queue>

#include "IPath.h"
#include "PathConstants.h"
#include "PathDataTypes.h"
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

class CPathEstimator {
public:
	/**
	 * Creates a new estimator based on a couple of parameters
	 * @param pathFinder
	 *   The pathfinder to be used for exact cost-calculation of vertices.
	 *
	 * @param BLOCK_SIZE
	 *   The resolution of the estimator, given in mapsquares.
	 *
	 * @param cacheFileName
	 *   Name of the file on disk where pre-calculated data is stored.
	 *   The name given are added to the end of the filename, after the
	 *   name of the corresponding map.
	 *   Ex. PE-name "pe" + Mapname "Desert" => "Desert.pe"
	 */
	CPathEstimator(CPathFinder* pathFinder, unsigned int BLOCK_SIZE, const std::string& cacheFileName, const std::string& map);
	~CPathEstimator();

#if !defined(USE_MMGR)
	void* operator new(size_t size);
	void operator delete(void* p, size_t size);
#endif



	/**
	 * Returns an aproximate, low-resolution path from the starting location to
	 * the goal defined in CPathEstimatorDef, whenever any such are available.
	 * If no complete paths are found, then a path leading as "close" as
	 * possible to the goal is returned instead, together with
	 * SearchResult::OutOfRange.
	 * Only if no position closer to the goal than the starting location itself
	 * could be found, no path and SearchResult::CantGetCloser is returned.
	 * @param moveData
	 *   Defining the footprint of the unit to use the path.
	 *
	 * @param start
	 *   The starting location of the search.
	 *
	 * @param peDef
	 *   Object defining the goal of the search.
	 *   Could also be used to add constraints to the search.
	 *
	 * @param path
	 *   If a path could be found, it's generated and put into this structure.
	 *
	 * @param maxSearchedBlocks
	 *   The maximum number of nodes/blocks the search is allowed to analyze.
	 *   This restriction could be used in cases where CPU-consumption is
	 *   critical.
	 */
	IPath::SearchResult GetPath(
		const MoveData& moveData,
		float3 start,
		const CPathFinderDef& peDef,
		IPath::Path& path,
		unsigned int maxSearchedBlocks,
		bool synced = true
	);


	/**
	 * This is called whenever the ground structure of the map changes
	 * (for example on explosions and new buildings).
	 * The affected rectangular area is defined by (x1, z1)-(x2, z2).
	 * The estimator itself will decided if an update of the area is needed.
	 */
	void MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);


	/**
	 * called every frame
	 */
	void Update();

	/**
	 * Returns a checksum that can be used to check if every player has the same
	 * path data.
	 */
	boost::uint32_t GetPathChecksum() const { return pathChecksum; }

	unsigned int GetBlockSize() const { return BLOCK_SIZE; }
	unsigned int GetNumBlocksX() const { return nbrOfBlocksX; }
	unsigned int GetNumBlocksZ() const { return nbrOfBlocksZ; }

	PathNodeStateBuffer& GetNodeStateBuffer() { return blockStates; }

private:
	void InitEstimator(const std::string& cacheFileName, const std::string& map);
	void InitVertices();
	void InitBlocks();
	void CalcOffsetsAndPathCosts(int thread);
	void CalculateBlockOffsets(int, int);
	void EstimatePathCosts(int, int);

	const unsigned int BLOCK_SIZE;
	const unsigned int BLOCK_PIXEL_SIZE;
	const unsigned int BLOCKS_TO_UPDATE;



	struct SingleBlock {
		int2 block;
		const MoveData* moveData;
	};


	void FindOffset(const MoveData&, int, int);
	void CalculateVertices(const MoveData&, int, int, int thread = 0);
	void CalculateVertex(const MoveData&, int, int, unsigned int, int thread = 0);

	IPath::SearchResult InitSearch(const MoveData&, const CPathFinderDef&, bool);
	IPath::SearchResult DoSearch(const MoveData&, const CPathFinderDef&, bool);
	void TestBlock(const MoveData&, const CPathFinderDef&, PathNode&, unsigned int, bool);
	void FinishSearch(const MoveData& moveData, IPath::Path& path);
	void ResetSearch();

	bool ReadFile(const std::string& cacheFileName, const std::string& map);
	void WriteFile(const std::string& cacheFileName, const std::string& map);
	unsigned int Hash() const;

	/// Number of blocks on the X axis of the map.
	int nbrOfBlocksX;
	/// Number of blocks on the Z axis of the map.
	int nbrOfBlocksZ;

	PathNodeBuffer openBlockBuffer;
	PathNodeStateBuffer blockStates;
	/// The priority-queue used to select next block to be searched.
	PathPriorityQueue openBlocks;

	std::vector<float> vertices;
	/// List of blocks changed in last search.
	std::list<int> dirtyBlocks;
	/// Blocks that may need an update due to map changes.
	std::list<SingleBlock> needUpdate;

	static const int PATH_DIRECTIONS = 8;
	static const int PATH_DIRECTION_VERTICES = PATH_DIRECTIONS / 2;
	int2 directionVector[PATH_DIRECTIONS];
	int directionVertex[PATH_DIRECTIONS];

	float3 start;
	int2 startBlock;
	int2 goalBlock;
	int startBlocknr;
	float goalHeuristic;
	int2 goalSqrOffset;

	unsigned int maxBlocksToBeSearched;
	unsigned int testedBlocks;
	float maxNodeCost;

	std::vector<CPathFinder*> pathFinders;
	std::vector<boost::thread*> threads;

	CPathFinder* pathFinder;
	CPathCache* pathCache;

	/// currently crc from the zip
	boost::uint32_t pathChecksum;

	boost::mutex loadMsgMutex;
	boost::barrier* pathBarrier;
	boost::detail::atomic_count offsetBlockNum;
	boost::detail::atomic_count costBlockNum;

	int nextOffsetMessage;
	int nextCostMessage;
};

#endif
