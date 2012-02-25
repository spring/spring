/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHSEARCH_HDR
#define QTPFS_PATHSEARCH_HDR

#include <map>
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
			Execution(unsigned int f): searchFrame(f) {}
			~Execution() { iterations.clear(); }

			void AddIteration(const Iteration& iter) { iterations.push_back(iter); }
			const std::list<Iteration>& GetIterations() const { return iterations; }

			unsigned int GetFrame() const { return searchFrame; }
		private:
			std::list<Iteration> iterations;

			// sim-frame at which the search was executed
			unsigned int searchFrame;
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
		IPathSearch(unsigned int pathSearchType)
			: searchID(0)
			, searchTeam(0)
			, searchType(pathSearchType)
			, searchState(0)
			, searchMagic(0)
			{}
		virtual ~IPathSearch() {}

		virtual void Initialize(
			NodeLayer* layer,
			PathCache* cache,
			const float3& sourcePoint,
			const float3& targetPoint
		) = 0;
		virtual bool Execute(
			unsigned int searchStateOffset = 0,
			unsigned int searchMagicNumber = 0
		) = 0;
		virtual void Finalize(IPath* path) = 0;
		virtual void SharedFinalize(const IPath* srcPath, IPath* dstPath) {}
		virtual PathSearchTrace::Execution* GetExecutionTrace() { return NULL; }

		virtual const boost::uint64_t GetHash(unsigned int N, unsigned int k) const = 0;

		void SetID(unsigned int n) { searchID = n; }
		void SetTeam(unsigned int n) { searchTeam = n; }
		unsigned int GetID() const { return searchID; }
		unsigned int GetTeam() const { return searchTeam; }

	protected:
		unsigned int searchID;     // links us to the temp-path that this search will finalize
		unsigned int searchTeam;   // which team queued this search

		unsigned int searchType;   // indicates if Dijkstra (h==0) or A* (h!=0) search is employed
		unsigned int searchState;  // offset that identifies nodes as part of current search
		unsigned int searchMagic;  // used to signal nodes they should update their neighbor-set
	};


	struct PathSearch: public IPathSearch {
	public:
		PathSearch(unsigned int pathSearchType)
			: IPathSearch(pathSearchType)
			, nodeLayer(NULL)
			, pathCache(NULL)
			, srcNode(NULL)
			, tgtNode(NULL)
			, curNode(NULL)
			, nxtNode(NULL)
			, minNode(NULL)
			, searchExec(NULL)
			, haveFullPath(false)
			, havePartPath(false)
			, hCostMult(0.0f)
			{}
		~PathSearch() { openNodes.reset(); }

		void Initialize(
			NodeLayer* layer,
			PathCache* cache,
			const float3& sourcePoint,
			const float3& targetPoint
		);
		bool Execute(
			unsigned int searchStateOffset = 0,
			unsigned int searchMagicNumber = 0
		);
		void Finalize(IPath* path);
		void SharedFinalize(const IPath* srcPath, IPath* dstPath);
		PathSearchTrace::Execution* GetExecutionTrace() { return searchExec; }

		const boost::uint64_t GetHash(unsigned int N, unsigned int k) const;

		static void InitGlobalQueue(unsigned int n) { openNodes.reserve(n); }
		static void FreeGlobalQueue() { openNodes.clear(); }

	private:
		void IterateSearch(
			const std::vector<INode*>& allNodes,
			      std::vector<INode*>& ngbNodes
		);
		void TracePath(IPath* path);
		void SmoothPath(IPath* path);
		void UpdateNode(
			INode* nxtNode,
			INode* curNode,
			float gCost,
			float hCost,
			float mCost
		);
		void UpdateQueue();

		NodeLayer* nodeLayer;
		PathCache* pathCache;

		float3 srcPoint, tgtPoint;
		float3 curPoint, nxtPoint;

		INode *srcNode, *tgtNode;
		INode *curNode, *nxtNode;
		INode *minNode;

		// not used unless QTPFS_TRACE_PATH_SEARCHES is defined
		PathSearchTrace::Execution* searchExec;
		PathSearchTrace::Iteration searchIter;

		// global queue: allocated once, re-used by all searches without clear()'s
		// this relies on INode::operator< to sort the INode*'s by increasing f-cost
		static binary_heap<INode*> openNodes;

		bool haveFullPath;
		bool havePartPath;

		float hCostMult;
	};
};

#endif

