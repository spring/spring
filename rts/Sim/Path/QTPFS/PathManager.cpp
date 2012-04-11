/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>

#include "System/OpenMP_cond.h"
#include "lib/gml/gml.h" // for gmlCPUCount

#include "PathDefines.hpp"
#include "PathManager.hpp"
#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/Rectangle.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

#define NUL_RECTANGLE SRectangle(0, 0,         0,        0)
#define MAP_RECTANGLE SRectangle(0, 0,  gs->mapx, gs->mapy)

namespace QTPFS {
	const float PathManager::MIN_SPEEDMOD_VALUE = 0.0f;
	const float PathManager::MAX_SPEEDMOD_VALUE = 2.0f;

	struct PMLoadScreen {
		PMLoadScreen(): loading(true) {}
		~PMLoadScreen() { assert(loadMessages.empty()); }

		void SetLoading(bool b) { loading = b; }
		void AddLoadMessage(const std::string& msg) {
			boost::mutex::scoped_lock loadMessageLock(loadMessageMutex);
			loadMessages.push_back(msg);
		}
		void SetLoadMessage(const std::string& msg) {
			#ifdef QTPFS_NO_LOADSCREEN
			LOG("%s", msg.c_str());
			#else
			loadscreen->SetLoadMessage(msg);
			#endif
		}
		void SetLoadMessages() {
			boost::mutex::scoped_lock loadMessageLock(loadMessageMutex);

			while (!loadMessages.empty()) {
				SetLoadMessage(loadMessages.front());
				loadMessages.pop_front();
			}
		}
		void Loop() {
			while (loading) {
				boost::this_thread::sleep(boost::posix_time::millisec(50));

				// need this to be always executed after waking up
				SetLoadMessages();
			}

			// handle any leftovers
			SetLoadMessages();
		}

	private:
		std::list<std::string> loadMessages;
		boost::mutex loadMessageMutex;

		volatile bool loading;
	};

	static PMLoadScreen pmLoadScreen;
	static boost::thread pmLoadThread;

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

	NodeLayer* PathManager::serializingNodeLayer = NULL;
}



QTPFS::PathManager::PathManager() {
	pmLoadThread = boost::thread(boost::bind(&PathManager::Load, this));
	pmLoadScreen.Loop();
	pmLoadThread.join();
}

QTPFS::PathManager::~PathManager() {
	std::list<IPathSearch*>::const_iterator searchesIt;
	std::map<unsigned int, PathSearchTrace::Execution*>::const_iterator tracesIt;

	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		nodeTrees[i]->Delete();
		nodeLayers[i].Clear();

		for (searchesIt = pathSearches[i].begin(); searchesIt != pathSearches[i].end(); ++searchesIt) {
			delete (*searchesIt);
		}

		pathSearches[i].clear();
	}
	for (tracesIt = pathTraces.begin(); tracesIt != pathTraces.end(); ++tracesIt) {
		delete (tracesIt->second);
	}

	nodeTrees.clear();
	nodeLayers.clear();
	pathCaches.clear();
	pathSearches.clear();
	pathTypes.clear();
	pathTraces.clear();

	numCurrExecutedSearches.clear();
	numPrevExecutedSearches.clear();

	PathSearch::FreeGlobalQueue();
}

