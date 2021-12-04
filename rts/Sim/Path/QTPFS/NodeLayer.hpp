/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODELAYER_HDR
#define QTPFS_NODELAYER_HDR

#include <limits>
#include <vector>
#include <deque>
#include <cinttypes>

#include "System/Rectangle.h"
#include "Node.hpp"
#include "PathDefines.hpp"

struct MoveDef;

namespace QTPFS {
	struct INode;

	#ifdef QTPFS_STAGGERED_LAYER_UPDATES
	struct LayerUpdate {
		SRectangle rectangle;

		std::vector<float> speedMods;
		std::vector<int  > blockBits;

		unsigned int counter;
	};
	#endif

	struct NodeLayer {
	public:
		typedef unsigned char SpeedModType;
		typedef unsigned char SpeedBinType;

		static void InitStatic();
		static size_t MaxSpeedModTypeValue() { return (std::numeric_limits<SpeedModType>::max()); }
		static size_t MaxSpeedBinTypeValue() { return (std::numeric_limits<SpeedBinType>::max()); }

		NodeLayer() = default;
		NodeLayer(NodeLayer&& nl) = default;

		NodeLayer& operator = (const NodeLayer& nl) = delete;
		NodeLayer& operator = (NodeLayer&& nl) = default;

		void Init(unsigned int layerNum);
		void Clear();

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		void QueueUpdate(const SRectangle& r, const MoveDef* md);
		void PopQueuedUpdate() { layerUpdates.pop_front(); }
		bool ExecQueuedUpdate();
		bool HaveQueuedUpdate() const { return (!layerUpdates.empty()); }
		const LayerUpdate& GetQueuedUpdate() const { return (layerUpdates.front()); }
		unsigned int NumQueuedUpdates() const { return (layerUpdates.size()); }
		#endif

		bool Update(
			const SRectangle& r,
			const MoveDef* md,
			const std::vector<float>* luSpeedMods = nullptr,
			const std::vector<  int>* luBlockBits = nullptr
		);

		void ExecNodeNeighborCacheUpdate(unsigned int currFrameNum, unsigned int currMagicNum);
		void ExecNodeNeighborCacheUpdates(const SRectangle& ur, unsigned int currMagicNum);

		float GetNodeRatio() const { return (numLeafNodes / std::max(1.0f, float(xsize * zsize))); }
		const INode* GetNode(unsigned int x, unsigned int z) const { return nodeGrid[z * xsize + x]; }
		      INode* GetNode(unsigned int x, unsigned int z)       { return nodeGrid[z * xsize + x]; }
		const INode* GetNode(unsigned int i) const { return nodeGrid[i]; }
		      INode* GetNode(unsigned int i)       { return nodeGrid[i]; }

		const INode* GetPoolNode(unsigned int i) const { return &poolNodes[i / POOL_CHUNK_SIZE][i % POOL_CHUNK_SIZE]; }
		      INode* GetPoolNode(unsigned int i)       { return &poolNodes[i / POOL_CHUNK_SIZE][i % POOL_CHUNK_SIZE]; }

		INode* AllocRootNode(const INode* parent, unsigned int nn,  unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
			rootNode.Init(parent, nn, x1, z1, x2, z2);
			return &rootNode;
		}

		unsigned int AllocPoolNode(const INode* parent, unsigned int nn,  unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
			unsigned int idx = -1u;

			if (nodeIndcs.empty())
				return idx;

			if (poolNodes[(idx = nodeIndcs.back()) / POOL_CHUNK_SIZE].empty())
				poolNodes[idx / POOL_CHUNK_SIZE].resize(POOL_CHUNK_SIZE);

			poolNodes[idx / POOL_CHUNK_SIZE][idx % POOL_CHUNK_SIZE].Init(parent, nn, x1, z1, x2, z2);
			nodeIndcs.pop_back();

			return idx;
		}

		void FreePoolNode(unsigned int nodeIndex) { nodeIndcs.push_back(nodeIndex); }


		const std::vector<SpeedBinType>& GetOldSpeedBins() const { return oldSpeedBins; }
		const std::vector<SpeedBinType>& GetCurSpeedBins() const { return curSpeedBins; }
		const std::vector<SpeedModType>& GetOldSpeedMods() const { return oldSpeedMods; }
		const std::vector<SpeedModType>& GetCurSpeedMods() const { return curSpeedMods; }

		std::vector<INode*>& GetNodes() { return nodeGrid; }

		void RegisterNode(INode* n);

		void SetNumLeafNodes(unsigned int n) { numLeafNodes = n; }
		unsigned int GetNumLeafNodes() const { return numLeafNodes; }

		float GetMaxRelSpeedMod() const { return maxRelSpeedMod; }
		float GetAvgRelSpeedMod() const { return avgRelSpeedMod; }

		SpeedBinType GetSpeedModBin(float absSpeedMod, float relSpeedMod) const;

		std::uint64_t GetMemFootPrint() const {
			std::uint64_t memFootPrint = sizeof(NodeLayer);
			memFootPrint += (curSpeedMods.size() * sizeof(SpeedModType));
			memFootPrint += (oldSpeedMods.size() * sizeof(SpeedModType));
			memFootPrint += (curSpeedBins.size() * sizeof(SpeedBinType));
			memFootPrint += (oldSpeedBins.size() * sizeof(SpeedBinType));
			memFootPrint += (nodeGrid.size() * sizeof(decltype(nodeGrid)::value_type));
			// memFootPrint += (poolNodes.size() * sizeof(decltype(poolNodes)::value_type));
			for (size_t i = 0, n = NUM_POOL_CHUNKS; i < n; i++) {
				memFootPrint += (poolNodes[i].size() * sizeof(QTNode));
			}
			memFootPrint += (nodeIndcs.size() * sizeof(decltype(nodeIndcs)::value_type));
			return memFootPrint;
		}

	private:
		std::vector<INode*> nodeGrid;

		std::vector<QTNode> poolNodes[16];
		std::vector<unsigned int> nodeIndcs;

		std::vector<SpeedModType> curSpeedMods;
		std::vector<SpeedModType> oldSpeedMods;
		std::vector<SpeedBinType> curSpeedBins;
		std::vector<SpeedBinType> oldSpeedBins;

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		std::deque<LayerUpdate> layerUpdates;
		#endif

		// root lives outside pool s.t. all four children of a given node are always in one chunk
		QTNode rootNode;

		static constexpr unsigned int NUM_POOL_CHUNKS = sizeof(poolNodes) / sizeof(poolNodes[0]);
		static constexpr unsigned int POOL_TOTAL_SIZE = (1024 * 1024) / 2;
		static constexpr unsigned int POOL_CHUNK_SIZE = POOL_TOTAL_SIZE / NUM_POOL_CHUNKS;

		// NOTE:
		//   we need a fixed range that does not become wider / narrower
		//   during terrain deformations (otherwise the bins would change
		//   across ALL nodes)
		static unsigned int NUM_SPEEDMOD_BINS;
		static float        MIN_SPEEDMOD_VALUE;
		static float        MAX_SPEEDMOD_VALUE;

		unsigned int layerNumber = 0;
		unsigned int numLeafNodes = 0;
		unsigned int updateCounter = 0;

		unsigned int xsize = 0;
		unsigned int zsize = 0;

		float maxRelSpeedMod = 0.0f;
		float avgRelSpeedMod = 0.0f;
	};
}

#endif

