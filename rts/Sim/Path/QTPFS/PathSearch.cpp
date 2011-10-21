/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <list>

#include "PathSearch.hpp"
#include "Path.hpp"
#include "PathCache.hpp"
#include "NodeLayer.hpp"
#include "Sim/Misc/GlobalConstants.h"

void QTPFS::PathSearch::Initialize(const float3& sourcePoint, const float3& targetPoint) {
	srcPoint = sourcePoint; srcPoint.CheckInBounds();
	tgtPoint = targetPoint; tgtPoint.CheckInBounds();
	curPoint = srcPoint;
	nxtPoint = tgtPoint;
}

bool QTPFS::PathSearch::Execute(PathCache* cache, NodeLayer* layer, unsigned int searchStateOffset) {
	pathCache = cache;
	nodeLayer = layer;

	srcNode = nodeLayer->GetNode(srcPoint.x / SQUARE_SIZE, srcPoint.z / SQUARE_SIZE);
	tgtNode = nodeLayer->GetNode(tgtPoint.x / SQUARE_SIZE, tgtPoint.z / SQUARE_SIZE);
	curNode = NULL;
	nxtNode = NULL;

	searchState = searchStateOffset;

	haveOpenNode = true;
	haveFullPath = (srcNode == tgtNode);
//	haveFullPath = ((srcNode == tgtNode) || (curNode->GetDistance(tgtNode, NODE_DIST_EUCLIDEAN) <= radius));

	std::vector<INode*>& allNodes = nodeLayer->GetNodes();
	std::vector<INode*> ngbNodes;

	switch (searchType) {
		case PATH_SEARCH_ASTAR:    { hCostMult = 1.0f; } break;
		case PATH_SEARCH_DIJKSTRA: { hCostMult = 0.0f; } break;

		default: {
			assert(false);
		} break;
	}

	if (!haveFullPath) {
		openNodes.push(srcNode);
		srcNode->SetPrevNode(NULL);
		srcNode->SetPathCost(NODE_PATH_COST_G, 0.0f);
		srcNode->SetPathCost(NODE_PATH_COST_H, (tgtPoint - srcPoint).Length() * hCostMult);
		srcNode->SetPathCost(NODE_PATH_COST_F, (srcNode->GetPathCost(NODE_PATH_COST_G) + srcNode->GetPathCost(NODE_PATH_COST_H)));
	}

	while (haveOpenNode && !haveFullPath) {
		IterateSearch(allNodes, ngbNodes);
	}

	return haveFullPath;
}

void QTPFS::PathSearch::IterateSearch(const std::vector<INode*>& allNodes, std::vector<INode*>& ngbNodes) {
	curNode = openNodes.top();
	curNode->SetSearchState(searchState | NODE_STATE_CLOSED);

	openNodes.pop();

	if (curNode != srcNode)
		curPoint = curNode->GetNeighborEdgeMidPoint(curNode->GetPrevNode());

	haveFullPath = (curNode == tgtNode);

	if (!haveFullPath) {
		// calculate neighbors on-the-fly rather than
		// storing them in the nodes themselves (less
		// complex)
		const unsigned int numNgbs = curNode->GetNeighbors(allNodes, ngbNodes);

		// examine each neighbor of <curNode>
		for (unsigned int i = 0; i < numNgbs; i++) {
			nxtNode = ngbNodes[i];

			const bool isCurrent = (nxtNode->GetSearchState() >= searchState);
			const bool isClosed = ((nxtNode->GetSearchState() & 1) == NODE_STATE_CLOSED);

			// NOTE:
			//     this uses the actual distance that edges of the final path will cover,
			//     from <curPoint> (initialized to sourcePoint) to the middle of shared edge
			//     between <curNode> and <nxtNode>
			//     (each individual path-segment is weighted by the average move-cost of
			//     the node it crosses, UNLIKE the goal-heuristic)
			// NOTE:
			//     heading for the MIDDLE of the shared edge is not always the best option
			nxtPoint = curNode->GetNeighborEdgeMidPoint(nxtNode);

			const float mDist = (nxtPoint - curPoint).Length();
			const float gCost = curNode->GetPathCost(NODE_PATH_COST_G) + (mDist * curNode->GetMoveCost());
			bool keepNextNode = true;

			if (isCurrent) {
				if (isClosed) {
					continue;
				}

				// if true, we found a new shorter route to <nxtNode>
				keepNextNode = (gCost < nxtNode->GetPathCost(NODE_PATH_COST_G));
			} else {
				// at this point, we know that <nxtNode> is either
				// not from the current search (!current) or (if it
				// is) already placed in the open queue
				nxtNode->SetSearchState(searchState | NODE_STATE_OPEN);
				openNodes.push(nxtNode);
			}

			if (keepNextNode) {
				// NOTE:
				//     the heuristic must never over-estimate the distance,
				//     but this is *impossible* to achieve on a non-regular
				//     grid on which any node only has an average move-cost
				//     associated with it (and these costs can even be less
				//     than 1) --> paths will not be optimal, but they could
				//     not be anyway on an irregular grid
				nxtNode->SetPrevNode(curNode);
				nxtNode->SetPathCost(NODE_PATH_COST_G, gCost);
				nxtNode->SetPathCost(NODE_PATH_COST_H, (tgtPoint - nxtPoint).Length() * hCostMult);
				nxtNode->SetPathCost(NODE_PATH_COST_F, (nxtNode->GetPathCost(NODE_PATH_COST_G) + nxtNode->GetPathCost(NODE_PATH_COST_H)));
			}
		}
	}

	haveOpenNode = (!openNodes.empty());
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
		const INode* tmpNode = tgtNode;
		const INode* oldNode = tmpNode->GetPrevNode();

		while ((oldNode != NULL) && (tmpNode != srcNode)) {
			points.push_front(tmpNode->GetNeighborEdgeMidPoint(oldNode));

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

