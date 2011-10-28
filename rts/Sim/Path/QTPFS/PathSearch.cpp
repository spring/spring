/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <list>
#include <limits>

#include "PathSearch.hpp"
#include "Path.hpp"
#include "PathCache.hpp"
#include "NodeLayer.hpp"
#include "Sim/Misc/GlobalConstants.h"

#ifdef QTPFS_TRACE_PATH_SEARCHES
#include "Sim/Misc/GlobalSynced.h"
#endif

void QTPFS::PathSearch::Initialize(
	NodeLayer* layer,
	PathCache* cache,
	const float3& sourcePoint,
	const float3& targetPoint
) {
	srcPoint = sourcePoint; srcPoint.CheckInBounds();
	tgtPoint = targetPoint; tgtPoint.CheckInBounds();
	curPoint = srcPoint;
	nxtPoint = tgtPoint;

	nodeLayer = layer;
	pathCache = cache;

	srcNode = nodeLayer->GetNode(srcPoint.x / SQUARE_SIZE, srcPoint.z / SQUARE_SIZE);
	tgtNode = nodeLayer->GetNode(tgtPoint.x / SQUARE_SIZE, tgtPoint.z / SQUARE_SIZE);
	curNode = NULL;
	nxtNode = NULL;
}

bool QTPFS::PathSearch::Execute(
	PathSearchTrace::Execution* exec,
	unsigned int searchStateOffset,
	unsigned int searchMagicNumber
) {
	searchState = searchStateOffset;
	searchMagic = searchMagicNumber;

	haveOpenNode = true;
	haveFullPath = (srcNode == tgtNode);
//	haveFullPath = ((srcNode == tgtNode) || (curNode->GetDistance(tgtNode, NODE_DIST_EUCLIDEAN) <= radius));

	std::vector<INode*>& allNodes = nodeLayer->GetNodes();
	std::vector<INode*> ngbNodes;

	PathSearchTrace::Iteration iter;

	switch (searchType) {
		case PATH_SEARCH_ASTAR:    { hCostMult = 1.0f; } break;
		case PATH_SEARCH_DIJKSTRA: { hCostMult = 0.0f; } break;

		default: {
			assert(false);
		} break;
	}

	if (!haveFullPath) {
		openNodes.reserve(nodeLayer->GetNumLeafNodes());
		openNodes.push(srcNode);
		srcNode->SetPrevNode(NULL);
		srcNode->SetPathCost(NODE_PATH_COST_G, 0.0f);
		srcNode->SetPathCost(NODE_PATH_COST_H, (tgtPoint - srcPoint).Length() * hCostMult);
		srcNode->SetPathCost(NODE_PATH_COST_F, (srcNode->GetPathCost(NODE_PATH_COST_G) + srcNode->GetPathCost(NODE_PATH_COST_H)));
	}

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	assert(exec != NULL);
	#endif

	while ((haveOpenNode = !openNodes.empty()) && !(haveFullPath = (curNode == tgtNode))) {
		IterateSearch(allNodes, ngbNodes, &iter);

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		exec->AddIteration(iter);
		iter.Clear();
		#endif
	}

	return haveFullPath;
}



void QTPFS::PathSearch::UpdateNode(INode* nxt, INode* cur, float gCost) {
	// NOTE:
	//     the heuristic must never over-estimate the distance,
	//     but this is *impossible* to achieve on a non-regular
	//     grid on which any node only has an average move-cost
	//     associated with it (and these costs can even be less
	//     than 1) --> paths will not be optimal, but they could
	//     not be anyway
	nxt->SetSearchState(searchState | NODE_STATE_OPEN);
	nxt->SetPrevNode(cur);
	nxt->SetPathCost(NODE_PATH_COST_G, gCost);
	nxt->SetPathCost(NODE_PATH_COST_H, (tgtPoint - nxtPoint).Length() * hCostMult);
	nxt->SetPathCost(NODE_PATH_COST_F, (nxt->GetPathCost(NODE_PATH_COST_G) + nxt->GetPathCost(NODE_PATH_COST_H)));
}

