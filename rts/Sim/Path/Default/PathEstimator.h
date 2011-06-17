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
	 * @param BSIZE
	 *   The resolution of the estimator, given in mapsquares.
	 *
	 * @param cacheFileName
	 *   Name of the file on disk where pre-calculated data is stored.
	 *   The name given are added to the end of the filename, after the
	 *   name of the corresponding map.
	 *   Ex. PE-name "pe" + Mapname "Desert" => "Desert.pe"
	 */
	CPathEstimator(CPathFinder*, unsigned int BSIZE, const std::string& cacheFileName, const std::string& mapFileName);
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
	void InitBlocks();
	void CalcOffsetsAndPathCosts(unsigned int thread);
	void CalculateBlockOffsets(unsigned int, unsigned int);
	void EstimatePathCosts(unsigned int, unsigned int);

	struct SingleBlock {
		int2 blockPos;
		const MoveData* moveData;
	};

	void FindOffset(const MoveData&, unsigned int, unsigned int);
	void CalculateVertices(const MoveData&, unsigned int, unsigned int, unsigned int threadNum = 0);
	void CalculateVertex(const MoveData&, unsigned int, unsigned int, unsigned int, unsigned int threadNum = 0);

	IPath::SearchResult InitSearch(const MoveData&, const CPathFinderDef&, bool);
	IPath::SearchResult DoSearch(const MoveData&, const CPathFinderDef&, bool);
	void TestBlock(const MoveData&, const CPathFinderDef&, PathNode&, unsigned int pathDir, bool synced);
	void FinishSearch(const MoveData& moveData, IPath::Path& path);
	void ResetSearch();

	bool ReadFile(const std::string& cacheFileName, const std::string& map);
	void WriteFile(const std::string& cacheFileName, const std::string& map);
	unsigned int Hash() const;

	int2 mStartBlock;
	int2 mGoalBlock;
	int2 mGoalSqrOffset;
	unsigned int mStartBlockIdx;
	float mGoalHeuristic;

	unsigned int maxBlocksToBeSearched;
	unsigned int testedBlocks;
	float maxNodeCost;

	std::vector<CPathFinder*> pathFinders;
	std::vector<boost::thread*> threads;

	CPathFinder* pathFinder;
	CPathCache* pathCache;


	const unsigned int BLOCK_SIZE;
	const unsigned int BLOCK_PIXEL_SIZE;
	const unsigned int BLOCKS_TO_UPDATE;

	unsigned int nbrOfBlocksX;                  /// Number of blocks on the X axis of the map.
	unsigned int nbrOfBlocksZ;                  /// Number of blocks on the Z axis of the map.
	unsigned int moveMathOptions;

	unsigned int nextOffsetMessageIdx;
	unsigned int nextCostMessageIdx;

	boost::uint32_t pathChecksum;               ///< currently crc from the zip
	boost::detail::atomic_count offsetBlockNum;
	boost::detail::atomic_count costBlockNum;
	boost::barrier* pathBarrier;

	PathNodeBuffer openBlockBuffer;             /// The priority-queue used to select next block to be searched.
	PathNodeStateBuffer blockStates;
	PathPriorityQueue openBlocks;

	std::vector<float> vertexCosts;
	std::list<unsigned int> dirtyBlocks;        /// List of blocks changed in last search.
	std::list<SingleBlock> changedBlocks;       /// Blocks that may need an update due to map changes.

	int2 directionVectors[PATH_DIRECTIONS];
};

#endif
