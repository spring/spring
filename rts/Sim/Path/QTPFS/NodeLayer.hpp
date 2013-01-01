/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODELAYER_HDR
#define QTPFS_NODELAYER_HDR

#include <vector>
#include <list> // for QTPFS_STAGGERED_LAYER_UPDATES
#include <boost/cstdint.hpp>

#include "PathDefines.hpp"
#include "PathRectangle.hpp"

struct MoveDef;

namespace QTPFS {
	struct PathRectangle;
	struct INode;

	#ifdef QTPFS_STAGGERED_LAYER_UPDATES
	struct LayerUpdate {
		PathRectangle rectangle;

		std::vector<float> speedMods;
		std::vector<int  > blockBits;

		unsigned int counter;
	};
	#endif

	struct NodeLayer {
	public:
		NodeLayer()
			: numLeafNodes(0)
			, layerNumber(0)
			, updateCounter(0)
			, xsize(0)
			, zsize(0)
			{}

		void Init(unsigned int layerNum);
		void Clear();

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		void QueueUpdate(const PathRectangle& r, const MoveDef* md);
		void PopQueuedUpdate() { layerUpdates.pop_front(); }
		bool ExecQueuedUpdate();
		bool HaveQueuedUpdate() const { return (!layerUpdates.empty()); }
		const LayerUpdate& GetQueuedUpdate() const { return (layerUpdates.front()); }
		#endif

		bool Update(
			const PathRectangle& r,
			const MoveDef* md,
			const std::vector<float>* luSpeedMods = NULL,
			const std::vector<  int>* luBlockBits = NULL
		);

		void ExecNodeNeighborCacheUpdate(unsigned int currFrameNum, unsigned int currMagicNum);
		void ExecNodeNeighborCacheUpdates(const PathRectangle& ur, unsigned int currMagicNum);

		float GetNodeRatio() const { return (numLeafNodes / float(xsize * zsize)); }
		const INode* GetNode(unsigned int x, unsigned int z) const { return nodeGrid[z * xsize + x]; }
		      INode* GetNode(unsigned int x, unsigned int z)       { return nodeGrid[z * xsize + x]; }
		const INode* GetNode(unsigned int i) const { return nodeGrid[i]; }
		      INode* GetNode(unsigned int i)       { return nodeGrid[i]; }

		const std::vector<unsigned char >& GetOldSpeedBins() const { return oldSpeedBins; }
		const std::vector<unsigned char >& GetCurSpeedBins() const { return curSpeedBins; }
		const std::vector<unsigned short>& GetOldSpeedMods() const { return oldSpeedMods; }
		const std::vector<unsigned short>& GetCurSpeedMods() const { return curSpeedMods; }

		std::vector<INode*>& GetNodes() { return nodeGrid; }
		void RegisterNode(INode* n);

		void SetNumLeafNodes(unsigned int n) { numLeafNodes = n; }
		unsigned int GetNumLeafNodes() const { return numLeafNodes; }

		unsigned int GetLayerNumber() const { return layerNumber; }

		boost::uint64_t GetMemFootPrint() const {
			boost::uint64_t memFootPrint = sizeof(NodeLayer);
			memFootPrint += (curSpeedMods.size() * sizeof(unsigned short));
			memFootPrint += (oldSpeedMods.size() * sizeof(unsigned short));
			memFootPrint += (curSpeedBins.size() * sizeof(unsigned  char));
			memFootPrint += (oldSpeedBins.size() * sizeof(unsigned  char));
			memFootPrint += (nodeGrid.size() * sizeof(INode*));
			return memFootPrint;
		}

		// NOTE:
		//   we need a fixed range that does not become wider / narrower
		//   during terrain deformations (otherwise the bins would change
		//   across ALL nodes)
		static const float MIN_SPEEDMOD_VALUE = 0.0f;
		static const float MAX_SPEEDMOD_VALUE = 2.0f;

	private:
		std::vector<INode*> nodeGrid;

		std::vector<unsigned short> curSpeedMods;
		std::vector<unsigned short> oldSpeedMods;
		std::vector<unsigned  char> curSpeedBins;
		std::vector<unsigned  char> oldSpeedBins;

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		std::list<LayerUpdate> layerUpdates;
		#endif

		unsigned int numLeafNodes;
		unsigned int layerNumber;
		unsigned int updateCounter;

		unsigned int xsize;
		unsigned int zsize;
	};
};

#endif

