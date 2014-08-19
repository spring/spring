/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHESTIMATOR_H
#define PATHESTIMATOR_H

#include <string>
#include <vector>
#include <list>

#include "IPath.h"
#include "IPathFinder.h"
#include "PathConstants.h"
#include "PathDataTypes.h"
#include "System/float3.h"

#include <boost/detail/atomic_count.hpp>
#include <boost/cstdint.hpp>

struct MoveDef;
class CPathFinder;
class CPathEstimatorDef;
class CPathFinderDef;
class CPathCache;
class CSolidObject;


namespace boost {
	class thread;
	class barrier;
}

class CPathEstimator: public IPathFinder {
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


	/**
	 * Returns an aproximate, low-resolution path from the starting location to
	 * the goal defined in CPathEstimatorDef, whenever any such are available.
	 * If no complete paths are found, then a path leading as "close" as
	 * possible to the goal is returned instead, together with
	 * SearchResult::OutOfRange.
	 * Only if no position closer to the goal than the starting location itself
	 * could be found, no path and SearchResult::CantGetCloser is returned.
	 * @param moveDef
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
		const MoveDef& moveDef,
		const CPathFinderDef& peDef,
		float3 start,
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

	static const int2* GetDirectionVectorsTable();

private:
	void InitEstimator(const std::string& cacheFileName, const std::string& map);
	void InitBlocks();

	void CalcOffsetsAndPathCosts(unsigned int threadNum);
	void CalculateBlockOffsets(unsigned int, unsigned int);
	void EstimatePathCosts(unsigned int, unsigned int);

	int2 FindOffset(const MoveDef&, unsigned int, unsigned int);
	void CalculateVertices(const MoveDef&, unsigned int, unsigned int, unsigned int threadNum = 0);
	void CalculateVertex(const MoveDef&, unsigned int, unsigned int, unsigned int, unsigned int threadNum = 0);

	IPath::SearchResult InitSearch(const MoveDef&, const CPathFinderDef&, bool);
	IPath::SearchResult DoSearch(const MoveDef&, const CPathFinderDef&, bool);
	void TestBlock(const MoveDef&, const CPathFinderDef&, PathNode&, unsigned int pathDir, bool synced);
	void FinishSearch(const MoveDef& moveDef, IPath::Path& path);

	bool ReadFile(const std::string& cacheFileName, const std::string& map);
	void WriteFile(const std::string& cacheFileName, const std::string& map);
	unsigned int Hash() const;

private:
	friend class CPathManager;
	friend class CDefaultPathDrawer;

	const unsigned int BLOCK_PIXEL_SIZE;
	const unsigned int BLOCKS_TO_UPDATE;

	unsigned int nextOffsetMessageIdx;
	unsigned int nextCostMessageIdx;

	boost::uint32_t pathChecksum;               ///< currently crc from the zip

	boost::detail::atomic_count offsetBlockNum;
	boost::detail::atomic_count costBlockNum;
	boost::barrier* pathBarrier;

	CPathFinder* pathFinder;
	CPathCache* pathCache[2];                   /// [0] = !synced, [1] = synced

	std::vector<CPathFinder*> pathFinders;
	std::vector<boost::thread*> threads;

	std::vector<float> vertexCosts;	
	std::list<int2> updatedBlocks;       /// Blocks that may need an update due to map changes.

	int blockUpdatePenalty;
};

#endif
