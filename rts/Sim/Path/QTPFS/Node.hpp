/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_NODE_HDR
#define QTPFS_NODE_HDR

#include <vector>
#include <fstream>

#include "Enums.hpp"
#include "System/float3.h"

struct SRectangle;

namespace QTPFS {
	struct NodeLayer;
	struct INode {
		bool operator < (const INode* n) const { return (fCost < n->fCost); }
		bool operator () (const INode* a, const INode* b) const { return (a->fCost > b->fCost); }

		virtual void Serialize(std::fstream&, bool) { assert(false); }
		virtual unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&) { return 0; }

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

		void SetPathCost(unsigned int type, float cost);
		float GetPathCost(unsigned int type) const;

		void SetSearchState(unsigned int state) { searchState = state; }
		unsigned int GetSearchState() const { return searchState; }

		void SetPrevNode(INode* n) { prevNode = n; }
		const INode* GetPrevNode() const { return prevNode; }

	protected:
		unsigned int identifier;
		unsigned int searchState;

		float fCost;
		float gCost;
		float hCost;

		// points back to previous node in path
		INode* prevNode;
	};



	struct QTNode: public INode {
		QTNode(
			unsigned int id,
			unsigned int x1, unsigned int z1,
			unsigned int x2, unsigned int z2
		);
		~QTNode();

		// NOTE:
		//     root-node identifier is always 0
		//     <i> is a NODE_IDX index in [0, 3]
		unsigned int GetChildID(unsigned int i) const { return (identifier << 2) + (i + 1); }
		unsigned int GetParentID() const { return ((identifier - 1) >> 2); }

		void Delete();
		void PreTesselate(NodeLayer& nl, const SRectangle& r);
		void Tesselate(NodeLayer& nl, const SRectangle& r);
		void Serialize(std::fstream& fStream, bool read);

		bool IsLeaf() const;
		bool Split(NodeLayer& nl);
		bool Merge(NodeLayer& nl);

		unsigned int GetMaxNumNeighbors() const;
		unsigned int GetNeighbors(const std::vector<INode*>&, std::vector<INode*>&);
		float GetMoveCost() const { return moveCostAvg; }

		unsigned int xmin() const { return _xmin; }
		unsigned int zmin() const { return _zmin; }
		unsigned int xmax() const { return _xmax; }
		unsigned int zmax() const { return _zmax; }
		unsigned int xmid() const { return _xmid; }
		unsigned int zmid() const { return _zmid; }
		unsigned int xsize() const { return _xsize; }
		unsigned int zsize() const { return _zsize; }

		static const unsigned int CHILD_COUNT = 4;
	private:
		unsigned int _xmin, _xmax, _xmid, _xsize;
		unsigned int _zmin, _zmax, _zmid, _zsize;

		int tempNum;

		float speedModAvg;
		float speedModSum;
		float moveCostAvg;

		std::vector<QTNode*> children;
		std::vector<QTNode*> neighbors;
	};
};

#endif