void QTPFS::PathManager::Load() {
	pmLoadScreen.SetLoading(true);

	// NOTE: offset *must* start at a non-zero value
	searchStateOffset = NODE_STATE_OFFSET;
	numTerrainChanges = 0;
	numPathRequests   = 0;
	maxNumLeafNodes   = 0;

	nodeTrees.resize(moveDefHandler->moveDefs.size(), NULL);
	nodeLayers.resize(moveDefHandler->moveDefs.size());
	pathCaches.resize(moveDefHandler->moveDefs.size());
	pathSearches.resize(moveDefHandler->moveDefs.size());

	// add one extra element for object-less requests
	numCurrExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);
	numPrevExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);

	{
		static const boost::uint32_t mapCheckSum = archiveScanner->GetArchiveCompleteChecksum(gameSetup->mapName);
		static const boost::uint32_t modCheckSum = archiveScanner->GetArchiveCompleteChecksum(gameSetup->modName);

		// NOTE:
		//     should be sufficient in theory, because if either
		//     the map or the mod changes then the checksum does
		//     (should!) as well and we get a cache-miss
		//     this value is also combined with the tree-sums to
		//     make it depend on the tesselation code specifics
		pfsCheckSum = mapCheckSum ^ modCheckSum;

		const std::string& cacheDirName = GetCacheDirName(mapCheckSum, modCheckSum);
		const bool haveCacheDir = FileSystem::DirExists(cacheDirName);

		InitNodeLayersThreaded(MAP_RECTANGLE, haveCacheDir);
		Serialize(cacheDirName);

		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			pfsCheckSum ^= nodeTrees[layerNum]->GetCheckSum();
			maxNumLeafNodes = std::max(nodeLayers[layerNum].GetNumLeafNodes(), maxNumLeafNodes);
		}

		#ifdef SYNCDEBUG
		{ SyncedUint tmp(pfsCheckSum); }
		#endif

		PathSearch::InitGlobalQueue(maxNumLeafNodes);
	}

	{
		const std::string sumStr = "pfs-checksum: " + IntToString(pfsCheckSum, "%08x") + ", ";
		const std::string memStr = "mem-footprint: " + IntToString(GetMemFootPrint()) + "MB";
		pmLoadScreen.AddLoadMessage("[PathManager] " + sumStr + memStr);
		pmLoadScreen.SetLoading(false);
	}
}

boost::uint64_t QTPFS::PathManager::GetMemFootPrint() const {
	boost::uint64_t memFootPrint = sizeof(PathManager);

	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		memFootPrint += nodeLayers[i].GetMemFootPrint();
		memFootPrint += nodeTrees[i]->GetMemFootPrint();
	}

	// convert to megabytes
	return (memFootPrint / (1024 * 1024));
}



void QTPFS::PathManager::SpawnBoostThreads(MemberFunc f, const SRectangle& r, bool b) {
	static std::vector<boost::thread*> threads(std::min(GetNumThreads(), nodeLayers.size()), NULL);

	for (unsigned int threadNum = 0; threadNum < threads.size(); threadNum++) {
		threads[threadNum] = new boost::thread(boost::bind(f, this, threadNum, threads.size(), r, b));
	}

	for (unsigned int threadNum = 0; threadNum < threads.size(); threadNum++) {
		threads[threadNum]->join(); delete threads[threadNum];
	}
}



void QTPFS::PathManager::InitNodeLayersThreaded(const SRectangle& rect, bool haveCacheDir) {
	streflop_init<streflop::Simple>();

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] using %u threads for %u node-layers (cached? %s)";

	#ifdef QTPFS_OPENMP_ENABLED
	{
		#pragma omp parallel
		if (omp_get_thread_num() == 0) {
			// "trust" OpenMP implementation to set a pool-size that
			// matches the CPU threading capacity (eg. the number of
			// active physical cores * number of threads per core);
			// too many and esp. too few threads would be wasteful
			//
			// TODO: OpenMP needs project-global linking changes
			sprintf(loadMsg, fmtString, __FUNCTION__, omp_get_num_threads(), nodeLayers.size(), (haveCacheDir? "true": "false"));
			pmLoadScreen.AddLoadMessage(loadMsg);
		}

		const char* preFmtStr = "  initializing node-layer %u (thread %u)";
		const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";

		#pragma omp parallel for private(loadMsg)
		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			sprintf(loadMsg, preFmtStr, layerNum, omp_get_thread_num());
			pmLoadScreen.AddLoadMessage(loadMsg);

			// construct each tree from scratch IFF no cache-dir exists
			// (if it does, we only need to initialize speed{Mods, Bins})
			InitNodeLayer(layerNum, rect);
			UpdateNodeLayer(layerNum, rect, !haveCacheDir);

			const QTNode* tree = nodeTrees[layerNum];
			const NodeLayer& layer = nodeLayers[layerNum];
			const unsigned int mem = (tree->GetMemFootPrint() + layer.GetMemFootPrint()) / (1024 * 1024);

			sprintf(loadMsg, pstFmtStr, layerNum, mem, layer.GetNumLeafNodes(), layer.GetNodeRatio());
			pmLoadScreen.AddLoadMessage(loadMsg);
		}
	}
	#else
	{
		sprintf(loadMsg, fmtString, __FUNCTION__, GetNumThreads(), nodeLayers.size(), (haveCacheDir? "true": "false"));
		pmLoadScreen.AddLoadMessage(loadMsg);

		SpawnBoostThreads(&PathManager::InitNodeLayersThread, rect, haveCacheDir);
	}
	#endif

	streflop_init<streflop::Simple>();
}

