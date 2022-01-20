/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TKPFS_PATHINGSTATESYSTEM_H
#define TKPFS_PATHINGSTATESYSTEM_H

#include <atomic>
#include <string>
#include <vector>

#include "IPathFinder.h"
#include "Sim/Path/Default/PathDataTypes.h"
#include "System/Threading/SpringThreading.h"

#include "Sim/Path/TKPFS/PathEstimator.h"
#include "Sim/Path/TKPFS/PathManager.h"

struct TKPFSPathDrawer;

namespace TKPFS {

class CPathEstimator;
class CPathFinder;

class PathingState {
public:

	PathingState();

    void Init(std::vector<IPathFinder*> pathFinderlist, PathingState* parentState, unsigned int BLOCK_SIZE, const std::string& peFileName, const std::string& mapFileName);

    void Terminate();

	static void KillStatic();

	void AllocStateBuffer();

    bool RemoveCacheFile(const std::string& peFileName, const std::string& mapFileName);

    float GetMaxSpeedMod(unsigned int pathType) const { return maxSpeedMods[pathType]; };

    float GetVertexCost(size_t index) const { return vertexCosts[index]; };

	const std::vector<float>& GetVertexCosts() const { return vertexCosts; }
	const std::deque<int2>& GetUpdatedBlocks() const { return updatedBlocks; }

	struct SOffsetBlock {
		float cost;
		int2 offset;
		SOffsetBlock(const float _cost, const int x, const int y) : cost(_cost), offset(x,y) {}
	};
    const std::vector<SOffsetBlock>& getOffsetBlocksSortedByCost() const { return offsetBlocksSortedByCost; };

    int2 BlockIdxToPos(const unsigned idx) const { return int2(idx % mapDimensionsInBlocks.x, idx / mapDimensionsInBlocks.x); }
    int  BlockPosToIdx(const int2 pos) const { return (pos.y * mapDimensionsInBlocks.x + pos.x); }

	std::uint32_t CalcChecksum() const;
	std::uint32_t CalcHash(const char* caller) const;

	unsigned int GetBlockSize() const { return BLOCK_SIZE; }
	int2 GetNumBlocks() const { return nbrOfBlocks; }

    void Update();

	/**
	 * This is called whenever the ground structure of the map changes
	 * (for example on explosions and new buildings).
	 * The affected rectangular area is defined by (x1, z1)-(x2, z2).
	 * The estimator itself will decided if an update of the area is needed.
	 */
	void MapChanged(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);

	/**
	 * Returns a checksum that can be used to check if every player has the same
	 * path data.
	 */
	std::uint32_t GetPathChecksum() const { return pathChecksum; }

    // Re-entrant - return value cannot be a ref to avoid race conditions
	//const
	CPathCache::CacheItem GetCache(
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	) const {
		const std::lock_guard<std::mutex> lock(cacheAccessLock);
		return pathCache[synced]->GetCachedPath(strtBlock, goalBlock, goalRadius, pathType);
	}

    // Re-entrant, but not MT sync-safe
	void AddCache(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	);

	void AddPathForCurrentFrame(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const int2 strtBlock,
		const int2 goalBlock,
		float goalRadius,
		int pathType,
		const bool synced
	);

	void PromotePathForCurrentFrame(
		const IPath::Path* path,
		const IPath::SearchResult result,
		const float3 startPosition,
		const float3 goalPosition,
		float goalRadius,
		int pathType,
		const bool synced
	);

	PathNodeStateBuffer& GetNodeStateBuffer() { return blockStates; }

private:
	friend class TKPFS::CPathManager;
	friend struct ::TKPFSPathDrawer;

    void InitEstimator(const std::string& peFileName, const std::string& mapFileName);
    void InitBlocks();

    void CalcOffsetsAndPathCosts(unsigned int threadNum, spring::barrier* pathBarrier);
    void CalculateBlockOffsets(unsigned int, unsigned int);
    void EstimatePathCosts(unsigned int, unsigned int);

    int2 FindBlockPosOffset(const MoveDef&, unsigned int, unsigned int) const;
    void CalcVertexPathCosts(const MoveDef&, int2, unsigned int threadNum = 0);
    void CalcVertexPathCost(const MoveDef&, int2, unsigned int pathDir, unsigned int threadNum = 0);

	bool ReadFile(const std::string& peFileName, const std::string& mapFileName);
	bool WriteFile(const std::string& peFileName, const std::string& mapFileName);

private:
	friend class TKPFS::CPathEstimator;

    unsigned int BLOCK_SIZE = 0;
	unsigned int BLOCK_PIXEL_SIZE = 0;
    unsigned int BLOCKS_TO_UPDATE = 0;

    std::uint32_t pathChecksum = 0;
    std::uint32_t fileHashCode = 0;

    mutable std::mutex cacheAccessLock;

    int blockUpdatePenalty = 0;
	unsigned int instanceIndex = 0;

	std::atomic<std::int64_t> offsetBlockNum = {0};
	std::atomic<std::int64_t> costBlockNum = {0};

    unsigned int nextOffsetMessageIdx = 0;
    unsigned int nextCostMessageIdx = 0;

	//IPathFinder* parentPathFinder; // parent (PF if BLOCK_SIZE is 16, PE[16] if 32)
    PathingState* nextPathState = nullptr;

    CPathCache* pathCache[2]; // [0] = !synced, [1] = synced

    unsigned int mapBlockCount = 0;
    int2 mapDimensionsInBlocks = {0, 0};
	int2 nbrOfBlocks;

    //IPathFinder* pathFinders = nullptr;
    std::vector<IPathFinder*> pathFinders; // InitEstimator helpers
    //std::vector<spring::thread> threads;

    std::vector<float> maxSpeedMods;
    std::vector<float> vertexCosts;
    std::deque<int2> updatedBlocks;

    PathNodeStateBuffer blockStates;

	struct SingleBlock {
		int2 blockPos;
		const MoveDef* moveDef;
		SingleBlock(const int2& pos, const MoveDef* md) : blockPos(pos), moveDef(md) {}
	};

    std::vector<SingleBlock> consumedBlocks;
	std::vector<SOffsetBlock> offsetBlocksSortedByCost;

	int updatedBlocksDelayTimeout = 0;
	bool updatedBlocksDelayActive = false;
};

}

#endif
