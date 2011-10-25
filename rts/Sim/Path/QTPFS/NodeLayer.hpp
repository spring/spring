/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODELAYER_HDR
#define QTPFS_NODELAYER_HDR

#include <vector>

struct SRectangle;
struct MoveData;
struct CMoveMath;

namespace QTPFS {
	struct INode;
	struct NodeLayer {
	public:
		void Init(unsigned int n);
		bool Update(const SRectangle& r, const MoveData* md, const CMoveMath* mm);
		void Clear();

		float GetNodeRatio() const { return (numLeafNodes / float(xsize * zsize)); }
		const INode* GetNode(unsigned int x, unsigned int z) const { return nodeGrid[z * xsize + x]; }
		      INode* GetNode(unsigned int x, unsigned int z)       { return nodeGrid[z * xsize + x]; }
		const INode* GetNode(unsigned int i) const { return nodeGrid[i]; }
		      INode* GetNode(unsigned int i)       { return nodeGrid[i]; }

		const std::vector<int>& GetOldSpeedBins() const { return oldSpeedBins; }
		const std::vector<int>& GetCurSpeedBins() const { return curSpeedBins; }
		const std::vector<float>& GetOldSpeedMods() const { return oldSpeedMods; }
		const std::vector<float>& GetCurSpeedMods() const { return curSpeedMods; }

		std::vector<INode*>& GetNodes() { return nodeGrid; }
		void RegisterNode(INode* n);

		void SetNumLeafNodes(unsigned int n) { numLeafNodes = n; }
		unsigned int GetNumLeafNodes() const { return numLeafNodes; }

		unsigned int GetLayerNumber() const { return layerNumber; }
	private:
		std::vector<INode*> nodeGrid;

		std::vector<float> curSpeedMods;
		std::vector<float> oldSpeedMods;
		std::vector<int  > curSpeedBins;
		std::vector<int  > oldSpeedBins;

		unsigned int numLeafNodes;
		unsigned int layerNumber;

		unsigned int xsize;
		unsigned int zsize;
	};
};

#endif