void QTPFS::PathManager::InitNodeLayersThread(
	unsigned int threadNum,
	unsigned int numThreads,
	const SRectangle& rect,
	bool haveCacheDir
) {
	const unsigned int layersPerThread = (nodeLayers.size() / numThreads);
	const unsigned int numExcessLayers = (threadNum == (numThreads - 1))?
		(nodeLayers.size() % numThreads): 0;

	const unsigned int minLayer = threadNum * layersPerThread;
	const unsigned int maxLayer = minLayer + layersPerThread + numExcessLayers;

	char loadMsg[512] = {'\0'};
	const char* preFmtStr = "  initializing node-layer %u (thread %u)";
	const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";

	for (unsigned int layerNum = minLayer; layerNum < maxLayer; layerNum++) {
		sprintf(loadMsg, preFmtStr, layerNum, threadNum);
		pmLoadScreen.AddLoadMessage(loadMsg);

		InitNodeLayer(layerNum, rect);
		UpdateNodeLayer(layerNum, rect, !haveCacheDir);

		const QTNode* tree = nodeTrees[layerNum];
		const NodeLayer& layer = nodeLayers[layerNum];
		const unsigned int mem = (tree->GetMemFootPrint() + layer.GetMemFootPrint()) / (1024 * 1024);

		sprintf(loadMsg, pstFmtStr, layerNum, mem, layer.GetNumLeafNodes(), layer.GetNodeRatio());
		pmLoadScreen.AddLoadMessage(loadMsg);
	}
}

void QTPFS::PathManager::InitNodeLayer(unsigned int layerNum, const SRectangle& rect) {
	nodeTrees[layerNum] = new QTPFS::QTNode(NULL,  0,  rect.x1, rect.z1,  rect.x2, rect.z2);
	nodeLayers[layerNum].Init(layerNum);
	nodeLayers[layerNum].RegisterNode(nodeTrees[layerNum]);
}



void QTPFS::PathManager::UpdateNodeLayersThreaded(const SRectangle& rect) {
	streflop_init<streflop::Simple>();

	#ifdef QTPFS_OPENMP_ENABLED
	{
		#pragma omp parallel for
		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			UpdateNodeLayer(layerNum, rect, true);
		}
	}
	#else
	{
		SpawnBoostThreads(&PathManager::UpdateNodeLayersThread, rect, true);
	}
	#endif

	streflop_init<streflop::Simple>();
}

void QTPFS::PathManager::UpdateNodeLayersThread(
	unsigned int threadNum,
	unsigned int numThreads,
	const SRectangle& rect,
	bool wantTesselation
) {
	const unsigned int layersPerThread = (nodeLayers.size() / numThreads);
	const unsigned int numExcessLayers = (threadNum == (numThreads - 1))?
		(nodeLayers.size() % numThreads): 0;

	const unsigned int minLayer = threadNum * layersPerThread;
	const unsigned int maxLayer = minLayer + layersPerThread + numExcessLayers;

	for (unsigned int layerNum = minLayer; layerNum < maxLayer; layerNum++) {
		UpdateNodeLayer(layerNum, rect, wantTesselation);
	}
}

void QTPFS::PathManager::UpdateNodeLayer(unsigned int layerNum, const SRectangle& r, bool wantTesselation) {
	const MoveDef*  md = moveDefHandler->moveDefs[layerNum];
	const CMoveMath* mm = md->moveMath;

	if (md->unitDefRefCount == 0)
		return;

	// FIXME?
	//     needed for IsBlocked* --> SquareIsBlocked --> IsNonBlocking
	//     no point doing this in ExecuteSearch because the IsBlocked*
	//     calls are only made from here, no point doing it here since
	//     we are independent of a specific path --> requires redesign
	//
	// md->tempOwner = const_cast<CSolidObject*>(path->GetOwner());

	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed
	SRectangle mr = SRectangle(r);
	mr.x1 = std::max(int(r.x1) - (md->xsizeh),        0);
	mr.z1 = std::max(int(r.z1) - (md->zsizeh),        0);
	mr.x2 = std::min(int(r.x2) + (md->xsizeh), gs->mapx);
	mr.z2 = std::min(int(r.z2) + (md->zsizeh), gs->mapy);

	if (nodeLayers[layerNum].Update(mr, md, mm) && wantTesselation) {
		nodeTrees[layerNum]->PreTesselate(nodeLayers[layerNum], mr);
		pathCaches[layerNum].MarkDeadPaths(mr);
	}
}



