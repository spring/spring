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

struct MoveData;
struct SRectangle;
class CSolidObject;

namespace QTPFS {
	struct QTNode;
	class PathManager: public IPathManager {
	public:
		PathManager();
		~PathManager();

		boost::uint32_t GetPathCheckSum() const { return pfsCheckSum; }

		void TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2);
		void Update();
		void UpdatePath(const CSolidObject* owner, unsigned int pathID);
		void DeletePath(unsigned int pathID);

		unsigned int RequestPath(
			const MoveData* moveData,
			const float3& sourcePos,
			const float3& targetPos,
			float radius,
			CSolidObject* object,
			bool synced
		);

		float3 NextWayPoint(
			unsigned int pathID,
			float3 point,
			float radius = 0.0f,
			int = 0, // numRetries
			int = 0, // ownerID
			bool synced = true
		);

		void GetPathWayPoints(
			unsigned int pathID,
			std::vector<float3>& points,
			std::vector<int>& starts
		) const;

		static NodeLayer* GetSerializingNodeLayer() { return serializingNodeLayer; }

		static const unsigned int MAX_UPDATE_DELAY = 10;
		static const unsigned int MAX_TEAM_SEARCHES = 10;
		static const unsigned int NUM_SPEEDMOD_BINS = 20;
		static const float MIN_SPEEDMOD_VALUE;
		static const float MAX_SPEEDMOD_VALUE;

	private:
		void Load();

		boost::uint64_t GetMemFootPrint() const;

		typedef void (PathManager::*MemberFunc)(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect,
			bool wantTesselation
		);
		typedef std::map<unsigned int, unsigned int> PathTypeMap;
		typedef std::map<unsigned int, unsigned int>::iterator PathTypeMapIt;
		typedef std::map<unsigned int, PathSearchTrace::Execution*> PathTraceMap;
		typedef std::map<unsigned int, PathSearchTrace::Execution*>::iterator PathTraceMapIt;
		typedef std::map<boost::uint64_t, IPath*> SharedPathMap;
		typedef std::map<boost::uint64_t, IPath*>::iterator SharedPathMapIt;
		typedef std::list<IPathSearch*> PathSearchList;
		typedef std::list<IPathSearch*>::iterator PathSearchListIt;

		void SpawnBoostThreads(MemberFunc f, const SRectangle& r, bool b);

		void InitNodeLayersThreaded(const SRectangle& rect, bool haveCacheDir);
		void UpdateNodeLayersThreaded(const SRectangle& rect);
		void InitNodeLayersThread(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect,
			bool haveCacheDir
		);
		void UpdateNodeLayersThread(
			unsigned int threadNum,
			unsigned int numThreads,
			const SRectangle& rect,
			bool wantTesselation
		);
		void InitNodeLayer(unsigned int layerNum, const SRectangle& rect);
		void UpdateNodeLayer(unsigned int layerNum, const SRectangle& r, bool wantTesselation);

		void ExecuteQueuedSearches(unsigned int pathType);
		void QueueDeadPathSearches(unsigned int pathType);

		unsigned int QueueSearch(
			const IPath* oldPath,
			const CSolidObject* object,
			const MoveData* moveData,
			const float3& sourcePoint,
			const float3& targetPoint,
			const float radius,
			const bool synced
		);

		void ExecuteSearch(
			PathSearchList& searches,
			PathSearchListIt& searchesIt,
			NodeLayer& nodeLayer,
			PathCache& pathCache,
			unsigned int pathType
		);


		std::string GetCacheDirName(boost::uint32_t mapCheckSum, boost::uint32_t modCheckSum) const;
		void Serialize(const std::string& cacheFileDir);

		std::vector<NodeLayer> nodeLayers;
		std::vector<QTNode*> nodeTrees;
		std::vector<PathCache> pathCaches;
		std::vector< std::list<IPathSearch*> > pathSearches;
		std::map<unsigned int, unsigned int> pathTypes;
		std::map<unsigned int, PathSearchTrace::Execution*> pathTraces;

		// maps "hashes" of executed searches to the found paths
		std::map<boost::uint64_t, IPath*> sharedPaths;

		std::vector<unsigned int> numCurrExecutedSearches;
		std::vector<unsigned int> numPrevExecutedSearches;


		static NodeLayer* serializingNodeLayer;

		unsigned int searchStateOffset;
		unsigned int numTerrainChanges;
		unsigned int numPathRequests;
		unsigned int maxNumLayerNodes;

		boost::uint32_t pfsCheckSum;
	};
};

#endif

