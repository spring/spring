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

		void TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2);
		void Update();
		void DeletePath(unsigned int pathID);

		unsigned int RequestPath(
			const MoveData* moveData,
			const float3& sourcePos,
			const float3& targetPos,
			float radius,
			CSolidObject* object,
			bool synced
		);

		float3 NextWaypoint(unsigned int pathID, float3 pos, float rad, int, int, bool);
		static NodeLayer* GetSerializingNodeLayer() { return serializingNodeLayer; }

		static const unsigned int NUM_SPEEDMOD_BINS = 20;
		static const float MIN_SPEEDMOD_VALUE = 0.0f;
		static const float MAX_SPEEDMOD_VALUE = 2.0f;

	private:
		std::string GetCacheDirName(const std::string& mapArchiveName, const std::string& modArchiveName) const;
		void InitNodeLayers(unsigned int threadNum, unsigned int numThreads, const SRectangle& mapRect, bool haveCacheDir);
		void UpdateNodeLayer(unsigned int i, const SRectangle& r, bool wantTesselation);
		void Serialize(const std::string& cacheFileDir);

		std::vector<NodeLayer> nodeLayers;
		std::vector<QTNode*> nodeTrees;
		std::vector<PathCache> pathCaches;
		std::map<unsigned int, unsigned int> pathCacheMap;
		std::vector< std::list<IPathSearch*> > pathSearches;

		typedef std::map<unsigned int, unsigned int> PathCacheMap;
		typedef std::map<unsigned int, unsigned int>::iterator PathCacheMapIt;

		static NodeLayer* serializingNodeLayer;

		unsigned int searchStateOffset;
		unsigned int numTerrainChanges;
		unsigned int numPathRequests;
	};
};

#endif

