/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <limits>

#include "lib/streflop/streflop_cond.h"

#include "Node.hpp"
#include "NodeLayer.hpp"
#include "PathDefines.hpp"
#include "PathManager.hpp"

#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"

unsigned int QTPFS::QTNode::MIN_SIZE_X;
unsigned int QTPFS::QTNode::MIN_SIZE_Z;
unsigned int QTPFS::QTNode::MAX_DEPTH;



void QTPFS::INode::SetPathCost(unsigned int type, float cost) {
	#ifndef QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
	switch (type) {
		case NODE_PATH_COST_F: { fCost = cost; return; } break;
		case NODE_PATH_COST_G: { gCost = cost; return; } break;
		case NODE_PATH_COST_H: { hCost = cost; return; } break;
	}

	assert(false);
	#else
	assert(&gCost == &fCost + 1);
	assert(&hCost == &gCost + 1);
	assert(type <= NODE_PATH_COST_H);

	*(&fCost + type) = cost;
	#endif
}

float QTPFS::INode::GetPathCost(unsigned int type) const {
	#ifndef QTPFS_ENABLE_MICRO_OPTIMIZATION_HACKS
	switch (type) {
		case NODE_PATH_COST_F: { return fCost; } break;
		case NODE_PATH_COST_G: { return gCost; } break;
		case NODE_PATH_COST_H: { return hCost; } break;
	}

	assert(false);
	return 0.0f;
	#else
	assert(&gCost == &fCost + 1);
	assert(&hCost == &gCost + 1);
	assert(type <= NODE_PATH_COST_H);

	return *(&fCost + type);
	#endif
}

float QTPFS::INode::GetDistance(const INode* n, unsigned int type) const {
	const float dx = float(xmid() * SQUARE_SIZE) - float(n->xmid() * SQUARE_SIZE);
	const float dz = float(zmid() * SQUARE_SIZE) - float(n->zmid() * SQUARE_SIZE);

	switch (type) {
		case NODE_DIST_EUCLIDEAN: { return (math::sqrt((dx * dx) + (dz * dz))); } break;
		case NODE_DIST_MANHATTAN: { return (math::fabs(dx) + math::fabs(dz)); } break;
	}

	return -1.0f;
}

unsigned int QTPFS::INode::GetNeighborRelation(const INode* ngb) const {
	unsigned int rel = 0;

	rel |= ((xmin() == ngb->xmax()) * REL_NGB_EDGE_L);
	rel |= ((xmax() == ngb->xmin()) * REL_NGB_EDGE_R);
	rel |= ((zmin() == ngb->zmax()) * REL_NGB_EDGE_T);
	rel |= ((zmax() == ngb->zmin()) * REL_NGB_EDGE_B);

	return rel;
}

unsigned int QTPFS::INode::GetRectangleRelation(const SRectangle& r) const {
	// NOTE: we consider "interior" to be the set of all
	// legal indices, and conversely "exterior" the set
	// of all illegal indices (min-edges are inclusive,
	// max-edges are exclusive)
	//
	if ((r.x1 >= xmin() && r.x2 <  xmax()) && (r.z1 >= zmin() && r.z2 <  zmax())) { return REL_RECT_INTERIOR_NODE; }
	if ((r.x1 >= xmax() || r.x2 <  xmin()) || (r.z1 >= zmax() || r.z2 <  zmin())) { return REL_RECT_EXTERIOR_NODE; }
	if ((r.x1 <  xmin() && r.x2 >= xmax()) && (r.z1 <  zmin() && r.z2 >= zmax())) { return REL_NODE_INTERIOR_RECT; }

	return REL_NODE_OVERLAPS_RECT;
}

