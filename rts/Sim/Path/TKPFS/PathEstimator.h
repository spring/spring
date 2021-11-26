/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TKPFS_PATHESTIMATOR_H
#define TKPFS_PATHESTIMATOR_H

#include <atomic>
#include <cinttypes>
#include <deque>
#include <string>
#include <vector>

#include "PathingState.h"

#include "Sim/Path/Default/IPath.h"
#include "IPathFinder.h"
#include "Sim/Path/Default/PathConstants.h"
#include "Sim/Path/Default/PathDataTypes.h"
#include "System/float3.h"
//#include "System/Threading/SpringThreading.h"


struct MoveDef;
class CPathEstimatorDef;
class CPathFinderDef;
class CPathCache;
class CSolidObject;
struct TKPFSPathDrawer;

namespace TKPFS {

class PathingState;
class CPathFinder;

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
	 * @param peFileName
	 *   Name of the file on disk where pre-calculated data is stored.
	 *   The name given are added to the end of the filename, after the
	 *   name of the corresponding map.
	 *   Ex. PE-name "pe" + Mapname "Desert" => "Desert.pe"
	 */
	void Init(IPathFinder*, unsigned int BSIZE, PathingState* ps);
	void Kill();

	/**
	 * called every frame
	 */
	//void Update();

	IPathFinder* GetParent() override { return parentPathFinder; }

	//const std::vector<float>& GetVertexCosts() const { return vertexCosts; }
	//const std::deque<int2>& GetUpdatedBlocks() const { return updatedBlocks; }


protected: // IPathFinder impl
	IPath::SearchResult DoBlockSearch(const CSolidObject* owner, const MoveDef& moveDef, const int2 s, const int2 g);
	IPath::SearchResult DoBlockSearch(const CSolidObject* owner, const MoveDef& moveDef, const float3 sw, const float3 gw);
	IPath::SearchResult DoSearch(const MoveDef&, const CPathFinderDef&, const CSolidObject* owner) override;

	bool TestBlock(
		const MoveDef& moveDef,
		const CPathFinderDef& pfDef,
		const PathNode* parentSquare,
		const CSolidObject* owner,
		const unsigned int pathOptDir,
		const unsigned int blockStatus,
		float speedMod
	) override;
	void FinishSearch(const MoveDef& moveDef, const CPathFinderDef& pfDef, IPath::Path& path) const override;

	const CPathCache::CacheItem& GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const override;

	void AddCache(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) override;

private:
	void InitEstimator();
	void InitBlocks();

	std::uint32_t CalcHash(const char* caller) const;

private:
	friend class CPathManager;
	friend struct ::TKPFSPathDrawer;

	unsigned int BLOCKS_TO_UPDATE = 0;

	unsigned int nextOffsetMessageIdx = 0;
	//unsigned int nextCostMessageIdx = 0;

	std::uint32_t pathChecksum = 0;
	std::uint32_t fileHashCode = 0;

	std::atomic<std::int64_t> offsetBlockNum = {0};
	std::atomic<std::int64_t> costBlockNum = {0};

	IPathFinder* parentPathFinder; // parent (PF if BLOCK_SIZE is 16, PE[16] if 32)
	//CPathEstimator* nextPathEstimator; // next lower-resolution estimator

	mutable CPathCache::CacheItem tempCacheItem;

	PathingState* pathingState;
};

}

#endif
