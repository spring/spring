/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/bind.hpp>
#include <boost/thread.hpp>
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

	nodeTrees.resize(moveinfo->moveData.size(), NULL);
	nodeLayers.resize(moveinfo->moveData.size());
	pathCaches.resize(moveinfo->moveData.size());
	pathSearches.resize(moveinfo->moveData.size());

	// add one extra element for object-less requests
	numCurrExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);
	numPrevExecutedSearches.resize(teamHandler->ActiveTeams() + 1, 0);

	const std::string& cacheDirName = GetCacheDirName(gameSetup->mapName, gameSetup->modName);
	const bool haveCacheDir = FileSystem::DirExists(cacheDirName);

	static const SRectangle mapRect = SRectangle(0, 0,  gs->mapx, gs->mapy);

	{
		streflop_init<streflop::Simple>();

		char loadMsg[512] = {'\0'};
		const char* fmtString = "[%s] using %u threads for %u node-layers (cached? %s)";

		#ifdef OPENMP
			// never use more threads than the number of layers
			// TODO: this needs project-global linking changes
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
				nodeLayers[i].Init(i);
				nodeLayers[i].RegisterNode(nodeTrees[i]);

				// construct each tree from scratch IFF no cache-dir exists
				// (if it does, we only need to initialize speed{Mods, Bins})
				UpdateNodeLayer(i, mapRect, true, !haveCacheDir);

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
		nodeLayers[i].Init(i);
		nodeLayers[i].RegisterNode(nodeTrees[i]);

		UpdateNodeLayer(i, mapRect, true, !haveCacheDir);

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
	std::vector<std::string> fileNames(nodeTrees.size());
	std::vector<std::fstream*> fileStreams(nodeTrees.size());

	if (!FileSystem::DirExists(cacheFileDir)) {
		FileSystem::CreateDirectory(cacheFileDir);
		assert(FileSystem::DirExists(cacheFileDir));
	}

	bool read = false;
	char loadMsg[512] = {'\0'};
	const char* fmtString = "[%s] serializing node-tree %u";

	// TODO: calculate checksum over each tree
	// NOTE: also compress the tree cache-files?
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

		sprintf(loadMsg, fmtString, __FUNCTION__, i);
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
//     gets called during initialization for trees etc., but
//     never with a map-covering rectangle (constructor does
//     this for us)
//
//     all layers *must* be updated on the same frame
//
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2) {
	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed
	x1 = std::max(int(x1) - 1,        0);
	z1 = std::max(int(z1) - 1,        0);
	x2 = std::min(int(x2) + 1, gs->mapx);
	z2 = std::min(int(z2) + 1, gs->mapy);

	const SRectangle r = SRectangle(x1, z1,  x2, z2);

	{
		SCOPED_TIMER("PathManager::TerrainChange");
		streflop_init<streflop::Simple>();

		#pragma omp parallel for
		for (unsigned int i = 0; i < nodeLayers.size(); i++) {
			UpdateNodeLayer(i, r, false, true);
		}

		streflop_init<streflop::Simple>();
	}

	numTerrainChanges += 1;
}

void QTPFS::PathManager::UpdateNodeLayer(unsigned int i, const SRectangle& r, bool isInitializing, bool wantTesselation) {
	const MoveData*  md = moveinfo->moveData[i];
	const CMoveMath* mm = md->moveMath;

	if (md->unitDefRefCount == 0)
		return;

	if (nodeLayers[i].Update(r, md, mm) && wantTesselation) {
		nodeTrees[i]->PreTesselate(nodeLayers[i], r, isInitializing);
		pathCaches[i].MarkDeadPaths(r);
	}
}






