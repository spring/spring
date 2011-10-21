/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHSEARCH_HDR
#define QTPFS_PATHSEARCH_HDR

#include <queue>
#include <vector>

#include "Node.hpp"
#include "System/float3.h"

namespace QTPFS {
	struct PathCache;
	struct NodeLayer;
	struct IPath;

	// NOTE:
	//     we could support "time-sliced" execution, but we would have
	//     to isolate each query from modifying another's INode members
	//     (*Cost, nodeState, etc.) --> memory-intensive
	//     also, terrain changes could invalidate partial paths without
	//     buffering the *entire* heightmap each frame --> not efficient
	// NOTE:
	//     with time-sliced execution, {src,tgt,cur,nxt}Node can become
	//     dangling
	struct IPathSearch {
		IPathSearch(unsigned int pathSearchType): searchType(pathSearchType) {}
		virtual ~IPathSearch() {}

		virtual void Initialize(const float3& sourcePoint, const float3& targetPoint) = 0;
		virtual bool Execute(PathCache* cache, NodeLayer* layer, unsigned int searchStateOffset) = 0;
		virtual void Finalize(IPath* path, bool replace) = 0;

		void SetID(unsigned int n) { searchID = n; }
		unsigned int GetID() const { return searchID; }

	protected:
		unsigned int searchType;
		unsigned int searchState;
		unsigned int searchID;
	};

	struct PathSearch: public IPathSearch {
	public:
		PathSearch(unsigned int pathSearchType): IPathSearch(pathSearchType) {}
		~PathSearch() { while (!openNodes.empty()) { openNodes.pop(); } }

		void Initialize(const float3& sourcePoint, const float3& targetPoint);
		bool Execute(PathCache* cache, NodeLayer* layer, unsigned int searchStateOffset);
		void Finalize(IPath* path, bool replace);

	private:
		void IterateSearch(const std::vector<INode*>& allNodes, std::vector<INode*>& ngbNodes);
		void FillPath(IPath* path);

		PathCache* pathCache;
		NodeLayer* nodeLayer;

		float3 srcPoint, tgtPoint;
		float3 curPoint, nxtPoint;

		INode *srcNode, *tgtNode;
		INode *curNode, *nxtNode;

		// rely on INode::operator() to sort the INode*'s by increasing f-cost
		std::priority_queue<INode*, std::vector<INode*>, INode> openNodes;

		bool haveOpenNode;
		bool haveFullPath;

		float hCostMult;
	};
};

#endif