std::string QTPFS::PathManager::GetCacheDirName(boost::uint32_t mapCheckSum, boost::uint32_t modCheckSum) const {
	static const std::string ver = IntToString(QTPFS_CACHE_VERSION, "%04x");
	static const std::string dir = QTPFS_CACHE_BASEDIR + ver + "/" +
		IntToString(mapCheckSum, "%08x") + "-" +
		IntToString(modCheckSum, "%08x") + "/";

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] using cache-dir %s (map-checksum %08x, mod-checksum %08x)";

	sprintf(loadMsg, fmtString, __FUNCTION__, dir.c_str(), mapCheckSum, modCheckSum);
	pmLoadScreen.AddLoadMessage(loadMsg);

	return dir;
}

void QTPFS::PathManager::Serialize(const std::string& cacheFileDir) {
	std::vector<std::string> fileNames(nodeTrees.size());
	std::vector<std::fstream*> fileStreams(nodeTrees.size());

	if (!FileSystem::DirExists(cacheFileDir)) {
		FileSystem::CreateDirectory(cacheFileDir);
		assert(FileSystem::DirExists(cacheFileDir));
	}

	bool read = false;
	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] serializing node-tree %u (%s)";

	// TODO: compress the tree cache-files?
	for (unsigned int i = 0; i < nodeTrees.size(); i++) {
		fileNames[i] = cacheFileDir + "tree" + IntToString(i, "%02x") + "-" + moveDefHandler->moveDefs[i]->name;
		fileStreams[i] = new std::fstream();

		if (FileSystem::FileExists(fileNames[i])) {
			// read the i-th tree
			read = true;
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::in | std::ios::binary);
			assert(nodeTrees[i]->IsLeaf());
		} else {
			// write the i-th tree
			read = false;
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::out | std::ios::binary);
		}

		sprintf(loadMsg, fmtString, __FUNCTION__, i, moveDefHandler->moveDefs[i]->name.c_str());
		pmLoadScreen.AddLoadMessage(loadMsg);

		serializingNodeLayer = &nodeLayers[i];
		nodeTrees[i]->Serialize(*fileStreams[i], read);
		serializingNodeLayer = NULL;

		fileStreams[i]->flush();
		fileStreams[i]->close();

		delete fileStreams[i];
	}
}



// NOTE:
//     all layers *must* be updated on the same frame
//
//     map-features added during loading do NOT trigger
//     this event (because map and features are already
//     present when PathManager gets instantiated)
//
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2) {
	SCOPED_TIMER("PathManager::TerrainChange");

	UpdateNodeLayersThreaded(SRectangle(x1, z1,  x2, z2));
	numTerrainChanges += 1;
}






void QTPFS::PathManager::Update() {
	SCOPED_TIMER("PathManager::Update");

	// NOTE:
	//     for a mod with N move-types, a unit will be waiting
	//     (N / MAX_UPDATE_DELAY) sim-frames before its request
	//     executes at a minimum
	static const unsigned int numPathTypeUpdates = std::max(1U, static_cast<unsigned int>(nodeLayers.size() / MAX_UPDATE_DELAY));
	static unsigned int minPathTypeUpdate = 0;
	static unsigned int maxPathTypeUpdate = numPathTypeUpdates;

	sharedPaths.clear();

	for (unsigned int pathTypeUpdate = minPathTypeUpdate; pathTypeUpdate < maxPathTypeUpdate; pathTypeUpdate++) {
		QueueDeadPathSearches(pathTypeUpdate);
		ExecuteQueuedSearches(pathTypeUpdate);
	}

	std::copy(numCurrExecutedSearches.begin(), numCurrExecutedSearches.end(), numPrevExecutedSearches.begin());

	minPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates);
	maxPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates);

	if (minPathTypeUpdate >= nodeLayers.size()) {
		minPathTypeUpdate = 0;
		maxPathTypeUpdate = numPathTypeUpdates;
		return;
	}
	if (maxPathTypeUpdate >= nodeLayers.size()) {
		maxPathTypeUpdate = nodeLayers.size();
	}
}