void QTPFS::PathSearch::UpdateQueue() {
	// restore ordering in case nxtNode was already open
	// (changing the f-cost of an OPEN node messes up the
	// queue's internal consistency; a pushed node remains
	// OPEN until it gets popped)
	INode* top = openNodes.top();
	openNodes.pop();
	openNodes.push(top);
}

void QTPFS::PathSearch::IterateSearch(
	const std::vector<INode*>& allNodes,
	      std::vector<INode*>& ngbNodes,
	PathSearchTrace::Iteration* iter
) {
	curNode = openNodes.top();
	curNode->SetSearchState(searchState | NODE_STATE_CLOSED);
	curNode->SetMagicNumber(searchMagic);

	openNodes.pop();

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	iter->SetPoppedNodeIdx(curNode->zmin() * gs->mapx + curNode->xmin());
	#endif

	if (curNode != srcNode)
		curPoint = curNode->GetNeighborEdgeMidPoint(curNode->GetPrevNode());
	if (curNode == tgtNode)
		return;
	if (curNode->GetMoveCost() == QTPFS_POSITIVE_INFINITY)
		return;


	#ifdef QTPFS_COPY_NEIGHBOR_NODES
	const unsigned int numNgbs = curNode->GetNeighbors(allNodes, ngbNodes);
	#else
	// cannot assign to <ngbNodes> because that would still make a copy
	const std::vector<INode*>& nxtNodes = curNode->GetNeighbors(allNodes);
	const unsigned int numNgbs = nxtNodes.size();
	#endif

	for (unsigned int i = 0; i < numNgbs; i++) {
		// NOTE:
		//     this uses the actual distance that edges of the final path will cover,
		//     from <curPoint> (initialized to sourcePoint) to the middle of the edge
		//     shared between <curNode> and <nxtNode>
		//     (each individual path-segment is weighted by the average move-cost of
		//     the node it crosses, UNLIKE the goal-heuristic)
		// NOTE:
		//     heading for the MIDDLE of the shared edge is not always the best option
		//     we deal with this sub-optimality later (in SmoothPath if it is enabled)
		// NOTE:
		//     short paths that should have 3 points (2 nodes) can contain 4 (3 nodes);
		//     this happens when a path takes a "detour" through a corner neighbor of
		//     srcNode if the shared corner vertex is closer to the goal position than
		//     the mid-point on the edge between srcNode and tgtNode
		#ifdef QTPFS_COPY_NEIGHBOR_NODES
		nxtNode = ngbNodes[i];
		nxtPoint = curNode->GetNeighborEdgeMidPoint(nxtNode);
		#else
		nxtNode = nxtNodes[i];
		nxtPoint = curNode->GetNeighborEdgeMidPoint(nxtNode);
		#endif

		assert(curNode->GetNeighborEdgeMidPoint(nxtNode) == nxtNode->GetNeighborEdgeMidPoint(curNode));

		const bool isCurrent = (nxtNode->GetSearchState() >= searchState);
		const bool isClosed = ((nxtNode->GetSearchState() & 1) == NODE_STATE_CLOSED);

		const float nCost = curNode->GetMoveCost() * (nxtPoint - curPoint).Length();
		const float gCost = curNode->GetPathCost(NODE_PATH_COST_G) + nCost;

		if (!isCurrent) {
			// at this point, we know that <nxtNode> is either
			// not from the current search (!current) or (if it
			// is) already placed in the open queue
			UpdateNode(nxtNode, curNode, gCost);

			#ifdef QTPFS_TRACE_PATH_SEARCHES
			iter->AddPushedNodeIdx(nxtNode->zmin() * gs->mapx + nxtNode->xmin());
			#endif

			openNodes.push(nxtNode);
			continue;
		}

		if (isClosed)
			continue;
		if (gCost >= nxtNode->GetPathCost(NODE_PATH_COST_G))
			continue;

		UpdateNode(nxtNode, curNode, gCost);
		UpdateQueue();
	}
}

