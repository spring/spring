/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <omp.h>

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

#ifdef QTPFS_NO_LOADSCREEN
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
	maxNumLayerNodes  = 0;

	nodeTrees.resize(moveinfo->moveData.size(), NULL);
	nodeLayers.resize(moveinfo->moveData.size());
	pathCaches.resize(moveinfo->moveData.size());
	pathSearches.resize(moveinfo->moveData.size());

	// add one extra element for object-less requests
	numCurrExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);
	numPrevExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);

	{
		const std::string& cacheDirName = GetCacheDirName(gameSetup->mapName, gameSetup->modName);
		const bool haveCacheDir = FileSystem::DirExists(cacheDirName);

		InitNodeLayersThreaded(SRectangle(0, 0,  gs->mapx, gs->mapy), haveCacheDir);
		Serialize(cacheDirName);

		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			maxNumLayerNodes = std::max(nodeLayers[layerNum].GetNumLeafNodes(), maxNumLayerNodes);
		}

		PathSearch::InitGlobalQueue(maxNumLayerNodes);
	}

	loadscreen->SetLoadMessage("[PathManager] memory-footprint: " + IntToString(GetMemFootPrint()) + "MB");
}

QTPFS::PathManager::~PathManager() {
	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		nodeTrees[i]->Delete();
		nodeLayers[i].Clear();
		pathSearches[i].clear(); // TODO: delete values in pathSearches[i]
	}

	nodeTrees.clear();
	nodeLayers.clear();
	pathCaches.clear();
	pathSearches.clear();
	pathTypes.clear();
	pathTraces.clear(); // TODO: delete values

	numCurrExecutedSearches.clear();
	numPrevExecutedSearches.clear();

	PathSearch::FreeGlobalQueue();
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
			loadscreen->SetLoadMessage(loadMsg);
		}

		const char* preFmtStr = "  initializing node-layer %u (thread %u)";
		const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";

		#pragma omp parallel for private(loadMsg)
		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			sprintf(loadMsg, preFmtStr, layerNum, omp_get_thread_num());
			loadscreen->SetLoadMessage(loadMsg);

			// construct each tree from scratch IFF no cache-dir exists
			// (if it does, we only need to initialize speed{Mods, Bins})
			InitNodeLayer(layerNum, rect);
			UpdateNodeLayer(layerNum, rect, !haveCacheDir);

			const QTNode* tree = nodeTrees[layerNum];
			const NodeLayer& layer = nodeLayers[layerNum];
			const unsigned int mem = (tree->GetMemFootPrint() + layer.GetMemFootPrint()) / (1024 * 1024);

			sprintf(loadMsg, pstFmtStr, layerNum, mem, layer.GetNumLeafNodes(), layer.GetNodeRatio());
			loadscreen->SetLoadMessage(loadMsg);
		}
	}
	#else
	{
		sprintf(loadMsg, fmtString, __FUNCTION__, GetNumThreads(), nodeLayers.size(), (haveCacheDir? "true": "false"));
		loadscreen->SetLoadMessage(loadMsg);

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
		loadscreen->SetLoadMessage(loadMsg);

		InitNodeLayer(layerNum, rect);
		UpdateNodeLayer(layerNum, rect, !haveCacheDir);

		const QTNode* tree = nodeTrees[layerNum];
		const NodeLayer& layer = nodeLayers[layerNum];
		const unsigned int mem = (tree->GetMemFootPrint() + layer.GetMemFootPrint()) / (1024 * 1024);

		sprintf(loadMsg, pstFmtStr, layerNum, mem, layer.GetNumLeafNodes(), layer.GetNodeRatio());
		loadscreen->SetLoadMessage(loadMsg);
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
	const MoveData*  md = moveinfo->moveData[layerNum];
	const CMoveMath* mm = md->moveMath;

	if (md->unitDefRefCount == 0)
		return;

	if (nodeLayers[layerNum].Update(r, md, mm) && wantTesselation) {
		nodeTrees[layerNum]->PreTesselate(nodeLayers[layerNum], r);
		pathCaches[layerNum].MarkDeadPaths(r);
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
	const char* fmtString = "[PathManager::%s] using cache-dir %s (map-checksum %08x, mod-checksum %08x)";

	sprintf(loadMsg, fmtString, __FUNCTION__, dir.c_str(), mapCheckSum, modCheckSum);
	loadscreen->SetLoadMessage(loadMsg);
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

	// TODO:
	//     calculate checksum over each tree
	//     also compress the tree cache-files?
	for (unsigned int i = 0; i < nodeTrees.size(); i++) {
		fileNames[i] = cacheFileDir + "tree" + IntToString(i, "%02x") + "-" + moveinfo->moveData[i]->name;
		fileStreams[i] = new std::fstream();

		if (FileSystem::FileExists(fileNames[i])) {
			// read the i-th tree
			read = true;
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::in | std::ios::binary);
		} else {
			// write the i-th tree
			read = false;
			fileStreams[i]->open(fileNames[i].c_str(), std::ios::out | std::ios::binary);
		}

		sprintf(loadMsg, fmtString, __FUNCTION__, i, moveinfo->moveData[i]->name.c_str());
		loadscreen->SetLoadMessage(loadMsg);

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

	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed
	x1 = std::max(int(x1) - 1,        0);
	z1 = std::max(int(z1) - 1,        0);
	x2 = std::min(int(x2) + 1, gs->mapx);
	z2 = std::min(int(z2) + 1, gs->mapy);

	UpdateNodeLayersThreaded(SRectangle(x1, z1,  x2, z2));
	numTerrainChanges += 1;
}






void QTPFS::PathManager::Update() {
	SCOPED_TIMER("PathManager::Update");

	// NOTE:
	//     for a mod with N move-types, a unit will be waiting
	//     <numUpdates> sim-frames before its request executes
	//     (at a minimum)
	static const unsigned int numPathTypeUpdates =
		std::max(1U, static_cast<unsigned int>(nodeLayers.size() / MAX_UPDATE_DELAY));

	static unsigned int minPathTypeUpdate = 0;
	static unsigned int maxPathTypeUpdate = numPathTypeUpdates;

	sharedPaths.clear();

	for (unsigned int pathTypeUpdate = minPathTypeUpdate; pathTypeUpdate < maxPathTypeUpdate; pathTypeUpdate++) {
		QueueDeadPathSearches(pathTypeUpdate);
		ExecuteQueuedSearches(pathTypeUpdate);
	}

	minPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates) % nodeLayers.size();
	maxPathTypeUpdate = (minPathTypeUpdate + numPathTypeUpdates);

	std::copy(numCurrExecutedSearches.begin(), numCurrExecutedSearches.end(), numPrevExecutedSearches.begin());
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

	search->Initialize(&nodeLayer, &pathCache, path->GetSourcePoint(), path->GetTargetPoint());
	path->SetHash(search->GetHash(gs->mapx * gs->mapy, pathType));

	#ifdef QTPFS_SEARCH_SHARED_PATHS
	{
		SharedPathMap::const_iterator sharedPathsIt = sharedPaths.find(path->GetHash());

		if (sharedPathsIt != sharedPaths.end()) {
			search->SharedFinalize(sharedPathsIt->second, path);
			*searchesIt = NULL;
			searchesIt = searches.erase(searchesIt);
			delete search;
			return;
		}
	}
	#endif

	{
		const unsigned int numCurrSearches = numCurrExecutedSearches[search->GetTeam()];
		const unsigned int numPrevSearches = numPrevExecutedSearches[search->GetTeam()];

		if ((numCurrSearches - numPrevSearches) >= MAX_TEAM_SEARCHES) {
			++searchesIt; return;
		}

		numCurrExecutedSearches[search->GetTeam()] += 1;
	}

	#ifdef QTPFS_TRACE_PATH_SEARCHES
	PathSearchTrace::Execution* searchExec = new PathSearchTrace::Execution(gs->frameNum);
	#else
	PathSearchTrace::Execution* searchExec = NULL;
	#endif

	// removes path from temp-paths, adds it to live-paths
	if (search->Execute(searchExec, searchStateOffset, numTerrainChanges)) {
		search->Finalize(path);

		#ifdef QTPFS_SEARCH_SHARED_PATHS
		sharedPaths[path->GetHash()] = path;
		#endif

		pathTraces[path->GetID()] = searchExec;
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
	NodeLayer& nodeLayer = nodeLayers[pathType];
	PathCache& pathCache = pathCaches[pathType];
	PathCache::PathMap::const_iterator deadPathsIt;

	const PathCache::PathMap& deadPaths = pathCache.GetDeadPaths();
	const MoveData* moveData = moveinfo->moveData[pathType];

	if (!deadPaths.empty()) {
		// re-request LIVE paths that were marked as DEAD by TerrainChange
		for (deadPathsIt = deadPaths.begin(); deadPathsIt != deadPaths.end(); ++deadPathsIt) {
			QueueSearch(deadPathsIt->second, NULL, moveData, ZeroVector, ZeroVector, -1.0f, false);
		}

		pathCache.KillDeadPaths();
	}
	#endif
}

unsigned int QTPFS::PathManager::QueueSearch(
	const IPath* oldPath,
	const CSolidObject* object,
	const MoveData* moveData,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced
) {
	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	IPath* newPath = new Path();
	IPathSearch* newSearch = new PathSearch(PATH_SEARCH_ASTAR);

	assert(newPath != NULL);
	assert(newSearch != NULL);

	if (oldPath != NULL) {
		assert(oldPath->GetID() != 0);

		newPath->SetID(oldPath->GetID());
		newPath->SetOwnerID(oldPath->GetOwnerID());
		newPath->SetRadius(oldPath->GetRadius());
		newPath->SetSynced(oldPath->GetSynced());

		// start re-request from the current point
		// along the path, not the original source
		// FIXME: this point can now have +inf cost
		newPath->AllocPoints(2);
		// newPath->SetSourcePoint(oldPath->GetPoint(oldPath->GetPointIdx() - 1));
		newPath->SetSourcePoint(oldPath->GetSourcePoint());
		newPath->SetTargetPoint(oldPath->GetTargetPoint());
		newSearch->SetID(oldPath->GetID());
		newSearch->SetTeam(teamHandler->ActiveTeams());
	} else {
		// NOTE:
		//     the unclamped end-points are temporary
		//     zero is a reserved ID, so pre-increment
		newPath->SetID(++numPathRequests);
		newPath->SetOwnerID((object != NULL)? object->id: -1U);
		newPath->SetRadius(radius);
		newPath->SetSynced(synced);
		newPath->AllocPoints(2);
		newPath->SetSourcePoint(sourcePoint);
		newPath->SetTargetPoint(targetPoint);
		newSearch->SetID(newPath->GetID());
		newSearch->SetTeam((object != NULL)? object->team: teamHandler->ActiveTeams());
	}

	// TODO:
	//     introduce synced and unsynced path-caches;
	//     somehow support extra-cost overlays again
	//
	// map the path-ID to the index of the cache that stores it
	pathTypes[newPath->GetID()] = moveData->pathType;
	pathSearches[moveData->pathType].push_back(newSearch);
	pathCaches[moveData->pathType].AddTempPath(newPath);

	return (newPath->GetID());
}



float3 QTPFS::PathManager::NextWayPoint(
	unsigned int pathID,
	float3 curPoint,
	float radius,
	int, // numRetries
	int, // ownerID
	bool // synced
) {
	SCOPED_TIMER("PathManager::NextWayPoint");

	const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);

	// dangling ID after re-request failure or regular deletion
	if (pathTypeIt == pathTypes.end())
		return curPoint;

	IPath* tempPath = pathCaches[pathTypeIt->second].GetTempPath(pathID);
	IPath* livePath = pathCaches[pathTypeIt->second].GetLivePath(pathID);

	if (tempPath->GetID() != 0) {
		// path-request has not yet been processed (so ID still maps to
		// a temporary path); just set the unit off toward its target
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
		//     if the queued search fails, the next call to us should make
		//     the unit stop
		assert(livePath->GetID() == 0);

		const float3& sourcePoint = curPoint;
		const float3& targetPoint = tempPath->GetTargetPoint();
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize();
		return (sourcePoint + targetDirec * SQUARE_SIZE);
	} else {
		assert(livePath->GetID() != 0);
	}

	const float radiusSq = std::max(float(SQUARE_SIZE * SQUARE_SIZE), radius * radius);

	unsigned int curPointIdx =  0;
	unsigned int nxtPointIdx = -1U;

	// find the point furthest along the
	// path within distance <rad> of <pos>
	for (unsigned int i = livePath->GetPointIdx(); i < (livePath->NumPoints() - 1); i++) {
		if ((curPoint - livePath->GetPoint(i)).SqLength() < radiusSq) {
			curPointIdx = i;
			nxtPointIdx = i + 1;
		}
	}

	if (nxtPointIdx != -1U) {
		// if close enough to at least one <curPoint>,
		// switch to <nxtPoint> immediately after it
		livePath->SetPointIdx(nxtPointIdx);
	}

	return (livePath->GetPoint(livePath->GetPointIdx()));
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
		pathTraces.erase(pathTraceIt);
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
	SCOPED_TIMER("PathManager::RequestPath");
	return (QueueSearch(NULL, object, moveData, sourcePoint, targetPoint, radius, synced));
}

