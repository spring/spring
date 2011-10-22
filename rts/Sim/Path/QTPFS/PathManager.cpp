/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <omp.h>

#include "PathManager.hpp"
#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/Rectangle.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"

#define NO_LOADSCREEN
#ifdef NO_LOADSCREEN
	struct DummyLoadScreen { void SetLoadMessage(const std::string& msg) const { LOG("%s", msg.c_str()); } };
	static DummyLoadScreen dummyLoadScreen;
	#undef loadscreen
	#define loadscreen (&dummyLoadScreen)
#endif



QTPFS::NodeLayer* QTPFS::PathManager::serializingNodeLayer = NULL;

static size_t GetNumThreads() {
	size_t numThreads = std::max(0, configHandler->GetInt("HardwareThreadCount"));

	if (numThreads == 0) {
		// auto-detect
		#if (BOOST_VERSION >= 103500)
		numThreads = boost::thread::hardware_concurrency();
		#elif defined(USE_GML)
		numThreads = gmlCPUCount();
		#else
		numThreads = 1;
		#endif
	}

	return numThreads;
}



QTPFS::PathManager::PathManager() {
	// NOTE: offset *must* start at a non-zero value
	searchStateOffset = NODE_STATE_OFFSET;
	numTerrainChanges = 0;
	numPathRequests   = 0;

	nodeTrees.resize(moveinfo->moveData.size(), NULL);
	nodeLayers.resize(moveinfo->moveData.size());
	pathCaches.resize(moveinfo->moveData.size());
	pathSearches.resize(moveinfo->moveData.size());

	const std::string& cacheDirName = GetCacheDirName(gameSetup->mapName, gameSetup->modName);
	const bool haveCacheDir = FileSystem::DirExists(cacheDirName);

	static const SRectangle mapRect = SRectangle(0, 0,  gs->mapx, gs->mapy);

	{
		streflop_init<streflop::Simple>();

		char loadMsg[512] = {'\0'};
		const char* fmtString = "[%s] using %u threads for %u node-layers (cached? %s)";

		#ifdef OPENMP
			// never use more threads than the number of layers
			// FIXME: this needs project-global linking changes
			//
			// const unsigned int maxThreads = omp_get_max_threads();
			// const unsigned int minThreads = std::min(size_t(GetNumThreads()), nodeLayers.size());
			//
			// omp_set_num_threads(std::min(maxThreads, minThreads));

			sprintf(loadMsg, fmtString, __FUNCTION__, omp_get_num_threads(), nodeLayers.size(), (haveCacheDir? "true": "false"));
			loadscreen->SetLoadMessage(loadMsg);

			#pragma omp parallel for private(loadMsg)
			for (unsigned int i = 0; i < nodeLayers.size(); i++) {
				sprintf(loadMsg, "  initializing node-layer %u (thread %u)", i, omp_get_thread_num());
				loadscreen->SetLoadMessage(loadMsg);

				nodeTrees[i] = new QTPFS::QTNode(0,  mapRect.x1, mapRect.z1,  mapRect.x2, mapRect.z2);
				nodeLayers[i].Init();
				nodeLayers[i].RegisterNode(nodeTrees[i]);

				// construct each tree from scratch IFF no cache-dir exists
				// (if it does, we only need to initialize speed{Mods, Bins})
				UpdateNodeLayer(i, mapRect, !haveCacheDir);

				sprintf(loadMsg, "  initialized node-layer %u (%u leafs, ratio %f)", i, nodeLayers[i].GetNumLeafNodes(), nodeLayers[i].GetNodeRatio());
				loadscreen->SetLoadMessage(loadMsg);
			}

		#else

			sprintf(loadMsg, fmtString, __FUNCTION__, GetNumThreads(), nodeLayers.size(), (haveCacheDir? "true": "false"));
			loadscreen->SetLoadMessage(loadMsg);

			std::vector<boost::thread*> threads(std::min(GetNumThreads(), nodeLayers.size()), NULL);

			for (unsigned int i = 0; i < threads.size(); i++) {
				threads[i] = new boost::thread(boost::bind(&PathManager::InitNodeLayers, this, i, threads.size(), mapRect, haveCacheDir));
			}
			for (unsigned int i = 0; i < threads.size(); i++) {
				threads[i]->join(); delete threads[i];
			}
		#endif

		streflop_init<streflop::Simple>();
	}

	Serialize(cacheDirName);
}