void QTPFS::PathManager::Update() {
	SCOPED_TIMER("PathManager::Update");

	// NOTE:
	//     for a mod with N move-types, a unit will be waiting
	//     <numUpdates> sim-frames before its request executes
	//     (at a minimum)
	static const unsigned int numUpdates =
		std::max(1U, static_cast<unsigned int>(nodeLayers.size() / MAX_UPDATE_DELAY));

	static unsigned int minUpdate = 0;
	static unsigned int maxUpdate = numUpdates;

	for (unsigned int i = minUpdate; i < maxUpdate; i++) {
		NodeLayer& nodeLayer = nodeLayers[i];
		PathCache& pathCache = pathCaches[i];
		PathCache::PathIDSet::const_iterator deadPathsIt;

		const MoveData* moveData = moveinfo->moveData[i];
		const PathCache::PathIDSet& deadPaths = pathCache.GetDeadPaths();

		assert(i == moveData->pathType);

		std::list<IPathSearch*>& searches = pathSearches[i];
		std::list<IPathSearch*>::iterator searchesIt = searches.begin();

		if (!searches.empty()) {
			#ifdef QTPFS_SEARCH_SHARED_PATHS
			std::map<boost::uint64_t, IPath*> sharedPaths;
			#endif

			// execute all pending searches collected via RequestPath
			while (searchesIt != searches.end()) {
				IPathSearch* search = *searchesIt;
				IPath* path = pathCache.GetTempPath(search->GetID());

				assert(search != NULL);
				assert(path != NULL);

				search->Initialize(&nodeLayer, &pathCache, path->GetSourcePoint(), path->GetTargetPoint());

				// temp-path might have been removed already via DeletePath
				if (path->GetID() == 0) {
					*searchesIt = NULL;
					searchesIt = searches.erase(searchesIt);
					delete search;
					continue;
				}


				#ifdef QTPFS_SEARCH_SHARED_PATHS
				const boost::uint32_t N = nodeLayer.GetNumLeafNodes();
				const boost::uint64_t K = search->GetHash(N, i);
				const std::map<boost::uint64_t, IPath*>::const_iterator sharedPathsIt = sharedPaths.find(K);

				if (sharedPathsIt != sharedPaths.end()) {
					search->SharedFinalize(sharedPathsIt->second, path);
					*searchesIt = NULL;
					searchesIt = searches.erase(searchesIt);
					delete search;
					continue;
				}
				#endif


				assert(search->GetID() != 0);
				assert(path->GetID() == search->GetID());

				const unsigned int numCurrSearches = numCurrExecutedSearches[search->GetTeam()];
				const unsigned int numPrevSearches = numPrevExecutedSearches[search->GetTeam()];

				if ((numCurrSearches - numPrevSearches) >= MAX_TEAM_SEARCHES) {
					++searchesIt; continue;
				}

				numCurrExecutedSearches[search->GetTeam()] += 1;

				#ifdef QTPFS_TRACE_PATH_SEARCHES
				PathSearchTrace::Execution* searchExec = new PathSearchTrace::Execution(gs->frameNum);
				pathTraces[path->GetID()] = searchExec;
				#else
				PathSearchTrace::Execution* searchExec = NULL;
				pathTraces[path->GetID()] = searchExec;
				#endif

				// removes path from temp-paths, adds it to live-paths
				if (search->Execute(searchExec, searchStateOffset, numTerrainChanges)) {
					search->Finalize(path, false);

					#ifdef QTPFS_SEARCH_SHARED_PATHS
					sharedPaths[K] = path;
					#endif
				}

				*searchesIt = NULL;
				searchesIt = searches.erase(searchesIt);
				delete search;

				searchStateOffset += NODE_STATE_OFFSET;
			}
		}

		#ifndef IGNORE_DEAD_PATHS
		if (!deadPaths.empty()) {
			std::list<unsigned int> replacedPathIDs;

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

				// start re-request from the current point
				// along the path, not the original source
				//
				// NOTE: we do not really need this search
				// to have an ID (since it is executed for
				// a live-path)
				newSearch->SetID(oldPath->GetID());
				newSearch->Initialize(&nodeLayer, &pathCache, oldPath->GetPoint(oldPath->GetPointIdx() - 1), oldPath->GetTargetPoint());

				if (newSearch->Execute(NULL, searchStateOffset, numTerrainChanges)) {
					newSearch->Finalize(newPath, true);

					// path was succesfully re-requested, remove
					// it from the DEAD paths before killing them
					replacedPathIDs.push_back(oldPath->GetID());
				} else {
					// failed to re-request, so path is still dead
					// and will be cleaned up next by KillDeadPaths
					pathTypes.erase(oldPath->GetID());
				}

				delete newSearch;
				searchStateOffset += NODE_STATE_OFFSET;
			}

			// NOTE:
			//     any path that was not succesfully re-requested
			//     will now have a "dangling" ID in GroundMoveType
			//     --> all subsequent NextWaypoint (and DeletePath)
			//     calls will be no-ops
			pathCache.KillDeadPaths(replacedPathIDs);
		}
		#endif
	}

	minUpdate = (minUpdate + numUpdates) % nodeLayers.size();
	maxUpdate = (minUpdate + numUpdates);

	std::copy(numCurrExecutedSearches.begin(), numCurrExecutedSearches.end(), numPrevExecutedSearches.begin());
}

float3 QTPFS::PathManager::NextWayPoint(
	unsigned int pathID,
	float3 curPoint,
	float radius,
	int, // numRetries
	int, // ownerID
	bool synced
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
	for (unsigned int i = 0; i < (livePath->NumPoints() - 1); i++) {
		const float3& point = livePath->GetPoint(i);

		if ((curPoint - point).SqLength() < radiusSq) {
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

	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	IPath* path = new Path();
	IPathSearch* pathSearch = new PathSearch(PATH_SEARCH_ASTAR);

	assert(path != NULL);
	assert(pathSearch != NULL);

	// NOTE:
	//     the unclamped end-points are temporary
	//     zero is a reserved ID, so pre-increment
	path->SetID(++numPathRequests);
	path->SetOwnerID((object != NULL)? object->id: -1U);
	path->SetRadius(radius);
	path->SetSynced(synced);
	path->AllocPoints(2);
	path->SetSourcePoint(sourcePoint);
	path->SetTargetPoint(targetPoint);
	pathSearch->SetID(path->GetID());
	pathSearch->SetTeam((object != NULL)? object->team: teamHandler->ActiveTeams());

	// TODO:
	//     introduce synced and unsynced path-caches;
	//     somehow support extra-cost overlays again
	pathSearches[moveData->pathType].push_back(pathSearch);
	pathCaches[moveData->pathType].AddTempPath(path);

	// map the path-ID to the index of the cache that stores it
	pathTypes[path->GetID()] = moveData->pathType;
	return (path->GetID());
}

