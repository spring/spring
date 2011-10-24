/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHSEARCH_HDR
#define QTPFS_PATHSEARCH_HDR

#include <vector>

#include "Node.hpp"
#include "PathDefines.hpp"
#include "PathDataTypes.hpp"
#include "System/float3.h"

namespace QTPFS {
	struct PathCache;
	struct NodeLayer;
	struct IPath;

	namespace PathSearchTrace {
		struct Iteration {
			Iteration() { nodeIndices.push_back(-1U); }
			~Iteration() { nodeIndices.clear(); }

			void Clear() {
				nodeIndices.clear();
				nodeIndices.push_back(-1U);
			}
			void SetPoppedNodeIdx(unsigned int i) { (nodeIndices.front()) = i; }
			void AddPushedNodeIdx(unsigned int i) { (nodeIndices.push_back(i)); }

			const std::list<unsigned int>& GetNodeIndices() const { return nodeIndices; }

		private:
			// NOTE: indices are only valid so long as tree is not re-tesselated
			std::list<unsigned int> nodeIndices;
		};

		struct Execution {
			Execution(unsigned int f): frame(f) {}
			~Execution() { iterations.clear(); }

			void AddIteration(const Iteration& iter) { iterations.push_back(iter); }
			const std::list<Iteration>& GetIterations() const { return iterations; }

			unsigned int GetFrame() const { return frame; }
		private:
			std::list<Iteration> iterations;

			// sim-frame at which the search was executed
			unsigned int frame;
		};
	};


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
		virtual bool Execute(
			PathCache* cache,
			NodeLayer* layer,
			PathSearchTrace::Execution* exec,
			unsigned int searchStateOffset = 0,
			unsigned int searchMagicNumber = 0
		) = 0;
		virtual void Finalize(IPath* path, bool replace) = 0;

		void SetID(unsigned int n) { searchID = n; }
		void SetTeam(unsigned int n) { searchTeam = n; }
		unsigned int GetID() const { return searchID; }
		unsigned int GetTeam() const { return searchTeam; }

	protected:
		unsigned int searchID;     // links us to the temp-path that this search will finalize
		unsigned int searchTeam;   // which team queued this search

		unsigned int searchType;   // indicates whether A* or Dijkstra search is employed
		unsigned int searchState;  // offset that identifies nodes as part of current search
		unsigned int searchMagic;  // used to signal nodes they should update their neighbor-set
	};


	struct PathSearch: public IPathSearch {
	public:
		PathSearch(unsigned int pathSearchType): IPathSearch(pathSearchType) {}
		~PathSearch() { while (!openNodes.empty()) { openNodes.pop(); } }

		void Initialize(const float3& sourcePoint, const float3& targetPoint);
		bool Execute(
			PathCache* cache,
			NodeLayer* layer,
			PathSearchTrace::Execution* exec,
			unsigned int searchStateOffset = 0,
			unsigned int searchMagicNumber = 0
		);
		void Finalize(IPath* path, bool replace);

	private:
		void IterateSearch(
			const std::vector<INode*>& allNodes,
			      std::vector<INode*>& ngbNodes,
			      PathSearchTrace::Iteration* iter
			);
		void FillPath(IPath* path);
		void UpdateNode(INode* nxtNode, INode* curNode, float gCost);
		void UpdateQueue();

		PathCache* pathCache;
		NodeLayer* nodeLayer;

		float3 srcPoint, tgtPoint;
		float3 curPoint, nxtPoint;

		INode *srcNode, *tgtNode;
		INode *curNode, *nxtNode;

		// rely on INode::operator() to sort the INode*'s by increasing f-cost
		// std::priority_queue<INode*, std::vector<INode*>, INode> openNodes;
		reservable_priority_queue<INode*, std::vector<INode*>, INode> openNodes;

		bool haveOpenNode;
		bool haveFullPath;

		float hCostMult;
	};
};

#endif

