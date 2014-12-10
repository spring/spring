/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <vector>
#include <fstream>
#include <boost/cstdint.hpp>

#include "PathEnums.hpp"
#include "PathDefines.hpp"

#include "System/float3.h"
#include "System/Rectangle.h"

#ifndef QTPFS_VIRTUAL_NODE_FUNCTIONS
#define QTNode INode
#endif

namespace QTPFS {
	struct NodeLayer;
	struct INode {
	public:
		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		void SetHeapIndex(unsigned int n) { heapIndex = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }
		unsigned int GetHeapIndex() const { return heapIndex; }
		float GetHeapPriority() const { return GetPathCost(NODE_PATH_COST_F); }

		bool operator <  (const INode* n) const { return (fCost <  n->fCost); }
		bool operator >  (const INode* n) const { return (fCost >  n->fCost); }
		bool operator == (const INode* n) const { return (fCost == n->fCost); }
		bool operator <= (const INode* n) const { return (fCost <= n->fCost); }
		bool operator >= (const INode* n) const { return (fCost >= n->fCost); }

		#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
		virtual void Serialize(std::fstream&, NodeLayer&, unsigned int*, bool) = 0;
		virtual unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&) = 0;
		virtual const std::vector<INode*>& GetNeighbors(const std::vector<INode*>& v) = 0;
		virtual bool UpdateNeighborCache(const std::vector<INode*>& nodes) = 0;

		virtual void SetNeighborEdgeTransitionPoint(unsigned int ngbIdx, const float3& point) = 0;
		virtual const float3& GetNeighborEdgeTransitionPoint(unsigned int ngbIdx) const = 0;
		#endif

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float3 GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
		virtual unsigned int xmin() const = 0;
		virtual unsigned int zmin() const = 0;
		virtual unsigned int xmax() const = 0;
		virtual unsigned int zmax() const = 0;
		virtual unsigned int xmid() const = 0;
		virtual unsigned int zmid() const = 0;
		virtual unsigned int xsize() const = 0;
		virtual unsigned int zsize() const = 0;
		virtual unsigned int area() const = 0;

		virtual bool AllSquaresAccessible() const = 0;
		virtual bool AllSquaresImpassable() const = 0;

		virtual void SetMoveCost(float cost) = 0;
		virtual float GetMoveCost() const = 0;

		virtual void SetSearchState(unsigned int) = 0;
		virtual unsigned int GetSearchState() const = 0;

		virtual void SetMagicNumber(unsigned int) = 0;
		virtual unsigned int GetMagicNumber() const = 0;
		#endif

		void SetPathCosts(float g, float h) { fCost = g + h; gCost = g; hCost = h; }
		void SetPathCost(unsigned int type, float cost);
		const float* GetPathCosts() const { return &fCost; }
		float GetPathCost(unsigned int type) const;

		void SetPrevNode(INode* n) { prevNode = n; }
		INode* GetPrevNode() { return prevNode; }

	protected:
		// NOTE:
		//     storing the heap-index is an *UGLY* break of abstraction,
		//     but the only way to keep the cost of resorting acceptable
		unsigned int nodeNumber;
		unsigned int heapIndex;

		float fCost;
		float gCost;
		float hCost;

		// points back to previous node in path
		INode* prevNode;

	#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
	};
	#endif



	#ifdef QTPFS_VIRTUAL_NODE_FUNCTIONS
	struct QTNode: public INode {
	#endif
	public:
		QTNode(
			const QTNode* parent,
			unsigned int nn,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2
		);
		~QTNode();

		static void InitStatic();

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i) const { return (nodeNumber << 2) + (i + 1); }
		unsigned int GetParentID() const { return ((nodeNumber - 1) >> 2); }

		boost::uint64_t GetMemFootPrint() const;
		boost::uint64_t GetCheckSum() const;

		void Delete();
		void PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur);
		void Tesselate(NodeLayer& nl, const SRectangle& r);
		void Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, bool readMode);

		bool IsLeaf() const;
		bool CanSplit(bool forced) const;

		bool Split(NodeLayer& nl, bool forced);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		const std::vector<INode*>& GetNeighbors(const std::vector<INode*>&);
		bool UpdateNeighborCache(const std::vector<INode*>& nodes);

		void SetNeighborEdgeTransitionPoint(unsigned int ngbIdx, const float3& point) { netpoints[ngbIdx] = point; }
		const float3& GetNeighborEdgeTransitionPoint(unsigned int ngbIdx) const { return netpoints[ngbIdx]; }

		unsigned int xmin() const { return (_xminxmax  & 0xFFFF); }
		unsigned int zmin() const { return (_zminzmax  & 0xFFFF); }
		unsigned int xmax() const { return (_xminxmax >>     16); }
		unsigned int zmax() const { return (_zminzmax >>     16); }
		unsigned int xmid() const { return ((xmin() + xmax()) >> 1); }
		unsigned int zmid() const { return ((zmin() + zmax()) >> 1); }
		unsigned int xsize() const { return (xmax() - xmin()); }
		unsigned int zsize() const { return (zmax() - zmin()); }
		unsigned int depth() const { return _depth; }
		unsigned int area() const { return (xsize() * zsize()); }

		// true iff this node is fully open (partially open nodes have larger but non-infinite cost)
		bool AllSquaresAccessible() const { return (moveCostAvg < (QTPFS_CLOSED_NODE_COST / float(area()))); }
		bool AllSquaresImpassable() const { return (moveCostAvg == QTPFS_POSITIVE_INFINITY); }

		void SetMoveCost(float cost) { moveCostAvg = cost; }
		float GetMoveCost() const { return moveCostAvg; }

		void SetSearchState(unsigned int state) { searchState = state; }
		unsigned int GetSearchState() const { return searchState; }

		void SetMagicNumber(unsigned int number) { currMagicNum = number; }
		unsigned int GetMagicNumber() const { return currMagicNum; }

		static unsigned int MinSizeX() { return MIN_SIZE_X; }
		static unsigned int MinSizeZ() { return MIN_SIZE_Z; }

	private:
		bool UpdateMoveCost(
			const NodeLayer& nl,
			const SRectangle& r,
			unsigned int& numNewBinSquares,
			unsigned int& numDifBinSquares,
			unsigned int& numClosedSquares,
			bool& wantSplit,
			bool& needSplit
		);

		static unsigned int MIN_SIZE_X;
		static unsigned int MIN_SIZE_Z;
		static unsigned int MAX_DEPTH;

		unsigned int _depth;
		unsigned int _xminxmax;
		unsigned int _zminzmax;

		float speedModSum;
		float speedModAvg;
		float moveCostAvg;

		unsigned int searchState;
		unsigned int currMagicNum;
		unsigned int prevMagicNum;

		std::vector<QTNode*> children;
		std::vector<INode*> neighbors;

		// NOTE:
		//   these should be float2's, but profiling shows float3's to be *faster* and
		//   float3's are also more convenient to work with (so we take the memory hit)
		std::vector<float3> netpoints;
	};
}

#endif