QTPFS::PathManager::~PathManager() {
	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		nodeTrees[i]->Delete();
		nodeLayers[i].Clear();
		pathSearches[i].clear();
	}

	nodeTrees.clear();
	nodeLayers.clear();
	pathCaches.clear();
	pathSearches.clear();
}



void QTPFS::PathManager::InitNodeLayers(unsigned int threadNum, unsigned int numThreads, const SRectangle& mapRect, bool haveCacheDir) {
	static const unsigned int layersPerThread = (nodeLayers.size() / numThreads);
	static const unsigned int numExcessLayers = (threadNum == (numThreads - 1))?
		(nodeLayers.size() % numThreads): 0;

	const unsigned int minLayer = threadNum * layersPerThread;
	const unsigned int maxLayer = minLayer + layersPerThread + numExcessLayers;

	char loadMsg[512] = {'\0'};

	for (unsigned int i = minLayer; i < maxLayer; i++) {
		sprintf(loadMsg, "  initializing node-layer %u (thread %u)", i, threadNum);
		loadscreen->SetLoadMessage(loadMsg);

		nodeTrees[i] = new QTPFS::QTNode(0,  mapRect.x1, mapRect.z1,  mapRect.x2, mapRect.z2);
		nodeLayers[i].Init();
		nodeLayers[i].RegisterNode(nodeTrees[i]);

		UpdateNodeLayer(i, mapRect, !haveCacheDir);

		sprintf(loadMsg, "  initialized node-layer %u (%u leafs, ratio %f)", i, nodeLayers[i].GetNumLeafNodes(), nodeLayers[i].GetNodeRatio());
		loadscreen->SetLoadMessage(loadMsg);
	}
}



std::string QTPFS::PathManager::GetCacheDirName(const std::string& mapArchiveName, const std::string& modArchiveName) const {
	static const unsigned int mapCheckSum = archiveScanner->GetArchiveCompleteChecksum(mapArchiveName);
	static const unsigned int modCheckSum = archiveScanner->GetArchiveCompleteChecksum(modArchiveName);

	static const std::string dir =
		"cache/PathNodeTrees/" +
		IntToString(mapCheckSum, "%08x") + "-" +
		IntToString(modCheckSum, "%08x") + "/";

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[%s] using cache-dir %s (map-checksum %08x, mod-checksum %08x)";

	sprintf(loadMsg, fmtString, __FUNCTION__, dir.c_str(), mapCheckSum, modCheckSum);
	loadscreen->SetLoadMessage(loadMsg);
	return dir;
}

void QTPFS::PathManager::Serialize(const std::string& cacheFileDir) {
	return; // FIXME

	std::vector<std::string> fileNames(nodeTrees.size());
	std::vector<std::fstream*> fileStreams(nodeTrees.size());

	if (!FileSystem::DirExists(cacheFileDir)) {
		FileSystem::CreateDirectory(cacheFileDir);
		assert(FileSystem::DirExists(cacheFileDir));
	}

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[%s] serializing node-tree %u";

	// TODO: calculate checksum over each tree
	// NOTE: also compress the tree cache-files?
	for (unsigned int i = 0; i < nodeTrees.size(); i++) {
		fileNames[i] = cacheFileDir + "tree" + IntToString(i, "%02x");
		fileStreams[i] = new std::fstream();

		if (FileSystem::FileExists(fileNames[i])) {
			// read the i-th tree
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::in | std::ios::binary);
		} else {
			// write the i-th tree
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::out | std::ios::binary);
		}

		sprintf(loadMsg, fmtString, __FUNCTION__, i);
		loadscreen->SetLoadMessage(loadMsg);

		serializingNodeLayer = &nodeLayers[i];
		nodeTrees[i]->Serialize(*fileStreams[i], FileSystem::FileExists(fileNames[i]));
		serializingNodeLayer = NULL;

		fileStreams[i]->flush();
		fileStreams[i]->close();

		delete fileStreams[i];
	}
}