float3 QTPFS::INode::GetNeighborEdgeTransitionPoint(const INode* ngb, const float3& pos, float alpha) const {
	float3 p;

	const unsigned int
		minx = std::max(xmin(), ngb->xmin()),
		maxx = std::min(xmax(), ngb->xmax());
	const unsigned int
		minz = std::max(zmin(), ngb->zmin()),
		maxz = std::min(zmax(), ngb->zmax());

	// NOTE:
	//     do not use integer arithmetic for the mid-points,
	//     the path-backtrace expects all waypoints to have
	//     unique world-space coordinates (ortho-projection
	//     mode is broken in that regard) and this would not
	//     hold for a path through multiple neighboring nodes
	//     with xsize and/or zsize equal to 1 heightmap square
	#ifndef QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
	const float
		midx = minx * (1.0f - alpha) + maxx * (0.0f + alpha),
		midz = minz * (1.0f - alpha) + maxz * (0.0f + alpha);
	#endif

	switch (GetNeighborRelation(ngb)) {
		// corners
		case REL_NGB_EDGE_T | REL_NGB_EDGE_L: { p.x = xmin() * SQUARE_SIZE; p.z = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_T | REL_NGB_EDGE_R: { p.x = xmax() * SQUARE_SIZE; p.z = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B | REL_NGB_EDGE_R: { p.x = xmax() * SQUARE_SIZE; p.z = zmax() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B | REL_NGB_EDGE_L: { p.x = xmin() * SQUARE_SIZE; p.z = zmax() * SQUARE_SIZE; } break;

		#ifdef QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
		#define CAST static_cast<unsigned int>

		// edges
		// clamp <pos> (assumed to be inside <this>) to
		// the shared-edge bounds and ortho-project it
		case REL_NGB_EDGE_T: { p.x = Clamp(CAST(pos.x / SQUARE_SIZE), minx, maxx) * SQUARE_SIZE; p.z = minz * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B: { p.x = Clamp(CAST(pos.x / SQUARE_SIZE), minx, maxx) * SQUARE_SIZE; p.z = maxz * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_R: { p.z = Clamp(CAST(pos.z / SQUARE_SIZE), minz, maxz) * SQUARE_SIZE; p.x = maxx * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_L: { p.z = Clamp(CAST(pos.z / SQUARE_SIZE), minz, maxz) * SQUARE_SIZE; p.x = minx * SQUARE_SIZE; } break;

		// <ngb> had better be an actual neighbor
		case 0: { assert(false); } break;

		#undef CAST
		#else

		// edges
		case REL_NGB_EDGE_T:                  { p.x = midx   * SQUARE_SIZE; p.z = zmin() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_R:                  { p.x = xmax() * SQUARE_SIZE; p.z = midz   * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_B:                  { p.x = midx   * SQUARE_SIZE; p.z = zmax() * SQUARE_SIZE; } break;
		case REL_NGB_EDGE_L:                  { p.x = xmin() * SQUARE_SIZE; p.z = midz   * SQUARE_SIZE; } break;

		// <ngb> had better be an actual neighbor
		case 0: { assert(false); } break;

		#endif
	}

	return p;
}

// clip an OVERLAPPING rectangle against our boundaries
//
// NOTE:
//     the rectangle is only ASSUMED to not lie completely
//     inside <this> (in which case this function acts as
//     no-op), we do not explicitly test
//
//     both REL_RECT_EXTERIOR_NODE and REL_NODE_OVERLAPS_RECT
//     relations can produce zero- or negative-area rectangles
//     when clipping --> need to ensure to not leave move-cost
//     at its default value (0.0, which no node can logically
//     have)
//
SRectangle QTPFS::INode::ClipRectangle(const SRectangle& r) const {
	SRectangle cr = r;
	cr.x1 = std::max(int(xmin()), r.x1);
	cr.z1 = std::max(int(zmin()), r.z1);
	cr.x2 = std::min(int(xmax()), r.x2);
	cr.z2 = std::min(int(zmax()), r.z2);
	return cr;
}






void QTPFS::QTNode::InitStatic() {
	MIN_SIZE_X = std::max(1u, mapInfo->pfs.qtpfs_constants.minNodeSizeX);
	MIN_SIZE_Z = std::max(1u, mapInfo->pfs.qtpfs_constants.minNodeSizeZ);
	MAX_DEPTH  = std::max(1u, mapInfo->pfs.qtpfs_constants.maxNodeDepth);
}

QTPFS::QTNode::QTNode(
	const QTNode* parent,
	unsigned int nn,
	unsigned int x1, unsigned int z1,
	unsigned int x2, unsigned int z2
) {
	assert(MIN_SIZE_X > 0);
	assert(MIN_SIZE_Z > 0);

	nodeNumber = nn;
	heapIndex = -1u;

	searchState  =   0;
	currMagicNum =   0;
	prevMagicNum = -1u;

	_depth = (parent != NULL)? parent->depth() + 1: 0;
	_xminxmax = (x2 << 16) | (x1 << 0);
	_zminzmax = (z2 << 16) | (z1 << 0);

	assert(x2 < (1 << 16));
	assert(z2 < (1 << 16));
	assert(xsize() != 0);
	assert(zsize() != 0);

	fCost = 0.0f;
	gCost = 0.0f;
	hCost = 0.0f;

	speedModSum =  0.0f;
	speedModAvg =  0.0f;
	moveCostAvg = -1.0f;

	prevNode = NULL;

	// for leafs, all children remain NULL
	children.fill(NULL);
}

void QTPFS::QTNode::Delete() {
	if (!IsLeaf()) {
		for (unsigned int i = 0; i < children.size(); i++) {
			children[i]->Delete(); children[i] = NULL;
		}
	}

	delete this;
}



std::uint64_t QTPFS::QTNode::GetMemFootPrint() const {
	std::uint64_t memFootPrint = sizeof(QTNode);

	if (IsLeaf()) {
		memFootPrint += (neighbors.size() * sizeof(INode*));
		// already counted; std::array<> statically allocates
		// memFootPrint += (children.size() * sizeof(QTNode*));
		memFootPrint += (netpoints.size() * sizeof(float3));
	} else {
		for (unsigned int i = 0; i < children.size(); i++) {
			memFootPrint += (children[i]->GetMemFootPrint());
		}
	}

	return memFootPrint;
}

std::uint64_t QTPFS::QTNode::GetCheckSum() const {
	std::uint64_t sum = 0;

	{
		const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&nodeNumber);
		const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&hCost) + sizeof(hCost);

		assert(minByte < maxByte);

		// INode bytes (unpadded)
		for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
			sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
		}
	}
	{
		const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&_depth);
		const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&prevMagicNum) + sizeof(prevMagicNum);

		assert(minByte < maxByte);

		// QTNode bytes (unpadded)
		for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
			sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
		}
	}

	if (!IsLeaf()) {
		for (unsigned int n = 0; n < children.size(); n++) {
			sum ^= (((nodeNumber << 8) + 1) * children[n]->GetCheckSum());
		}
	}

	return sum;
}



bool QTPFS::QTNode::IsLeaf() const {
	assert(children.size() == QTNODE_CHILD_COUNT);
	assert(
		(children[0] == NULL && children[1] == NULL && children[2] == NULL && children[3] == NULL) ||
		(children[0] != NULL && children[1] != NULL && children[2] != NULL && children[3] != NULL)
	);
	return (children[0] == NULL);
}

bool QTPFS::QTNode::CanSplit(bool forced) const {
	// NOTE: caller must additionally check IsLeaf() before calling Split()
	if (forced) {
		return ((xsize() >> 1) > 0 && (zsize() >> 1) > 0);
	}

	if (depth() >= MAX_DEPTH) { return false; }

	#ifdef QTPFS_CONSERVATIVE_NODE_SPLITS
	if (xsize() <= MIN_SIZE_X) { return false; }
	if (zsize() <= MIN_SIZE_Z) { return false; }
	#else
	// aggressive splitting, important with respect to yardmaps
	// (one yardmap square represents four heightmap squares; a
	// node represents MIN_SIZE_X by MIN_SIZE_Z of such squares)
	if (((xsize() >> 1) ==          0) || ((zsize() >> 1) ==          0)) { return false; }
	if (( xsize()       <= MIN_SIZE_X) && ( zsize()       <= MIN_SIZE_Z)) { return false; }
	#endif

	return true;
}



bool QTPFS::QTNode::Split(NodeLayer& nl, bool forced) {
	if (!CanSplit(forced))
		return false;

	neighbors.clear();
	netpoints.clear();

	// can only split leaf-nodes (ie. nodes with NULL-children)
	assert(children[NODE_IDX_TL] == NULL);
	assert(children[NODE_IDX_TR] == NULL);
	assert(children[NODE_IDX_BR] == NULL);
	assert(children[NODE_IDX_BL] == NULL);

	children[NODE_IDX_TL] = new QTNode(this, GetChildID(NODE_IDX_TL),  xmin(), zmin(),  xmid(), zmid());
	children[NODE_IDX_TR] = new QTNode(this, GetChildID(NODE_IDX_TR),  xmid(), zmin(),  xmax(), zmid());
	children[NODE_IDX_BR] = new QTNode(this, GetChildID(NODE_IDX_BR),  xmid(), zmid(),  xmax(), zmax());
	children[NODE_IDX_BL] = new QTNode(this, GetChildID(NODE_IDX_BL),  xmin(), zmid(),  xmid(), zmax());

	nl.SetNumLeafNodes(nl.GetNumLeafNodes() + (4 - 1));
	assert(!IsLeaf());
	return true;
}

bool QTPFS::QTNode::Merge(NodeLayer& nl) {
	if (IsLeaf()) {
		return false;
	}

	neighbors.clear();

	// get rid of our children completely, but not of <this>!
	for (unsigned int i = 0; i < children.size(); i++) {
		children[i]->Delete(); children[i] = NULL;
	}

	nl.SetNumLeafNodes(nl.GetNumLeafNodes() - (4 - 1));
	assert(IsLeaf());
	return true;
}






#ifdef QTPFS_SLOW_ACCURATE_TESSELATION
	// re-tesselate a tree from the deepest node <n> that contains
	// rectangle <r> (<n> will be found from any higher node passed
	// in)
	//
	// this code can be VERY slow in the worst-case (eg. when <r>
	// overlaps all four children of the ROOT node), but minimizes
	// the overall number of nodes in the tree at any given time
	void QTPFS::QTNode::PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur) {
		bool cont = false;

		if (!IsLeaf()) {
			for (unsigned int i = 0; i < children.size(); i++) {
				if ((cont |= (children[i]->GetRectangleRelation(r) == REL_RECT_INTERIOR_NODE))) {
					// only need to descend down one branch
					children[i]->PreTesselate(nl, r, ur);
					break;
				}
			}
		}

		if (!cont) {
			ur.x1 = std::min(ur.x1, int(xmin()));
			ur.z1 = std::min(ur.z1, int(zmin()));
			ur.x2 = std::max(ur.x2, int(xmax()));
			ur.z2 = std::max(ur.z2, int(zmax()));

			Merge(nl);
			Tesselate(nl, r);
		}
	}

#else

	void QTPFS::QTNode::PreTesselate(NodeLayer& nl, const SRectangle& r, SRectangle& ur) {
		const unsigned int rel = GetRectangleRelation(r);

		// use <r> if it is fully inside <this>, otherwise clip against our edges
		const SRectangle& cr = (rel != REL_RECT_INTERIOR_NODE)? ClipRectangle(r): r;

		if ((cr.x2 - cr.x1) <= 0 || (cr.z2 - cr.z1) <= 0) {
			return;
		}

		// continue recursion while our CHILDREN are still larger than the clipped rectangle
		//
		// NOTE: this is a trade-off between minimizing the number of leaf-nodes (achieved by
		// re-tesselating in its entirety the deepest node that fully contains <r>) and cost
		// of re-tesselating (which grows as the node-count decreases, kept under control by
		// breaking <r> up into pieces while descending further)
		//
		const bool leaf = IsLeaf();
		const bool cont = (rel == REL_RECT_INTERIOR_NODE) ||
			(((xsize() >> 1) > (cr.x2 - cr.x1)) &&
			 ((zsize() >> 1) > (cr.z2 - cr.z1)));

		if (leaf || !cont) {
			// extend a bounding box around every
			// node modified during re-tesselation
			ur.x1 = std::min(ur.x1, int(xmin()));
			ur.z1 = std::min(ur.z1, int(zmin()));
			ur.x2 = std::max(ur.x2, int(xmax()));
			ur.z2 = std::max(ur.z2, int(zmax()));

			Merge(nl);
			Tesselate(nl, cr);
			return;
		}

		for (unsigned int i = 0; i < children.size(); i++) {
			children[i]->PreTesselate(nl, cr, ur);
		}
	}

#endif



void QTPFS::QTNode::Tesselate(NodeLayer& nl, const SRectangle& r) {
	unsigned int numNewBinSquares = 0; // nr. of squares in <r> that changed bin after deformation
	unsigned int numDifBinSquares = 0; // nr. of different bin-types across all squares within <r>
	unsigned int numClosedSquares = 0;

	// if true, we are at the bottom of the recursion
	bool registerNode = true;
	bool wantSplit = false;
	bool needSplit = false;

	// if we just entered Tesselate from PreTesselate, <this> was
	// merged and we need to update squares across the entire node
	//
	// if we entered from a higher-level Tesselate, <this> is newly
	// allocated and we STILL need to update across the entire node
	//
	// this means the rectangle is actually irrelevant: only use it
	// has is that numNewBinSquares can be calculated for area under
	// rectangle rather than full node
	//
	// we want to *keep* splitting so long as not ALL squares
	// within <r> share the SAME bin, OR we keep finding one
	// that SWITCHED bins after the terrain change (we already
	// know this is true for the entire rectangle or we would
	// not have reached PreTesselate)
	//
	// NOTE: during tree construction, numNewBinSquares is ALWAYS
	// non-0 for the entire map-rectangle (see NodeLayer::Update)
	//
	// NOTE: if <r> fully overlaps <this>, then splitting is *not*
	// technically required whenever numRefBinSquares is zero, ie.
	// when ALL squares in <r> changed bins in unison
	//
	UpdateMoveCost(nl, r, numNewBinSquares, numDifBinSquares, numClosedSquares, wantSplit, needSplit);

	if ((wantSplit && Split(nl, false)) || (needSplit && Split(nl, true))) {
		registerNode = false;

		for (unsigned int i = 0; i < children.size(); i++) {
			QTNode* cn = children[i];
			SRectangle cr = cn->ClipRectangle(r);

			cn->Tesselate(nl, cr);
			assert(cn->GetMoveCost() != -1.0f);
		}
	}

	if (registerNode) {
		nl.RegisterNode(this);
	}
}

bool QTPFS::QTNode::UpdateMoveCost(
	const NodeLayer& nl,
	const SRectangle& r,
	unsigned int& numNewBinSquares,
	unsigned int& numDifBinSquares,
	unsigned int& numClosedSquares,
	bool& wantSplit,
	bool& needSplit
) {
	const std::vector<NodeLayer::SpeedBinType>& oldSpeedBins = nl.GetOldSpeedBins();
	const std::vector<NodeLayer::SpeedBinType>& curSpeedBins = nl.GetCurSpeedBins();
	const std::vector<NodeLayer::SpeedModType>& oldSpeedMods = nl.GetOldSpeedMods();
	const std::vector<NodeLayer::SpeedModType>& curSpeedMods = nl.GetCurSpeedMods();

	const NodeLayer::SpeedBinType refSpeedBin = curSpeedBins[zmin() * mapDims.mapx + xmin()];

	// <this> can either just have been merged or added as
	// new child of split parent; in the former case we can
	// restrict ourselves to <r> and update the sum in part
	// (as well as checking homogeneousness just for squares
	// in <r> with a single reference point outside it)
	assert(moveCostAvg == -1.0f || moveCostAvg > 0.0f);

	if (false && moveCostAvg > 0.0f) {
		// just merged, so <r> is fully inside <this>
		//
		// the reference-square (xmin, zmin) MUST lie
		// outside <r> when <r> does not cover <this>
		// 100%, otherwise we would find a value of 0
		// for numDifBinSquares in some situations
		assert((r.x2 - r.x1) >= 0);
		assert((r.z2 - r.z1) >= 0);

		const unsigned int minx = std::max(r.x1, int(xmin()));
		const unsigned int maxx = std::min(r.x2, int(xmax()));
		const unsigned int minz = std::max(r.z1, int(zmin()));
		const unsigned int maxz = std::min(r.z2, int(zmax()));

		for (unsigned int hmz = minz; hmz < maxz; hmz++) {
			for (unsigned int hmx = minx; hmx < maxx; hmx++) {
				const unsigned int sqrIdx = hmz * mapDims.mapx + hmx;

				const NodeLayer::SpeedBinType oldSpeedBin = oldSpeedBins[sqrIdx];
				const NodeLayer::SpeedBinType curSpeedBin = curSpeedBins[sqrIdx];

				numNewBinSquares += int(curSpeedBin != oldSpeedBin);
				numDifBinSquares += int(curSpeedBin != refSpeedBin);
				numClosedSquares += int(curSpeedMods[sqrIdx] <= 0);

				speedModSum -= (oldSpeedMods[sqrIdx] / float(NodeLayer::MaxSpeedModTypeValue()));
				speedModSum += (curSpeedMods[sqrIdx] / float(NodeLayer::MaxSpeedModTypeValue()));

				assert(speedModSum >= 0.0f);
			}
		}
	} else {
		speedModSum = 0.0f;

		for (unsigned int hmz = zmin(); hmz < zmax(); hmz++) {
			for (unsigned int hmx = xmin(); hmx < xmax(); hmx++) {
				const unsigned int sqrIdx = hmz * mapDims.mapx + hmx;

				const NodeLayer::SpeedBinType oldSpeedBin = oldSpeedBins[sqrIdx];
				const NodeLayer::SpeedBinType curSpeedBin = curSpeedBins[sqrIdx];

				numNewBinSquares += int(curSpeedBin != oldSpeedBin);
				numDifBinSquares += int(curSpeedBin != refSpeedBin);
				numClosedSquares += int(curSpeedMods[sqrIdx] <= 0);

				speedModSum += (curSpeedMods[sqrIdx] / float(NodeLayer::MaxSpeedModTypeValue()));
			}
		}
	}

	// (re-)calculate the average cost of this node
	assert(speedModSum >= 0.0f);

	speedModAvg = speedModSum / area();
	moveCostAvg = (speedModAvg <= 0.001f)? QTPFS_POSITIVE_INFINITY: (1.0f / speedModAvg);

	// no node can have ZERO traversal cost
	assert(moveCostAvg > 0.0f);

	wantSplit |= (numDifBinSquares > 0);
	needSplit |= (numClosedSquares > 0 && numClosedSquares < area());

	// if we are not going to tesselate this node further
	// and there is at least one impassable square inside
	// it, make sure the pathfinder will not pick us
	//
	// HACK:
	//   set the cost for *!PARTIALLY!* closed nodes to a
	//   non-infinite value since these are often created
	//   along factory exit lanes (most on non-square maps
	//   or when MIN_SIZE_X > 1 or MIN_SIZE_Z > 1), but do
	//   ensure their cost is still high enough so they get
	//   expanded only when absolutely necessary
	//
	//   units with footprint dimensions equal to the size
	//   of a lane would otherwise be unable to find a path
	//   out of their factories
	//
	//   this is crucial for handling the squares underneath
	//   static obstacles (eg. factories) if MIN_SIZE_* != 1
	if (numClosedSquares > 0) {
		if (numClosedSquares < area()) {
			moveCostAvg = QTPFS_CLOSED_NODE_COST * (numClosedSquares / float(xsize() * xsize()));
		} else {
			moveCostAvg = QTPFS_POSITIVE_INFINITY;
		}
	}

	return (wantSplit || needSplit);
}






// get the maximum number of neighbors this node
// can have, based on its position / size and the
// assumption that all neighbors are 1x1
//
// NOTE: this intentionally does not count corners
unsigned int QTPFS::QTNode::GetMaxNumNeighbors() const {
	unsigned int n = 0;

	if (xmin() > (               0)) { n += zsize(); } // count EDGE_L ngbs
	if (xmax() < (mapDims.mapx - 1)) { n += zsize(); } // count EDGE_R ngbs
	if (zmin() > (               0)) { n += xsize(); } // count EDGE_T ngbs
	if (zmax() < (mapDims.mapy - 1)) { n += xsize(); } // count EDGE_B ngbs

	return n;
}





void QTPFS::QTNode::Serialize(std::fstream& fStream, NodeLayer& nodeLayer, unsigned int* streamSize, bool readMode) {
	// overwritten when de-serializing
	unsigned int numChildren = QTNODE_CHILD_COUNT * (1 - int(IsLeaf()));

	(*streamSize) += (2 * sizeof(unsigned int));
	(*streamSize) += (3 * sizeof(float));

	if (readMode) {
		fStream.read(reinterpret_cast<char*>(&nodeNumber), sizeof(unsigned int));
		fStream.read(reinterpret_cast<char*>(&numChildren), sizeof(unsigned int));

		fStream.read(reinterpret_cast<char*>(&speedModAvg), sizeof(float));
		fStream.read(reinterpret_cast<char*>(&speedModSum), sizeof(float));
		fStream.read(reinterpret_cast<char*>(&moveCostAvg), sizeof(float));

		if (numChildren > 0) {
			// re-create child nodes
			assert(IsLeaf());
			Split(nodeLayer, true);
		} else {
			// node was a leaf in an earlier life, register it
			nodeLayer.RegisterNode(this);
		}
	} else {
		fStream.write(reinterpret_cast<const char*>(&nodeNumber), sizeof(unsigned int));
		fStream.write(reinterpret_cast<const char*>(&numChildren), sizeof(unsigned int));

		fStream.write(reinterpret_cast<const char*>(&speedModAvg), sizeof(float));
		fStream.write(reinterpret_cast<const char*>(&speedModSum), sizeof(float));
		fStream.write(reinterpret_cast<const char*>(&moveCostAvg), sizeof(float));
	}

	for (unsigned int i = 0; i < numChildren; i++) {
		children[i]->Serialize(fStream, nodeLayer, streamSize, readMode);
	}
}

unsigned int QTPFS::QTNode::GetNeighbors(const std::vector<INode*>& nodes, std::vector<INode*>& ngbs) {
	#ifdef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
	UpdateNeighborCache(nodes);
	#endif

	if (!neighbors.empty()) {
		ngbs.clear();
		ngbs.resize(neighbors.size());

		std::copy(neighbors.begin(), neighbors.end(), ngbs.begin());
	}

	return (neighbors.size());
}

const std::vector<QTPFS::INode*>& QTPFS::QTNode::GetNeighbors(const std::vector<INode*>& nodes) {
	#ifdef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
	UpdateNeighborCache(nodes);
	#endif
	return neighbors;
}

// this is *either* called from ::GetNeighbors when the conservative
// update-scheme is enabled, *or* from PM::ExecQueuedNodeLayerUpdates
// (never both)
bool QTPFS::QTNode::UpdateNeighborCache(const std::vector<INode*>& nodes) {
	assert(IsLeaf());
	assert(!nodes.empty());

	if (prevMagicNum != currMagicNum) {
		prevMagicNum = currMagicNum;

		unsigned int ngbRels = 0;
		unsigned int maxNgbs = GetMaxNumNeighbors();

		// regenerate our neighbor cache
		if (maxNgbs > 0) {
			neighbors.clear();
			neighbors.reserve(maxNgbs + 4);

			netpoints.clear();
			netpoints.reserve(1 + maxNgbs * QTPFS_MAX_NETPOINTS_PER_NODE_EDGE + 4);
			// NOTE: caching ETP's breaks QTPFS_ORTHOPROJECTED_EDGE_TRANSITIONS
			// NOTE: [0] is a reserved index and must always be valid
			netpoints.push_back(float3());

			INode* ngb = NULL;

			if (xmin() > 0) {
				const unsigned int hmx = xmin() - 1;

				// walk along EDGE_L (west) neighbors
				for (unsigned int hmz = zmin(); hmz < zmax(); ) {
					ngb = nodes[hmz * mapDims.mapx + hmx];
					hmz = ngb->zmax();

					neighbors.push_back(ngb);

					for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
						netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngb, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
					}
				}

				ngbRels |= REL_NGB_EDGE_L;
			}
			if (xmax() < static_cast<unsigned int>(mapDims.mapx)) {
				const unsigned int hmx = xmax() + 0;

				// walk along EDGE_R (east) neighbors
				for (unsigned int hmz = zmin(); hmz < zmax(); ) {
					ngb = nodes[hmz * mapDims.mapx + hmx];
					hmz = ngb->zmax();

					neighbors.push_back(ngb);

					for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
						netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngb, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
					}
				}

				ngbRels |= REL_NGB_EDGE_R;
			}

			if (zmin() > 0) {
				const unsigned int hmz = zmin() - 1;

				// walk along EDGE_T (north) neighbors
				for (unsigned int hmx = xmin(); hmx < xmax(); ) {
					ngb = nodes[hmz * mapDims.mapx + hmx];
					hmx = ngb->xmax();

					neighbors.push_back(ngb);

					for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
						netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngb, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
					}
				}

				ngbRels |= REL_NGB_EDGE_T;
			}
			if (zmax() < static_cast<unsigned int>(mapDims.mapy)) {
				const unsigned int hmz = zmax() + 0;

				// walk along EDGE_B (south) neighbors
				for (unsigned int hmx = xmin(); hmx < xmax(); ) {
					ngb = nodes[hmz * mapDims.mapx + hmx];
					hmx = ngb->xmax();

					neighbors.push_back(ngb);

					for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
						netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngb, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
					}
				}

				ngbRels |= REL_NGB_EDGE_B;
			}

			#ifdef QTPFS_CORNER_CONNECTED_NODES
			// top- and bottom-left corners
			if ((ngbRels & REL_NGB_EDGE_L) != 0) {
				if ((ngbRels & REL_NGB_EDGE_T) != 0) {
					const INode* ngbL = nodes[(zmin() + 0) * mapDims.mapx + (xmin() - 1)];
					const INode* ngbT = nodes[(zmin() - 1) * mapDims.mapx + (xmin() + 0)];
						  INode* ngbC = nodes[(zmin() - 1) * mapDims.mapx + (xmin() - 1)];

					// VERT_TL ngb must be distinct from EDGE_L and EDGE_T ngbs
					if (ngbC != ngbL && ngbC != ngbT) {
						if (ngbL->AllSquaresAccessible() && ngbT->AllSquaresAccessible()) {
							neighbors.push_back(ngbC);

							for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
								netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngbC, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
							}
						}
					}
				}
				if ((ngbRels & REL_NGB_EDGE_B) != 0) {
					const INode* ngbL = nodes[(zmax() - 1) * mapDims.mapx + (xmin() - 1)];
					const INode* ngbB = nodes[(zmax() + 0) * mapDims.mapx + (xmin() + 0)];
						  INode* ngbC = nodes[(zmax() + 0) * mapDims.mapx + (xmin() - 1)];

					// VERT_BL ngb must be distinct from EDGE_L and EDGE_B ngbs
					if (ngbC != ngbL && ngbC != ngbB) {
						if (ngbL->AllSquaresAccessible() && ngbB->AllSquaresAccessible()) {
							neighbors.push_back(ngbC);

							for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
								netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngbC, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
							}
						}
					}
				}
			}

			// top- and bottom-right corners
			if ((ngbRels & REL_NGB_EDGE_R) != 0) {
				if ((ngbRels & REL_NGB_EDGE_T) != 0) {
					const INode* ngbR = nodes[(zmin() + 0) * mapDims.mapx + (xmax() + 0)];
					const INode* ngbT = nodes[(zmin() - 1) * mapDims.mapx + (xmax() - 1)];
						  INode* ngbC = nodes[(zmin() - 1) * mapDims.mapx + (xmax() + 0)];

					// VERT_TR ngb must be distinct from EDGE_R and EDGE_T ngbs
					if (ngbC != ngbR && ngbC != ngbT) {
						if (ngbR->AllSquaresAccessible() && ngbT->AllSquaresAccessible()) {
							neighbors.push_back(ngbC);

							for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
								netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngbC, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
							}
						}
					}
				}
				if ((ngbRels & REL_NGB_EDGE_B) != 0) {
					const INode* ngbR = nodes[(zmax() - 1) * mapDims.mapx + (xmax() + 0)];
					const INode* ngbB = nodes[(zmax() + 0) * mapDims.mapx + (xmax() - 1)];
						  INode* ngbC = nodes[(zmax() + 0) * mapDims.mapx + (xmax() + 0)];

					// VERT_BR ngb must be distinct from EDGE_R and EDGE_B ngbs
					if (ngbC != ngbR && ngbC != ngbB) {
						if (ngbR->AllSquaresAccessible() && ngbB->AllSquaresAccessible()) {
							neighbors.push_back(ngbC);

							for (unsigned int i = 0; i < QTPFS_MAX_NETPOINTS_PER_NODE_EDGE; i++) {
								netpoints.push_back(INode::GetNeighborEdgeTransitionPoint(ngbC, float3(), QTPFS_NETPOINT_EDGE_SPACING_SCALE * (i + 1)));
							}
						}
					}
				}
			}
			#endif

			// NOTE: gcc 4.7 should FINALLY define __cplusplus properly (and accept -std=c++11)
			#if (__cplusplus >= 201103L)
			// neighbors.shrink_to_fit();
			// netpoints.shrink_to_fit();
			#endif
		}

		return true;
	}

	return false;
}

