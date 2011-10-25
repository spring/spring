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

void QTPFS::PathSearch::Initialize(const float3& sourcePoint, const float3& targetPoint) {
	srcPoint = sourcePoint; srcPoint.CheckInBounds();
	tgtPoint = targetPoint; tgtPoint.CheckInBounds();
	curPoint = srcPoint;
	nxtPoint = tgtPoint;
}

bool QTPFS::PathSearch::Execute(
	PathCache* cache,
	NodeLayer* layer,
	PathSearchTrace::Execution* exec,
	unsigned int searchStateOffset,
	unsigned int searchMagicNumber
) {
	pathCache = cache;
	nodeLayer = layer;

	srcNode = nodeLayer->GetNode(srcPoint.x / SQUARE_SIZE, srcPoint.z / SQUARE_SIZE);
	tgtNode = nodeLayer->GetNode(tgtPoint.x / SQUARE_SIZE, tgtPoint.z / SQUARE_SIZE);
	curNode = NULL;
	nxtNode = NULL;

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
		openNodes.reserve(16384);
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
	if (curNode->GetMoveCost() == std::numeric_limits<float>::infinity())
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
		//     from <curPoint> (initialized to sourcePoint) to the middle of shared edge
		//     between <curNode> and <nxtNode>
		//     (each individual path-segment is weighted by the average move-cost of
		//     the node it crosses, UNLIKE the goal-heuristic)
		// NOTE:
		//     heading for the MIDDLE of the shared edge is not always the best option
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
	FillPath(path);

	path->SetPoint(                    0, srcPoint);
	path->SetPoint(path->NumPoints() - 1, tgtPoint);

	// path remains in live-cache until DeletePath is called
	pathCache->AddLivePath(path, replace);
}

void QTPFS::PathSearch::FillPath(IPath* path) {
	std::list<float3> points;
	std::list<float3>::const_iterator pointsIt;

	if (srcNode != tgtNode) {
		INode* tmpNode = tgtNode;
		INode* oldNode = tmpNode->GetPrevNode();

		while ((oldNode != NULL) && (tmpNode != srcNode)) {
			points.push_front(tmpNode->GetNeighborEdgeMidPoint(oldNode));
			// make sure these can never become dangling
			tmpNode->SetPrevNode(NULL);

			tmpNode = oldNode;
			oldNode = tmpNode->GetPrevNode();
		}
	}

	// if source equals target, we need only two points
	path->AllocPoints(points.size() + 2);

	// set waypoints with indices [1, N - 2]; the first (0)
	// and last (N - 1) waypoint are not set until after we
	// return
	while (!points.empty()) {
		path->SetPoint((path->NumPoints() - points.size()) - 1, points.front());
		points.pop_front();
	}
}

