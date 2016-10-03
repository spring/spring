/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODELAYER_HDR
#define QTPFS_NODELAYER_HDR

#include <limits>
#include <vector>
#include <list> // for QTPFS_STAGGERED_LAYER_UPDATES
#include <cinttypes>

#include "System/Rectangle.h"
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

		NodeLayer();

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
			const std::vector<float>* luSpeedMods = NULL,
			const std::vector<  int>* luBlockBits = NULL
		);

		void ExecNodeNeighborCacheUpdate(unsigned int currFrameNum, unsigned int currMagicNum);
		void ExecNodeNeighborCacheUpdates(const SRectangle& ur, unsigned int currMagicNum);

		float GetNodeRatio() const { return (numLeafNodes / std::max(1.0f, float(xsize * zsize))); }
		const INode* GetNode(unsigned int x, unsigned int z) const { return nodeGrid[z * xsize + x]; }
		      INode* GetNode(unsigned int x, unsigned int z)       { return nodeGrid[z * xsize + x]; }
		const INode* GetNode(unsigned int i) const { return nodeGrid[i]; }
		      INode* GetNode(unsigned int i)       { return nodeGrid[i]; }

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
			memFootPrint += (nodeGrid.size() * sizeof(INode*));
			return memFootPrint;
		}

	private:
		std::vector<INode*> nodeGrid;

		std::vector<SpeedModType> curSpeedMods;
		std::vector<SpeedModType> oldSpeedMods;
		std::vector<SpeedBinType> curSpeedBins;
		std::vector<SpeedBinType> oldSpeedBins;

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		std::list<LayerUpdate> layerUpdates;
		#endif

		// NOTE:
		//   we need a fixed range that does not become wider / narrower
		//   during terrain deformations (otherwise the bins would change
		//   across ALL nodes)
		static unsigned int NUM_SPEEDMOD_BINS;
		static float        MIN_SPEEDMOD_VALUE;
		static float        MAX_SPEEDMOD_VALUE;

		unsigned int layerNumber;
		unsigned int numLeafNodes;
		unsigned int updateCounter;

		unsigned int xsize;
		unsigned int zsize;

		float maxRelSpeedMod;
		float avgRelSpeedMod;
	};
}

#endif