void QTPFS::PathManager::ExecuteQueuedSearches(unsigned int pathType) {
	NodeLayer& nodeLayer = nodeLayers[pathType];
	PathCache& pathCache = pathCaches[pathType];

	std::list<IPathSearch*>& searches = pathSearches[pathType];
	std::list<IPathSearch*>::iterator searchesIt = searches.begin();

	if (!searches.empty()) {
		// execute pending searches collected via
		// RequestPath and QueueDeadPathSearches
		while (searchesIt != searches.end()) {
			ExecuteSearch(searches, searchesIt, nodeLayer, pathCache, pathType);
		}
	}
}

void QTPFS::PathManager::ExecuteSearch(
	PathSearchList& searches,
	PathSearchListIt& searchesIt,
	NodeLayer& nodeLayer,
	PathCache& pathCache,
	unsigned int pathType
) {
	IPathSearch* search = *searchesIt;
	IPath* path = pathCache.GetTempPath(search->GetID());

	assert(search != NULL);
	assert(path != NULL);

	// temp-path might have been removed already via
	// DeletePath before we got a chance to process it
	if (path->GetID() == 0) {
		*searchesIt = NULL;
		searchesIt = searches.erase(searchesIt);
		delete search;
		return;
	}

	assert(search->GetID() != 0);
	assert(path->GetID() == search->GetID());

	search->Initialize(&nodeLayer, &pathCache, path->GetSourcePoint(), path->GetTargetPoint(), MAP_RECTANGLE);
	path->SetHash(search->GetHash(gs->mapx * gs->mapy, pathType));

	{
		#ifdef QTPFS_SEARCH_SHARED_PATHS
		SharedPathMap::const_iterator sharedPathsIt = sharedPaths.find(path->GetHash());

		if (sharedPathsIt != sharedPaths.end()) {
			search->SharedFinalize(sharedPathsIt->second, path);
			*searchesIt = NULL;
			searchesIt = searches.erase(searchesIt);
			delete search;
			return;
		}
		#endif

		#ifdef QTPFS_LIMIT_TEAM_SEARCHES
		const unsigned int numCurrSearches = numCurrExecutedSearches[search->GetTeam()];
		const unsigned int numPrevSearches = numPrevExecutedSearches[search->GetTeam()];

		if ((numCurrSearches - numPrevSearches) >= MAX_TEAM_SEARCHES) {
			++searchesIt; return;
		}

		numCurrExecutedSearches[search->GetTeam()] += 1;
		#endif
	}

	// removes path from temp-paths, adds it to live-paths
	if (search->Execute(searchStateOffset, numTerrainChanges)) {
		search->Finalize(path);

		#ifdef QTPFS_SEARCH_SHARED_PATHS
		sharedPaths[path->GetHash()] = path;
		#endif

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		pathTraces[path->GetID()] = search->GetExecutionTrace();
		#endif
	} else {
		DeletePath(path->GetID());
	}

	*searchesIt = NULL;
	searchesIt = searches.erase(searchesIt);
	delete search;

	searchStateOffset += NODE_STATE_OFFSET;
}

void QTPFS::PathManager::QueueDeadPathSearches(unsigned int pathType) {
	#ifndef IGNORE_DEAD_PATHS
	PathCache& pathCache = pathCaches[pathType];
	PathCache::PathMap::const_iterator deadPathsIt;

	const PathCache::PathMap& deadPaths = pathCache.GetDeadPaths();
	const MoveDef* moveDef = moveDefHandler->moveDefs[pathType];

	if (!deadPaths.empty()) {
		// re-request LIVE paths that were marked as DEAD by TerrainChange
		// for each of these now-dead paths, reset the active point-ID to 0
		for (deadPathsIt = deadPaths.begin(); deadPathsIt != deadPaths.end(); ++deadPathsIt) {
			QueueSearch(deadPathsIt->second, NULL, moveDef, ZeroVector, ZeroVector, -1.0f, false);
		}

		pathCache.KillDeadPaths();
	}
	#endif
}

