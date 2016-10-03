/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATHMANAGER_HDR
#define QTPFS_PATHMANAGER_HDR

#include <map>
#include <list>
#include <vector>

#include "Sim/Path/IPathManager.h"
#include "NodeLayer.hpp"
#include "PathCache.hpp"
#include "PathSearch.hpp"

struct MoveDef;
struct SRectangle;
class CSolidObject;

#ifdef QTPFS_ENABLE_THREADED_UPDATE
namespace boost {
	class thread;
	class condition_variable;
};
namespace spring {
	class mutex;
};
#endif

namespace QTPFS {
	struct QTNode;
	class PathManager: public IPathManager {
	public:
		PathManager();
		~PathManager();

		static void InitStatic();

		unsigned int GetPathFinderType() const { return PFS_TYPE_QTPFS; }
		std::uint32_t GetPathCheckSum() const { return pfsCheckSum; }

		std::int64_t Finalize();

		bool PathUpdated(unsigned int pathID);

		void TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type);
		void Update();
		void UpdatePath(const CSolidObject* owner, unsigned int pathID);
		void DeletePath(unsigned int pathID);

		unsigned int RequestPath(
			CSolidObject* object,
			const MoveDef* moveDef,
			float3 sourcePos,
			float3 targetPos,
			float radius,
			bool synced
		);

		float3 NextWayPoint(
			const CSolidObject*, // owner
			unsigned int pathID,
			unsigned int, // numRetries
			float3 point,
			float radius,
			bool synced
		);

		void GetPathWayPoints(
			unsigned int pathID,
			std::vector<float3>& points,
			std::vector<int>& starts
		) const;

		int2 GetNumQueuedUpdates() const;

	private:
		void ThreadUpdate();
		void Load();

		std::uint64_t GetMemFootPrint() const;

		typedef void (PathManager::*MemberFunc)(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect
		);
		typedef std::map<unsigned int, unsigned int> PathTypeMap;
		typedef std::map<unsigned int, unsigned int>::iterator PathTypeMapIt;
		typedef std::map<unsigned int, PathSearchTrace::Execution*> PathTraceMap;
		typedef std::map<unsigned int, PathSearchTrace::Execution*>::iterator PathTraceMapIt;
		typedef std::map<std::uint64_t, IPath*> SharedPathMap;
		typedef std::map<std::uint64_t, IPath*>::iterator SharedPathMapIt;
		typedef std::list<IPathSearch*> PathSearchList;
		typedef std::list<IPathSearch*>::iterator PathSearchListIt;

		void SpawnBoostThreads(MemberFunc f, const SRectangle& r);

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
			PathSearchList& searches,
			PathSearchListIt& searchesIt,
			NodeLayer& nodeLayer,
			PathCache& pathCache,
			unsigned int pathType
		);

		bool IsFinalized() const { return (!nodeTrees.empty()); }


		std::string GetCacheDirName(std::uint32_t mapCheckSum, std::uint32_t modCheckSum) const;
		void Serialize(const std::string& cacheFileDir);

		std::vector<NodeLayer> nodeLayers;
		std::vector<QTNode*> nodeTrees;
		std::vector<PathCache> pathCaches;
		std::vector< std::list<IPathSearch*> > pathSearches;
		std::map<unsigned int, unsigned int> pathTypes;
		std::map<unsigned int, PathSearchTrace::Execution*> pathTraces;

		// maps "hashes" of executed searches to the found paths
		std::map<std::uint64_t, IPath*> sharedPaths;

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
		boost::thread* updateThread;
		spring::mutex* mutexThreadUpdate;
		boost::condition_variable* condThreadUpdate;
		boost::condition_variable* condThreadUpdated;
		#endif
	};
}

#endif