// NOTE:
//     gets called during initialization for trees etc., but
//     never with a map-covering rectangle (which constructor
//     takes care of)
//
//     all layers *must* be updated on the same frame
//
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2) {
	const SRectangle r = SRectangle(x1, z1,  x2, z2);

	{
		streflop_init<streflop::Simple>();

		#pragma omp parallel for
		for (unsigned int i = 0; i < nodeLayers.size(); i++) {
			UpdateNodeLayer(i, r, true);
		}

		streflop_init<streflop::Simple>();
	}

	numTerrainChanges += 1;
}

void QTPFS::PathManager::UpdateNodeLayer(unsigned int i, const SRectangle& r, bool wantTesselation) {
	const MoveData*  md = moveinfo->moveData[i];
	const CMoveMath* mm = md->moveMath;

	if (md->unitDefRefCount == 0)
		return;

	if (nodeLayers[i].Update(r, md, mm) && wantTesselation) {
		nodeTrees[i]->PreTesselate(nodeLayers[i], r);
		pathCaches[i].MarkDeadPaths(r);
	}
}






void QTPFS::PathManager::Update() {
	for (unsigned int i = 0; i < pathCaches.size(); i++) {
		PathCache& pathCache = pathCaches[i];
		PathCache::PathIDSet::const_iterator deadPathsIt;

		const MoveData* moveData = moveinfo->moveData[i];
		const PathCache::PathIDSet& deadPaths = pathCache.GetDeadPaths();

		assert(i == moveData->pathType);

		std::list<unsigned int> replacedPathIDs;
		std::list<IPathSearch*>& searches = pathSearches[i];
		std::list<IPathSearch*>::iterator searchesIt;

		// execute all pending searches collected via RequestPath
		for (searchesIt = searches.begin(); searchesIt != searches.end(); ++searchesIt) {
			IPathSearch* search = *searchesIt;
			IPath* path = pathCache.GetTempPath(search->GetID());

			// temp-path might have been removed already via DeletePath
			if (path->GetID() == 0)
				continue;

			assert(search->GetID() != 0);
			assert(path->GetID() == search->GetID());

			search->Initialize(path->GetSourcePoint(), path->GetTargetPoint());

			// removes path from temp-paths, adds it to live-paths
			if (search->Execute(&pathCaches[i], &nodeLayers[i], searchStateOffset)) {
				search->Finalize(path, false);
				delete search;
			}

			searchStateOffset += NODE_STATE_OFFSET;
		}

		searches.clear();

		// re-request LIVE paths that were marked as DEAD after a deformation
		for (deadPathsIt = deadPaths.begin(); deadPathsIt != deadPaths.end(); ++deadPathsIt) {
			const IPath* oldPath = pathCache.GetLivePath(*deadPathsIt);

			// path was deleted after being marked
			// dead, but before being re-requested
			if (oldPath->GetID() == 0)
				continue;

			IPath* newPath = new Path();
			IPathSearch* newSearch = new PathSearch(PATH_SEARCH_ASTAR);

			newPath->SetID(oldPath->GetID());
			newPath->SetOwnerID(oldPath->GetOwnerID());
			newPath->SetRadius(oldPath->GetRadius());
			newPath->SetSynced(oldPath->GetSynced());

			// start re-request from the current position
			// along the path, not the original source
			newSearch->SetID(oldPath->GetID());
			newSearch->Initialize(oldPath->GetPoint(oldPath->GetPointIdx() - 1), oldPath->GetTargetPoint());

			if (newSearch->Execute(&pathCaches[i], &nodeLayers[i], searchStateOffset)) {
				newSearch->Finalize(newPath, true);
				delete newSearch;

				// path was succesfully re-requested, remove
				// it from the DEAD paths before killing them
				replacedPathIDs.push_back(oldPath->GetID());
			} else {
				// failed to re-request, so path is still dead
				// and will be cleaned up next by KillDeadPaths
				pathCacheMap.erase(oldPath->GetID());
			}

			searchStateOffset += NODE_STATE_OFFSET;
		}

		// NOTE:
		//     any path that was not succesfully re-requested
		//     will now have a "dangling" ID in GroundMoveType
		//     --> all subsequent NextWaypoint (and DeletePath)
		//     calls will be no-ops
		pathCache.KillDeadPaths(replacedPathIDs);
	}
}

