/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <vector>
#include <fstream>
#include <boost/cstdint.hpp>

#include "PathEnums.hpp"
#include "System/float3.h"

struct SRectangle;

namespace QTPFS {
	struct NodeLayer;
	struct INode {
		unsigned int GetNodeNumber() const { return nodeNumber; }

		bool operator < (const INode* n) const { return (fCost < n->fCost); }
		bool operator () (const INode* a, const INode* b) const { return (a->fCost > b->fCost); }

		// NOTE:
		//     not pure virtuals, because INode is used as comparator in
		//     std::pqueue and STL does not allow abstract types for that
		virtual void Serialize(std::fstream&, bool) { assert(false); }
		virtual unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&) { assert(false); return 0; }
		virtual const std::vector<INode*>& GetNeighbors(const std::vector<INode*>& v) { assert(false); return v; }

		unsigned int GetNeighborRelation(const INode* ngb) const;
		unsigned int GetRectangleRelation(const SRectangle& r) const;
		float GetDistance(const INode* n, unsigned int type) const;
		float3 GetNeighborEdgeMidPoint(const INode* ngb) const;
		SRectangle ClipRectangle(const SRectangle& r) const;

		virtual unsigned int xmin() const { assert(false); return 0; }
		virtual unsigned int zmin() const { assert(false); return 0; }
		virtual unsigned int xmax() const { assert(false); return 1; }
		virtual unsigned int zmax() const { assert(false); return 1; }
		virtual unsigned int xmid() const { assert(false); return 0; }
		virtual unsigned int zmid() const { assert(false); return 0; }
		virtual unsigned int xsize() const { assert(false); return 1; }
		virtual unsigned int zsize() const { assert(false); return 1; }

		virtual float GetMoveCost() const { assert(false); return 1.0f; }

		virtual void SetSearchState(unsigned int) { assert(false); }
		virtual unsigned int GetSearchState() const { assert(false); return 0; }

		virtual void SetMagicNumber(unsigned int) { assert(false); }
		virtual unsigned int GetMagicNumber() const { assert(false); return 0; }

		void SetPathCost(unsigned int type, float cost);
		float GetPathCost(unsigned int type) const;

		void SetPrevNode(INode* n) { prevNode = n; }
		INode* GetPrevNode() { return prevNode; }

	protected:
		unsigned int nodeNumber;

		float fCost;
		float gCost;
		float hCost;

		// points back to previous node in path
		INode* prevNode;
	};



	struct QTNode: public INode {
		QTNode(
			const QTNode* parent,
			unsigned int id,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2
		);
		~QTNode();

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i) const { return (nodeNumber << 2) + (i + 1); }
		unsigned int GetParentID() const { return ((nodeNumber - 1) >> 2); }
		unsigned int GetDepth() const;

		boost::uint64_t GetMemFootPrint() const;

		void Delete();
		void PreTesselate(NodeLayer& nl, const SRectangle& r);
		void Tesselate(NodeLayer& nl, const SRectangle& r, bool merged, bool split);
		void Serialize(std::fstream& fStream, bool read);

		bool IsLeaf() const;
		bool Split(NodeLayer& nl);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		const std::vector<INode*>& GetNeighbors(const std::vector<INode*>&);
		float GetMoveCost() const { return moveCostAvg; }

		unsigned int xmin() const { return _xmin; }
		unsigned int zmin() const { return _zmin; }
		unsigned int xmax() const { return _xmax; }
		unsigned int zmax() const { return _zmax; }
		unsigned int xmid() const { return _xmid; }
		unsigned int zmid() const { return _zmid; }
		unsigned int xsize() const { return _xsize; }
		unsigned int zsize() const { return _zsize; }

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
			unsigned int& numDifBinSquares
		);
		bool UpdateNeighborCache(const std::vector<INode*>& nodes);

		unsigned int _xmin, _xmax, _xmid, _xsize;
		unsigned int _zmin, _zmax, _zmid, _zsize;

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

