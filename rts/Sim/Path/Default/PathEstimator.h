/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHESTIMATOR_H
#define PATHESTIMATOR_H

#include <atomic>
#include <cinttypes>
#include <deque>
#include <string>
#include <vector>

#include "IPath.h"
#include "IPathFinder.h"
#include "PathConstants.h"
#include "PathDataTypes.h"
#include "System/float3.h"
#include "System/Threading/SpringThreading.h"


struct MoveDef;
class CPathFinder;
class CPathEstimatorDef;
class CPathFinderDef;
class CPathCache;
class CSolidObject;

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
	CPathEstimator(IPathFinder*, unsigned int BSIZE, const std::string& cacheFileName, const std::string& mapFileName);
	~CPathEstimator();


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
	std::uint32_t GetPathChecksum() const { return pathChecksum; }

	static const int2* GetDirectionVectorsTable();

protected: // IPathFinder impl
	IPath::SearchResult DoBlockSearch(const CSolidObject* owner, const MoveDef& moveDef, const int2 s, const int2 g);
	IPath::SearchResult DoBlockSearch(const CSolidObject* owner, const MoveDef& moveDef, const float3 sw, const float3 gw);
	IPath::SearchResult DoSearch(const MoveDef&, const CPathFinderDef&, const CSolidObject* owner);

	bool TestBlock(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		const unsigned int pathOptDir,
		const unsigned int blockStatus,
		float speedMod);
	IPath::SearchResult FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& path) const;

	const CPathCache::CacheItem& GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const;

	void AddCache(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	);

private:
	void InitEstimator(const std::string& cacheFileName, const std::string& mapName);
	void InitBlocks();

	void CalcOffsetsAndPathCosts(unsigned int threadNum);
	void CalculateBlockOffsets(unsigned int, unsigned int);
	void EstimatePathCosts(unsigned int, unsigned int);

	int2 FindOffset(const MoveDef&, unsigned int, unsigned int) const;
	void CalculateVertices(const MoveDef&, int2, unsigned int threadNum = 0);
	void CalculateVertex(const MoveDef&, int2, unsigned int, unsigned int threadNum = 0);

	bool ReadFile(const std::string& baseFileName, const std::string& mapName);
	void WriteFile(const std::string& baseFileName, const std::string& mapName);

	std::uint32_t CalcChecksum() const;
	std::uint32_t CalcHash(const char* caller) const;

private:
	friend class CPathManager;
	friend class CDefaultPathDrawer;

	const unsigned int BLOCKS_TO_UPDATE;

	unsigned int nextOffsetMessageIdx;
	unsigned int nextCostMessageIdx;

	std::uint32_t pathChecksum;
	std::uint32_t fileHashCode;

	std::atomic<std::int64_t> offsetBlockNum;
	std::atomic<std::int64_t> costBlockNum;
	spring::barrier* pathBarrier;

	IPathFinder* pathFinder;
	CPathCache* pathCache[2]; // [0] = !synced, [1] = synced

	std::vector<IPathFinder*> pathFinders;
	std::vector<spring::thread*> threads;

	// next lower-resolution estimator
	CPathEstimator* nextPathEstimator;

	std::vector<float> maxSpeedMods;
	std::vector<float> vertexCosts;
	/// blocks that may need an update due to map changes
	std::deque<int2> updatedBlocks;

	int blockUpdatePenalty;

	struct SOffsetBlock {
		float cost;
		int2 offset;
		SOffsetBlock(const float _cost, const int x, const int y) : cost(_cost), offset(x,y) {}
	};
	std::vector<SOffsetBlock> offsetBlocksSortedByCost;
};

#endif