float3 QTPFS::PathManager::NextWaypoint(unsigned int pathID, float3 curPoint, float rad, int, int, bool) {
	const PathCacheMap::const_iterator cacheIt = pathCacheMap.find(pathID);

	// dangling ID after re-request failure or regular deletion
	if (cacheIt == pathCacheMap.end())
		return curPoint;

	IPath* tempPath = pathCaches[cacheIt->second].GetTempPath(pathID);
	IPath* livePath = pathCaches[cacheIt->second].GetLivePath(pathID);

	if (tempPath->GetID() != 0) {
		// path-request has not yet been processed
		// (so ID still maps to a temporary path);
		// just set the unit off toward its target
		assert(livePath->GetID() == 0);
		return curPoint;
	}

	assert(livePath->GetID() != 0);

	const float radSq = std::max(float(SQUARE_SIZE), rad * rad);

	unsigned int curPointIdx =  0;
	unsigned int nxtPointIdx = -1U;

	// find the point furthest along the
	// path within distance <rad> of <pos>
	for (unsigned int i = 0; i < (livePath->NumPoints() - 1); i++) {
		const float3& point = livePath->GetPoint(i);

		if ((curPoint - point).SqLength() < radSq) {
			curPointIdx = i;
			nxtPointIdx = i + 1;
		}
	}

	if (curPointIdx > 0 && nxtPointIdx != -1U) {
		// if close enough to at least one <curPoint>,
		// switch to <nxtPoint> immediately after it
		livePath->SetPointIdx(nxtPointIdx);
	}

	return (livePath->GetPoint(livePath->GetPointIdx()));
}

void QTPFS::PathManager::DeletePath(unsigned int pathID) {
	const PathCacheMapIt cacheIt = pathCacheMap.find(pathID);

	if (cacheIt != pathCacheMap.end()) {
		PathCache& pathCache = pathCaches[cacheIt->second];
		pathCache.DelPath(pathID);
		pathCacheMap.erase(cacheIt);
	}
}



unsigned int QTPFS::PathManager::RequestPath(
	const MoveData* moveData,
	const float3& sourcePoint,
	const float3& targetPoint,
	float radius,
	CSolidObject* object,
	bool synced)
{
	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	IPath* path = new Path();
	IPathSearch* pathSearch = new PathSearch(PATH_SEARCH_ASTAR);

	// NOTE:
	//     the unclamped end-points are temporary
	//     zero is a reserved ID, so pre-increment
	path->SetID(++numPathRequests);
	path->SetOwnerID((object != NULL)? object->id: -1U);
	path->SetRadius(radius);
	path->SetSynced(synced);
	path->SetEndPoints(sourcePoint, targetPoint);
	pathSearch->SetID(path->GetID());

	// TODO:
	//     introduce synced and unsynced path-caches;
	//     somehow support extra-cost overlays again
	pathSearches[moveData->pathType].push_back(pathSearch);
	pathCaches[moveData->pathType].AddTempPath(path);

	// map the path-ID to the index of the cache that stores it
	pathCacheMap[path->GetID()] = moveData->pathType;
	return (path->GetID());
}

