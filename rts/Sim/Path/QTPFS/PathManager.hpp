/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHMANAGER_HDR
#define QTPFS_PATHMANAGER_HDR

#include <vector>

#include "Sim/Path/IPathManager.h"
#include "NodeLayer.hpp"
#include "PathCache.hpp"
#include "PathSearch.hpp"
#include "System/UnorderedMap.hpp"

struct MoveDef;
struct SRectangle;
class CSolidObject;

#ifdef QTPFS_ENABLE_THREADED_UPDATE
namespace spring {
	class mutex;
	class thread;
	class condition_variable;
};
#endif

namespace QTPFS {
	struct QTNode;
	class PathManager: public IPathManager {
	public:
		PathManager();
		~PathManager();

		static void InitStatic();

		std::int32_t GetPathFinderType() const override { return QTPFS_TYPE; }
		std::uint32_t GetPathCheckSum() const override { return pfsCheckSum; }

		std::int64_t Finalize() override;

		bool PathUpdated(unsigned int pathID) override;

		void TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type) override;
		void Update() override;
		void UpdatePath(const CSolidObject* owner, unsigned int pathID) override;
		void DeletePath(unsigned int pathID) override;

		unsigned int RequestPath(
			CSolidObject* object,
			const MoveDef* moveDef,
			float3 sourcePos,
			float3 targetPos,
			float radius,
			bool synced
		) override;

		float3 NextWayPoint(
			const CSolidObject*, // owner
			unsigned int pathID,
			unsigned int, // numRetries
			float3 point,
			float radius,
			bool synced
		) override;

		void GetPathWayPoints(
			unsigned int pathID,
			std::vector<float3>& points,
			std::vector<int>& starts
		) const override;

		int2 GetNumQueuedUpdates() const override;


		const NodeLayer& GetNodeLayer(unsigned int pathType) const { return nodeLayers[pathType]; }
		const QTNode* GetNodeTree(unsigned int pathType) const { return nodeTrees[pathType]; }
		const PathCache& GetPathCache(unsigned int pathType) const { return pathCaches[pathType]; }

		const spring::unordered_map<unsigned int, unsigned int>& GetPathTypes() const { return pathTypes; }
		const spring::unordered_map<unsigned int, PathSearchTrace::Execution*>& GetPathTraces() const { return pathTraces; }

	private:
		void ThreadUpdate();
		void Load();

		std::uint64_t GetMemFootPrint() const;

		typedef void (PathManager::*MemberFunc)(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect
		);
		typedef spring::unordered_map<unsigned int, unsigned int> PathTypeMap;
		typedef spring::unordered_map<unsigned int, unsigned int>::iterator PathTypeMapIt;
		typedef spring::unordered_map<unsigned int, PathSearchTrace::Execution*> PathTraceMap;
		typedef spring::unordered_map<unsigned int, PathSearchTrace::Execution*>::iterator PathTraceMapIt;
		typedef spring::unordered_map<std::uint64_t, IPath*> SharedPathMap;
		typedef spring::unordered_map<std::uint64_t, IPath*>::iterator SharedPathMapIt;

		typedef std::vector<IPathSearch*> PathSearchVect;
		typedef std::vector<IPathSearch*>::iterator PathSearchVectIt;

		void SpawnSpringThreads(MemberFunc f, const SRectangle& r);

		void InitNodeLayersThreaded(const SRectangle& rect);
		void UpdateNodeLayersThreaded(const SRectangle& rect);
		void InitNodeLayersThread(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect
		);
		void UpdateNodeLayersThread(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect
		);
		void InitNodeLayer(unsigned int layerNum, const SRectangle& r);
		void UpdateNodeLayer(unsigned int layerNum, const SRectangle& r);

		#ifdef QTPFS_STAGGERED_LAYER_UPDATES
		void QueueNodeLayerUpdates(const SRectangle& r);
		void ExecQueuedNodeLayerUpdates(unsigned int layerNum, bool flushQueue);
		#endif

		void ExecuteQueuedSearches(unsigned int pathType);
		void QueueDeadPathSearches(unsigned int pathType);

		unsigned int QueueSearch(
			const IPath* oldPath,
			const CSolidObject* object,
			const MoveDef* moveDef,
			const float3& sourcePoint,
			const float3& targetPoint,
			const float radius,
			const bool synced
		);

		bool ExecuteSearch(
			PathSearchVect& searches,
			PathSearchVectIt& searchesIt,
			NodeLayer& nodeLayer,
			PathCache& pathCache,
			unsigned int pathType
		);

		bool IsFinalized() const { return (!nodeTrees.empty()); }


		std::string GetCacheDirName(const std::string& mapCheckSumHexStr, const std::string& modCheckSumHexStr) const;
		void Serialize(const std::string& cacheFileDir);

		static std::vector<NodeLayer> nodeLayers;
		static std::vector<QTNode*> nodeTrees;
		static std::vector<PathCache> pathCaches;
		static std::vector< std::vector<IPathSearch*> > pathSearches;

		spring::unordered_map<unsigned int, unsigned int> pathTypes;
		spring::unordered_map<unsigned int, PathSearchTrace::Execution*> pathTraces;

		// maps "hashes" of executed searches to the found paths
		spring::unordered_map<std::uint64_t, IPath*> sharedPaths;

		std::vector<unsigned int> numCurrExecutedSearches;
		std::vector<unsigned int> numPrevExecutedSearches;

		static unsigned int LAYERS_PER_UPDATE;
		static unsigned int MAX_TEAM_SEARCHES;

		unsigned int searchStateOffset;
		unsigned int numTerrainChanges;
		unsigned int numPathRequests;
		unsigned int maxNumLeafNodes;

		std::uint32_t pfsCheckSum;

		bool layersInited;
		bool haveCacheDir;

		#ifdef QTPFS_ENABLE_THREADED_UPDATE
		spring::thread updateThread;
		spring::mutex mutexThreadUpdate;
		spring::condition_variable condThreadUpdate;
		spring::condition_variable condThreadUpdated;
		#endif
	};
}

#endif