unsigned int QTPFS::PathManager::QueueSearch(
	const IPath* oldPath,
	const CSolidObject* object,
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced
) {
	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	// NOTE:
	//     the path-owner object handed to us can never become
	//     dangling (even with delayed execution) because ~GMT
	//     calls DeletePath, which ensures any path is removed
	//     from its cache before we get to ExecuteSearch
	IPath* newPath = new IPath();
	IPathSearch* newSearch = new PathSearch(PATH_SEARCH_ASTAR);

	assert(newPath != NULL);
	assert(newSearch != NULL);

	if (oldPath != NULL) {
		assert(oldPath->GetID() != 0);
		// argument values are unused in this case
		assert(object == NULL);
		assert(sourcePoint == ZeroVector);
		assert(targetPoint == ZeroVector);
		assert(radius == -1.0f);
		assert(!synced);

		const CSolidObject* obj = oldPath->GetOwner();
		const float3& pos = (obj != NULL)? obj->pos: oldPath->GetSourcePoint();

		newPath->SetID(oldPath->GetID());
		newPath->SetPointID(0);
		newPath->SetRadius(oldPath->GetRadius());
		newPath->SetSynced(oldPath->GetSynced());

		// start re-request from the current point
		// along the path, not the original source
		// (oldPath->GetSourcePoint())
		newPath->AllocPoints(2);
		newPath->SetOwner(oldPath->GetOwner());
		newPath->SetSourcePoint(pos);
		newPath->SetTargetPoint(oldPath->GetTargetPoint());
		newSearch->SetID(oldPath->GetID());
		newSearch->SetTeam(teamHandler->ActiveTeams());
	} else {
		// NOTE:
		//     the unclamped end-points are temporary
		//     zero is a reserved ID, so pre-increment
		newPath->SetID(++numPathRequests);
		newPath->SetRadius(radius);
		newPath->SetSynced(synced);
		newPath->AllocPoints(2);
		newPath->SetOwner(object);
		newPath->SetSourcePoint(sourcePoint);
		newPath->SetTargetPoint(targetPoint);
		newSearch->SetID(newPath->GetID());
		newSearch->SetTeam((object != NULL)? object->team: teamHandler->ActiveTeams());
	}

	assert((pathCaches[moveDef->pathType].GetTempPath(newPath->GetID()))->GetID() == 0);

	// TODO:
	//     introduce synced and unsynced path-caches;
	//     somehow support extra-cost overlays again
	//
	// map the path-ID to the index of the cache that stores it
	pathTypes[newPath->GetID()] = moveDef->pathType;
	pathSearches[moveDef->pathType].push_back(newSearch);
	pathCaches[moveDef->pathType].AddTempPath(newPath);

	return (newPath->GetID());
}



void QTPFS::PathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID) {
	const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);

	if (pathTypeIt != pathTypes.end()) {
		PathCache& pathCache = pathCaches[pathTypeIt->second];
		IPath* livePath = pathCache.GetLivePath(pathID);

		if (livePath->GetID() != 0) {
			assert(owner == livePath->GetOwner());
		}
	}
}

void QTPFS::PathManager::DeletePath(unsigned int pathID) {
	const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);
	const PathTraceMapIt pathTraceIt = pathTraces.find(pathID);

	if (pathTypeIt != pathTypes.end()) {
		PathCache& pathCache = pathCaches[pathTypeIt->second];
		pathCache.DelPath(pathID);

		pathTypes.erase(pathTypeIt);
	}

	if (pathTraceIt != pathTraces.end()) {
		delete (pathTraceIt->second);
		pathTraces.erase(pathTraceIt);
	}
}

unsigned int QTPFS::PathManager::RequestPath(
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	float radius,
	CSolidObject* object,
	bool synced)
{
	SCOPED_TIMER("PathManager::RequestPath");
	return (QueueSearch(NULL, object, moveDef, sourcePoint, targetPoint, radius, synced));
}



