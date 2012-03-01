/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <vector>
#include <fstream>
#include <boost/cstdint.hpp>

#include "PathEnums.hpp"
#include "PathDefines.hpp"
#include "System/float3.h"

struct SRectangle;

namespace QTPFS {
	struct NodeLayer;
	struct INode {
		void SetNodeNumber(unsigned int n) { nodeNumber = n; }
		void SetHeapIndex(unsigned int n) { heapIndex = n; }
		unsigned int GetNodeNumber() const { return nodeNumber; }
		unsigned int GetHeapIndex() const { return heapIndex; }

		#ifdef QTPFS_WEIGHTED_HEURISTIC_COST
		void SetNumPrevNodes(unsigned int n) { numPrevNodes = n; }
		unsigned int GetNumPrevNodes() const { return numPrevNodes; }
		#endif

		bool operator <  (const INode* n) const { return (fCost <  n->fCost); }
		bool operator >  (const INode* n) const { return (fCost >  n->fCost); }
		bool operator == (const INode* n) const { return (fCost == n->fCost); }
		bool operator <= (const INode* n) const { return (fCost <= n->fCost); }
		bool operator >= (const INode* n) const { return (fCost >= n->fCost); }

		virtual void Serialize(std::fstream&, bool) = 0;
		virtual unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&) = 0;
		virtual const std::vector<INode*>& GetNeighbors(const std::vector<INode*>& v) = 0;

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float3 GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		virtual unsigned int xmin() const = 0;
		virtual unsigned int zmin() const = 0;
		virtual unsigned int xmax() const = 0;
		virtual unsigned int zmax() const = 0;
		virtual unsigned int xmid() const = 0;
		virtual unsigned int zmid() const = 0;
		virtual unsigned int xsize() const = 0;
		virtual unsigned int zsize() const = 0;

		virtual void SetMoveCost(float cost) = 0;
		virtual float GetMoveCost() const = 0;

		virtual void SetSearchState(unsigned int) = 0;
		virtual unsigned int GetSearchState() const = 0;

		virtual void SetMagicNumber(unsigned int) = 0;
		virtual unsigned int GetMagicNumber() const = 0;

		void SetPathCost(unsigned int type, float cost);
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
		float mCost;

		#ifdef QTPFS_WEIGHTED_HEURISTIC_COST
		unsigned int numPrevNodes;
		#endif

		// points back to previous node in path
		INode* prevNode;
	};



	struct QTNode: public INode {
		QTNode(
			const QTNode* parent,
			unsigned int nn,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2
		);
		~QTNode();

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i) const { return (nodeNumber << 2) + (i + 1); }
		unsigned int GetParentID() const { return ((nodeNumber - 1) >> 2); }

		boost::uint64_t GetMemFootPrint() const;

		void Delete();
		void PreTesselate(NodeLayer& nl, const SRectangle& r);
		void Tesselate(NodeLayer& nl, const SRectangle& r, bool merged, bool split);
		void Serialize(std::fstream& fStream, bool read);

		bool IsLeaf() const;
		bool CanSplit() const;

		bool Split(NodeLayer& nl);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		const std::vector<INode*>& GetNeighbors(const std::vector<INode*>&);

		void SetMoveCost(float cost) { moveCostAvg = cost; }
		float GetMoveCost() const { return moveCostAvg; }

		unsigned int xmin() const { return _xmin; }
		unsigned int zmin() const { return _zmin; }
		unsigned int xmax() const { return _xmax; }
		unsigned int zmax() const { return _zmax; }
		unsigned int xmid() const { return ((_xmin + _xmax) >> 1); }
		unsigned int zmid() const { return ((_zmin + _zmax) >> 1); }
		unsigned int xsize() const { return (_xmax - _xmin); }
		unsigned int zsize() const { return (_zmax - _zmin); }
		unsigned int depth() const { return _depth; }

		void SetSearchState(unsigned int state) { searchState = state; }
		unsigned int GetSearchState() const { return searchState; }

		void SetMagicNumber(unsigned int number) { currMagicNum = number; }
		unsigned int GetMagicNumber() const { return currMagicNum; }

		static const unsigned int CHILD_COUNT = 4;
		static const unsigned int MIN_SIZE_X = 2;
		static const unsigned int MIN_SIZE_Z = 2;
		static const unsigned int MAX_DEPTH = 16;

	private:
		void UpdateMoveCost(
			const NodeLayer& nl,
			const SRectangle& r,
			unsigned int& numNewBinSquares,
			unsigned int& numDifBinSquares,
			unsigned int& numClosedSquares
		);
		bool UpdateNeighborCache(const std::vector<INode*>& nodes);

		unsigned int _xmin, _xmax;
		unsigned int _zmin, _zmax;
		unsigned int _depth;

		float speedModSum;
		float speedModAvg;
		float moveCostAvg;

		unsigned int searchState;
		unsigned int currMagicNum;
		unsigned int prevMagicNum;

		std::vector<QTNode*> children;
		std::vector<INode*> neighbors;
	};
};

#endif