void QTPFS::PathSearch::Finalize(IPath* path, bool replace) {
	TracePath(path);

	path->SetSourcePoint(srcPoint);
	path->SetTargetPoint(tgtPoint);

	#ifdef QTPFS_SMOOTH_PATHS
	SmoothPath(path);
	#endif

	// path remains in live-cache until DeletePath is called
	pathCache->AddLivePath(path, replace);
}

void QTPFS::PathSearch::TracePath(IPath* path) {
	std::list<float3> points;
	std::list<float3>::const_iterator pointsIt;

	if (srcNode != tgtNode) {
		INode* tmpNode = tgtNode;
		INode* oldNode = tmpNode->GetPrevNode();

		while ((oldNode != NULL) && (tmpNode != srcNode)) {
			points.push_front(tmpNode->GetNeighborEdgeMidPoint(oldNode));

			#ifndef QTPFS_SMOOTH_PATHS
			// make sure these can never become dangling
			// (if we smooth, we do this in SmoothPath())
			tmpNode->SetPrevNode(NULL);
			#endif

			tmpNode = oldNode;
			oldNode = tmpNode->GetPrevNode();
		}
	}

	// if source equals target, we need only two points
	if (!points.empty()) {
		path->AllocPoints(points.size() + 2);
	} else {
		assert(path->NumPoints() == 2);
		return;
	}

	// set waypoints with indices [1, N - 2]; the first (0)
	// and last (N - 1) waypoint are not set until after we
	// return
	while (!points.empty()) {
		path->SetPoint((path->NumPoints() - points.size()) - 1, points.front());
		points.pop_front();
	}
}