float3 QTPFS::PathManager::NextWayPoint(
	unsigned int pathID,
	float3 point,
	float radius,
	int, // numRetries
	int, // ownerID
	bool // synced
) {
	SCOPED_TIMER("PathManager::NextWayPoint");

	const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);
	const float3 noPathPoint = float3(-1.0f, 0.0f, -1.0f);

	// dangling ID after a re-request failure or regular deletion
	// return an error-vector so GMT knows it should stop the unit
	if (pathTypeIt == pathTypes.end())
		return noPathPoint;

	IPath* tempPath = pathCaches[pathTypeIt->second].GetTempPath(pathID);
	IPath* livePath = pathCaches[pathTypeIt->second].GetLivePath(pathID);

	if (tempPath->GetID() != 0) {
		// path-request has not yet been processed (so ID still maps to
		// a temporary path); just set the unit off toward its target to
		// hide latency
		//
		// <curPoint> is initially the position of the unit requesting a
		// path, but later changes to the subsequent values returned here
		//
		// NOTE:
		//     if the returned point P is too far away, then a unit U will
		//     never switch to its live-path even after it becomes available
		//     (because NextWayPoint is not called again until U gets close
		//     to P), so always keep it a fixed small distance in front
		//
		//     make the y-coordinate -1 to indicate these are temporary
		//     waypoints to GMT and should not be followed religiously
		const float3& sourcePoint = point;
		const float3& targetPoint = tempPath->GetTargetPoint();
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize() * SQUARE_SIZE;
		return float3(sourcePoint.x + targetDirec.x, -1.0f, sourcePoint.z + targetDirec.z);
	}
	if (livePath->GetID() == 0) {
		// the request WAS processed but then immediately undone by a
		// TerrainChange --> MarkDeadPaths event in the same frame as
		// NextWayPoint (so pathID is only in deadPaths)
		return point;
	}

	float minRadiusSq = QTPFS_POSITIVE_INFINITY;

	unsigned int minPointIdx = 0;
	unsigned int nxtPointIdx = 1;

	// find the next waypoint (ie. the node that is
	// furthest along the path *and* within distance
	// <radius> of <point>), as well as the waypoint
	// that is closest to <point>
	//
	for (unsigned int i = (livePath->GetPointID() * 1); i < (livePath->NumPoints() - 1); i++) {
		const float radiusSq = (point - livePath->GetPoint(i)).SqLength2D();

		// find waypoints <p0> and <p1> such that <point> is
		// "in front" of p0 and "behind" p1 (ie. in between)
		//
		// we do this rather than the radius-based search
		// since depending on the value of <radius> we may
		// or may not find a "next" node (even though one
		// always exists)
		const float3& p0 = livePath->GetPoint(i    ), v0 = float3(p0.x - point.x, 0.0f, p0.z - point.z);
		const float3& p1 = livePath->GetPoint(i + 1), v1 = float3(p1.x - point.x, 0.0f, p1.z - point.z);

		// NOTE:
		//     either v0 or v1 can be a zero-vector (p0 == point or p1 == point)
		//     in those two cases the dot-product is meaningless so we skip them
		//     vectors are NOT normalized, so it can happen that NO case matches
		//     and we must fall back to the radius-based closest point
		if (v0.SqLength() < 0.1f) { nxtPointIdx = i + 1; break; }
		if (v1.SqLength() < 0.1f) { nxtPointIdx = i + 2; break; }
		if (v0.dot(v1) <= -0.01f) { nxtPointIdx = i + 1;        }

		if (radiusSq < minRadiusSq) {
			minRadiusSq = radiusSq;
			minPointIdx = i + 0;
		}
	}

	// handle a corner-case in which a unit is at the start of its path
	// and the goal is in front of it, but on the other side of a cliff
	if ((livePath->GetPointID() == 0) && (nxtPointIdx == (livePath->NumPoints() - 1)))
		nxtPointIdx = 1;

	if (minPointIdx < nxtPointIdx) {
		// if close enough to at least one waypoint <i>,
		// switch to the point immediately following it
		livePath->SetPointID(nxtPointIdx);
	} else {
		// otherwise just pick the closest point
		livePath->SetPointID(minPointIdx);
	}

	return (livePath->GetPoint(livePath->GetPointID()));
}



void QTPFS::PathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);

	if (pathTypeIt == pathTypes.end())
		return;

	const PathCache& cache = pathCaches[pathTypeIt->second];
	const IPath* path = cache.GetLivePath(pathID);

	if (path->GetID() == 0)
		return;

	// maintain compatibility with the tri-layer legacy PFS
	points.resize(path->NumPoints());
	starts.resize(3, 0);

	for (unsigned int n = 0; n < path->NumPoints(); n++) {
		points[n] = path->GetPoint(n);
	}
}