void QTPFS::PathSearch::SmoothPath(IPath* path) {
	if (path->NumPoints() == 2)
		return;

	INode* n0 = tgtNode;
	INode* n1 = tgtNode;

	assert(srcNode->GetPrevNode() == NULL);

	// smooth in reverse order (target to source)
	unsigned int ni = path->NumPoints();

	while (n1 != srcNode) {
		n0 = n1;
		n1 = n0->GetPrevNode();
		n0->SetPrevNode(NULL);
		ni -= 1;

		assert(n1->GetNeighborRelation(n0) != 0);
		assert(n0->GetNeighborRelation(n1) != 0);
		assert(ni < path->NumPoints());

		const unsigned int ngbRel = n0->GetNeighborRelation(n1);
		const float3& p0 = path->GetPoint(ni    );
		      float3  p1 = path->GetPoint(ni - 1);
		const float3& p2 = path->GetPoint(ni - 2);

		// check if we can reduce the angle between segments
		// p0-p1 and p1-p2 (ideally to zero degrees, making
		// p0-p2 a straight line) without causing either of
		// the segments to cross into other nodes
		//
		// p1 always lies on the node to the right and/or to
		// the bottom of the shared edge between p0 and p2,
		// and we move it along the edge-dimension (x or z)
		// between [xmin, xmax] or [zmin, zmax]
		const float3 p1p0 = (p1 - p0).SafeNormalize();
		const float3 p2p1 = (p2 - p1).SafeNormalize();
		const float3 p2p0 = (p2 - p0).SafeNormalize();
		const float   dot = p1p0.dot(p2p1);

		// if segments are already nearly parallel, skip
		if (dot >= 0.995f)
			continue;

		// figure out if p1 is on a horizontal or a vertical edge
		// (if both of these are true, it is in fact in a corner)
		const bool hEdge = (((ngbRel & REL_NGB_EDGE_T) != 0) || ((ngbRel & REL_NGB_EDGE_B) != 0));
		const bool vEdge = (((ngbRel & REL_NGB_EDGE_L) != 0) || ((ngbRel & REL_NGB_EDGE_R) != 0));

		assert(hEdge || vEdge);

		// establish the x- and z-range within which p1 can be moved
		const unsigned int xmin = std::max(n1->xmin(), n0->xmin());
		const unsigned int zmin = std::max(n1->zmin(), n0->zmin());
		const unsigned int xmax = std::min(n1->xmax(), n0->xmax());
		const unsigned int zmax = std::min(n1->zmax(), n0->zmax());

		{
			// calculate intersection point between ray (p2 - p0) and edge
			// if pi lies between bounds, use that and move to next triplet
			//
			// cases:
			//     A) p0-p1-p2 (p2p0.xz >= 0 -- p0 in n0, p2 in n1)
			//     B) p2-p1-p0 (p2p0.xz <= 0 -- p2 in n1, p0 in n0)
			float3 pi = ZeroVector;

			// x- and z-distances to edge between n0 and n1
			const float dfx = (p2p0.x > 0.0f)?
				((n0->xmax() * SQUARE_SIZE) - p0.x): // A(x)
				((n0->xmin() * SQUARE_SIZE) - p0.x); // B(x)
			const float dfz = (p2p0.z > 0.0f)?
				((n0->zmax() * SQUARE_SIZE) - p0.z): // A(z)
				((n0->zmin() * SQUARE_SIZE) - p0.z); // B(z)

			const float dx = (math::fabs(p2p0.x) > 0.001f)? p2p0.x: 0.001f;
			const float dz = (math::fabs(p2p0.z) > 0.001f)? p2p0.z: 0.001f;
			const float tx = dfx / dx;
			const float tz = dfz / dz;

			bool ok = true;

			if (hEdge) {
				pi.x = p0.x + p2p0.x * tz;
				pi.z = p1.z;
			}
			if (vEdge) {
				pi.x = p1.x;
				pi.z = p0.z + p2p0.z * tx;
			}

			ok = ok && (pi.x >= (xmin * SQUARE_SIZE) && pi.x <= (xmax * SQUARE_SIZE));
			ok = ok && (pi.z >= (zmin * SQUARE_SIZE) && pi.z <= (zmax * SQUARE_SIZE));

			if (ok) {
				path->SetPoint(ni - 1, pi);
				continue;
			}
		}

		if (hEdge != vEdge) {
			// get the edge end-points
			float3 e0 = p1;
			float3 e1 = p1;

			if (hEdge) {
				e0.x = xmin * SQUARE_SIZE;
				e1.x = xmax * SQUARE_SIZE;
			}
			if (vEdge) {
				e0.z = zmin * SQUARE_SIZE;
				e1.z = zmax * SQUARE_SIZE;
			}

			// figure out what the angle between p0-p1 and p1-p2
			// would be after substituting the edge-ends for p1
			// (we want dot-products as close to 1 as possible)
			//
			// p0-e0-p2
			const float3 e0p0 = (e0 - p0).SafeNormalize();
			const float3 p2e0 = (p2 - e0).SafeNormalize();
			const float  dot0 = e0p0.dot(p2e0);
			// p0-e1-p2
			const float3 e1p0 = (e1 - p0).SafeNormalize();
			const float3 p2e1 = (p2 - e1).SafeNormalize();
			const float  dot1 = e1p0.dot(p2e1);

			// if neither end-point is an improvement, skip
			if (dot > std::max(dot0, dot1))
				continue;

			if (dot0 > std::max(dot1, dot)) { p1 = e0; }
			if (dot1 > std::max(dot0, dot)) { p1 = e1; }

			path->SetPoint(ni - 1, p1);
		}
	}
}



void QTPFS::PathSearch::SharedFinalize(const IPath* srcPath, IPath* dstPath) {
	assert(dstPath->GetID() != 0);
	assert(dstPath->GetID() != srcPath->GetID());
	assert(dstPath->NumPoints() == 2);

	// copy <srcPath> to <dstPath>
	dstPath->CopyPoints(*srcPath);
	dstPath->SetSourcePoint(srcPoint);
	dstPath->SetTargetPoint(tgtPoint);
	pathCache->AddLivePath(dstPath, false);
}

const boost::uint64_t QTPFS::PathSearch::GetHash(unsigned int N, unsigned int k) const {
	return (srcNode->GetNodeNumber() + (tgtNode->GetNodeNumber() * N) + (k * N * N));
}

