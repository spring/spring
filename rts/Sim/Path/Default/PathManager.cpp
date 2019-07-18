/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathManager.h"
#include "PathConstants.h"
#include "PathFinder.h"
#include "PathEstimator.h"
#include "PathFlowMap.hpp"
#include "PathHeatMap.hpp"
#include "PathLog.h"
#include "PathMemPool.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Objects/SolidObject.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"


static CPathFinder    gMaxResPF;
static CPathEstimator gMedResPE;
static CPathEstimator gLowResPE;


CPathManager::CPathManager()
: maxResPF(nullptr)
, medResPE(nullptr)
, lowResPE(nullptr)
, pathFlowMap(nullptr)
, pathHeatMap(nullptr)
, nextPathID(0)
{
	IPathFinder::InitStatic();
	CPathFinder::InitStatic();

	pathFlowMap = PathFlowMap::GetInstance();
	pathHeatMap = PathHeatMap::GetInstance();

	pathMap.reserve(1024);

	// PathNode::nodePos is an ushort2, PathNode::nodeNum is an int
	// therefore the maximum map size is limited to 64k*64k squares
	assert(mapDims.mapx <= 0xFFFFU && mapDims.mapy <= 0xFFFFU);
}

CPathManager::~CPathManager()
{
	// Finalize is not called in case of forced exit
	if (maxResPF != nullptr) {
		lowResPE->Kill();
		medResPE->Kill();
		maxResPF->Kill();

		maxResPF = nullptr;
		medResPE = nullptr;
		lowResPE = nullptr;
	}

	PathHeatMap::FreeInstance(pathHeatMap);
	PathFlowMap::FreeInstance(pathFlowMap);
	IPathFinder::KillStatic();
}


void CPathManager::RemoveCacheFiles()
{
	medResPE->RemoveCacheFile("pe" , mapInfo->map.name);
	lowResPE->RemoveCacheFile("pe2", mapInfo->map.name);
}


std::uint32_t CPathManager::GetPathCheckSum() const {
	assert(IsFinalized());
	return (medResPE->GetPathChecksum() + lowResPE->GetPathChecksum());
}

std::int64_t CPathManager::Finalize() {
	const spring_time t0 = spring_gettime();

	{
		maxResPF = &gMaxResPF;
		medResPE = &gMedResPE;
		lowResPE = &gLowResPE;

		// maxResPF only runs on the main thread, so can be unsafe
		maxResPF->Init(false);
		medResPE->Init(maxResPF, MEDRES_PE_BLOCKSIZE, "pe" , mapInfo->map.name);
		lowResPE->Init(medResPE, LOWRES_PE_BLOCKSIZE, "pe2", mapInfo->map.name);
	}

	const spring_time dt = spring_gettime() - t0;
	return (dt.toMilliSecsi());
}


void CPathManager::FinalizePath(MultiPath* path, const float3 startPos, const float3 goalPos, const bool cantGetCloser)
{
	IPath::Path* sp = &path->lowResPath;
	IPath::Path* ep = &path->maxResPath;

	if (!path->medResPath.path.empty())
		sp = &path->medResPath;
	if (!path->maxResPath.path.empty())
		sp = &path->maxResPath;

	if (!sp->path.empty()) {
		sp->path.back() = startPos;
		sp->path.back().y = CMoveMath::yLevel(*path->moveDef, sp->path.back());
	}

	if (!path->maxResPath.path.empty() && !path->medResPath.path.empty())
		path->medResPath.path.back() = path->maxResPath.path.front();

	if (!path->medResPath.path.empty() && !path->lowResPath.path.empty())
		path->lowResPath.path.back() = path->medResPath.path.front();

	if (cantGetCloser)
		return;


	if (!path->medResPath.path.empty())
		ep = &path->medResPath;
	if (!path->lowResPath.path.empty())
		ep = &path->lowResPath;

	if (!ep->path.empty()) {
		ep->path.front() = goalPos;
		ep->path.front().y = CMoveMath::yLevel(*path->moveDef, ep->path.front());
	}
}


IPath::SearchResult CPathManager::ArrangePath(
	MultiPath* newPath,
	const MoveDef* moveDef,
	const float3& startPos,
	const float3& goalPos,
	CSolidObject* caller
) const {
	CPathFinderDef* pfDef = &newPath->peDef;

	// choose the PF or the PE depending on the projected 2D goal-distance
	// NOTE: this distance can be far smaller than the actual path length!
	// NOTE: take height difference into consideration for "special" cases
	// (unit at top of cliff, goal at bottom or vv.)
	const float heurGoalDist2D = pfDef->Heuristic(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE, 1) + math::fabs(goalPos.y - startPos.y) / SQUARE_SIZE;
	const float searchDistances[] = {std::numeric_limits<float>::max(), MEDRES_SEARCH_DISTANCE, MAXRES_SEARCH_DISTANCE};

	// MAX_SEARCHED_NODES_PF is 65536, MAXRES_SEARCH_DISTANCE is 50 squares
	// the circular-constraint area therefore is PI*50*50 squares (i.e. 7854
	// rounded up to nearest integer) which means MAX_SEARCHED_NODES_*>>3 is
	// only slightly larger (8192) so the constraint has no purpose even for
	// max-res queries (!)
	assert(MAX_SEARCHED_NODES_PF <= 65536u);
	assert(MAXRES_SEARCH_DISTANCE <= 50.0f);

	constexpr unsigned int nodeLimits[] = {MAX_SEARCHED_NODES_PE >> 3, MAX_SEARCHED_NODES_PE >> 3, MAX_SEARCHED_NODES_PF >> 3};

	constexpr bool useConstraints[] = {false, false, false};
	constexpr bool allowRawSearch[] = {false, false, false};

	IPathFinder* pathFinders[] = {lowResPE, medResPE, maxResPF};
	IPath::Path* pathObjects[] = {&newPath->lowResPath, &newPath->medResPath, &newPath->maxResPath};

	IPath::SearchResult bestResult = IPath::Error;

#if 1

	unsigned int bestSearch = -1u; // index

	enum {
		PATH_LOW_RES = 0,
		PATH_MED_RES = 1,
		PATH_MAX_RES = 2,
	};

	{
		if (heurGoalDist2D <= (MAXRES_SEARCH_DISTANCE * modInfo.pfRawDistMult)) {
			pfDef->AllowRawPathSearch( true);
			pfDef->AllowDefPathSearch(false); // block default search

			// only the max-res CPathFinder implements DoRawSearch
			bestResult = pathFinders[PATH_MAX_RES]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[PATH_MAX_RES], nodeLimits[PATH_MAX_RES]);
			bestSearch = PATH_MAX_RES;

			pfDef->AllowRawPathSearch(false);
			pfDef->AllowDefPathSearch( true);
		}

		if (bestResult != IPath::Ok) {
			// try each pathfinder in order from MAX to LOW limited by distance,
			// with constraints disabled for all three since these break search
			// completeness (CPU usage is still limited by MAX_SEARCHED_NODES_*)
			for (int n = PATH_MAX_RES; n >= PATH_LOW_RES; n--) {
				// distance-limits are in ascending order
				if (heurGoalDist2D > searchDistances[n])
					continue;

				pfDef->DisableConstraint(!useConstraints[n]);
				pfDef->AllowRawPathSearch(allowRawSearch[n]);

				const IPath::SearchResult currResult = pathFinders[n]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[n], nodeLimits[n]);

				// note: GEQ s.t. MED-OK will be preferred over LOW-OK, etc
				if (currResult >= bestResult)
					continue;

				bestResult = currResult;
				bestSearch = n;

				if (currResult == IPath::Ok)
					break;
			}
		}
	}

	for (unsigned int n = PATH_LOW_RES; n <= PATH_MAX_RES; n++) {
		if (n != bestSearch) {
			pathObjects[n]->path.clear();
			pathObjects[n]->squares.clear();
		}
	}

	if (bestResult == IPath::Ok)
		return bestResult;

	// if we did not get a complete path with distance/search
	// constraints enabled, run a final unconstrained fallback
	// MED search (unconstrained MAX search is not useful with
	// current node limits and could kill performance without)
	if (heurGoalDist2D > searchDistances[PATH_MED_RES]) {
		pfDef->DisableConstraint(true);

		// we can only have a low-res result at this point
		pathObjects[PATH_LOW_RES]->path.clear();
		pathObjects[PATH_LOW_RES]->squares.clear();

		bestResult = std::min(bestResult, pathFinders[PATH_MED_RES]->GetPath(*moveDef, *pfDef, caller, startPos, *pathObjects[PATH_MED_RES], nodeLimits[PATH_MED_RES]));
	}

	return bestResult;


#else


	enum {
		PATH_MAX_RES = 0,
		PATH_MED_RES = 1,
		PATH_LOW_RES = 3
	};

	int origPathRes = PATH_LOW_RES;

	// first attempt - use ideal pathfinder (performance-wise)
	{
		if (heurGoalDist2D < MAXRES_SEARCH_DISTANCE) {
			origPathRes = PATH_MAX_RES;
		} else if (heurGoalDist2D < MEDRES_SEARCH_DISTANCE) {
			origPathRes = PATH_MED_RES;
		//} else {
		//	origPathRes = PATH_LOW_RES;
		}

		switch (origPathRes) {
			case PATH_MAX_RES: bestResult = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, nodeLimits[2]); break;
			case PATH_MED_RES: bestResult = medResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->medResPath, nodeLimits[1]); break;
			case PATH_LOW_RES: bestResult = lowResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->lowResPath, nodeLimits[0]); break;
		}

		if (bestResult == IPath::Ok) {
			return bestResult;
		}
	}

	// second attempt - try to reverse path
	/*{
		CCircularSearchConstraint reversedPfDef(goalPos, startPos, pfDef->sqGoalRadius, 7.0f, 8000);
		switch (pathres) {
			case PATH_MAX_RES: bestResult = maxResPF->GetPath(*moveDef, reversedPfDef, caller, goalPos, newPath->maxResPath, nodeLimits[2]); break;
			case PATH_MED_RES: bestResult = medResPE->GetPath(*moveDef, reversedPfDef, caller, goalPos, newPath->medResPath, nodeLimits[1]); break;
			case PATH_LOW_RES: bestResult = lowResPE->GetPath(*moveDef, reversedPfDef, caller, goalPos, newPath->lowResPath, nodeLimits[0]); break;
		}

		if (bestResult == IPath::Ok) {
			assert(false);

			float3 midPos;
			switch (pathres) {
				case PATH_MAX_RES: midPos = newPath->maxResPath.path.back(); break;
				case PATH_MED_RES: midPos = newPath->medResPath.path.back(); break;
				case PATH_LOW_RES: midPos = newPath->lowResPath.path.back(); break;
			}

			CCircularSearchConstraint midPfDef(startPos, midPos, pfDef->sqGoalRadius, 3.0f, 8000);
			bestResult = maxResPF->GetPath(*moveDef, midPfDef, caller, startPos, newPath->maxResPath, MAX_SEARCHED_NODES_PF >> 3);

			CCircularSearchConstraint restPfDef(midPos, goalPos, pfDef->sqGoalRadius, 7.0f, 8000);
			switch (pathres) {
				case PATH_MAX_RES:
				case PATH_MED_RES: bestResult = medResPE->GetPath(*moveDef, restPfDef, caller, startPos, newPath->medResPath, nodeLimits[1]); break;
				case PATH_LOW_RES: bestResult = lowResPE->GetPath(*moveDef, restPfDef, caller, startPos, newPath->lowResPath, nodeLimits[0]); break;
			}

			return bestResult;

		}
	}*/

	// third attempt - use better pathfinder
	{
		int advPathRes = origPathRes;
		int maxRes = (heurGoalDist2D < (MAXRES_SEARCH_DISTANCE * 2.0f)) ? PATH_MAX_RES : PATH_MED_RES;

		while (--advPathRes >= maxRes) {
			switch (advPathRes) {
				case PATH_MAX_RES: bestResult = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, nodeLimits[2]); break;
				case PATH_MED_RES: bestResult = medResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->medResPath, nodeLimits[1]); break;
				case PATH_LOW_RES: bestResult = lowResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->lowResPath, nodeLimits[0]); break;
			}

			if (bestResult == IPath::Ok) {
				return bestResult;
			}
		}
	}

	// fourth attempt - unconstrained search radius (performance heavy, esp. on max_res)
	pfDef->DisableConstraint(true);
	if (origPathRes > PATH_MAX_RES) {
		int advPathRes = origPathRes;
		int maxRes = PATH_MED_RES;

		while (--advPathRes >= maxRes) {
			switch (advPathRes) {
				case PATH_MAX_RES: bestResult = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, nodeLimits[2]); break;
				case PATH_MED_RES: bestResult = medResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->medResPath, nodeLimits[1]); break;
				case PATH_LOW_RES: bestResult = lowResPE->GetPath(*moveDef, *pfDef, caller, startPos, newPath->lowResPath, nodeLimits[0]); break;
			}

			if (bestResult == IPath::Ok) {
				return bestResult;
			}
		}
	}

	LOG_L(L_DEBUG, "PathManager: no path found");
	return bestResult;
	#endif
}


/*
Request a new multipath, store the result and return a handle-id to it.
*/
unsigned int CPathManager::RequestPath(
	CSolidObject* caller,
	const MoveDef* moveDef,
	float3 startPos,
	float3 goalPos,
	float goalRadius,
	bool synced
) {
	if (!IsFinalized())
		return 0;

	// in misc since it is called from many points
	SCOPED_TIMER("Misc::Path::RequestPath");
	startPos.ClampInBounds();
	goalPos.ClampInBounds();

	// Create an estimator definition.
	goalRadius = std::max<float>(goalRadius, PATH_NODE_SPACING * SQUARE_SIZE); //FIXME do on a per PE & PF level?
	assert(moveDef == moveDefHandler.GetMoveDefByPathType(moveDef->pathType));

	MultiPath newPath = MultiPath(moveDef, startPos, goalPos, goalRadius);
	newPath.finalGoal = goalPos;
	newPath.caller = caller;
	newPath.peDef.synced = synced;

	if (caller != nullptr)
		caller->UnBlock();

	const IPath::SearchResult result = ArrangePath(&newPath, moveDef, startPos, goalPos, caller);

	unsigned int pathID = 0;

	if (result != IPath::Error) {
		if (newPath.maxResPath.path.empty()) {
			if (result != IPath::CantGetCloser) {
				LowRes2MedRes(newPath, startPos, caller, synced);
				MedRes2MaxRes(newPath, startPos, caller, synced);
			} else {
				// add one dummy waypoint so that the calling MoveType
				// does not consider this request a failure, which can
				// happen when startPos is very close to goalPos
				//
				// otherwise, code relying on MoveType::progressState
				// (eg. BuilderCAI::MoveInBuildRange) would misbehave
				// (eg. reject build orders)
				newPath.maxResPath.path.push_back(startPos);
				newPath.maxResPath.squares.push_back(int2(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE));
			}
		}

		FinalizePath(&newPath, startPos, goalPos, result == IPath::CantGetCloser);
		newPath.searchResult = result;
		pathID = Store(newPath);
	}

	if (caller != nullptr)
		caller->Block();

	return pathID;
}


// converts part of a med-res path into a max-res path
void CPathManager::MedRes2MaxRes(MultiPath& multiPath, const float3& startPos, const CSolidObject* owner, bool synced) const
{
	assert(IsFinalized());

	IPath::Path& maxResPath = multiPath.maxResPath;
	IPath::Path& medResPath = multiPath.medResPath;
	IPath::Path& lowResPath = multiPath.lowResPath;

	if (medResPath.path.empty())
		return;

	medResPath.path.pop_back();

	// remove med-res waypoints until the next one is far enough
	// note: this should normally never consume the entire path!
	while (!medResPath.path.empty() && startPos.SqDistance2D(medResPath.path.back()) < Square(MAXRES_SEARCH_DISTANCE_EXT)) {
		medResPath.path.pop_back();
	}

	// get the goal of the detailed search
	float3 goalPos = medResPath.pathGoal;
	if (!medResPath.path.empty())
		goalPos = medResPath.path.back();

	// define the search
	CCircularSearchConstraint rangedGoalDef(startPos, goalPos, 0.0f, 2.0f, Square(MAXRES_SEARCH_DISTANCE));
	rangedGoalDef.synced = synced;
	// TODO
	// rangedGoalDef.allowRawPath = true;

	// Perform the search.
	// If this is the final improvement of the path, then use the original goal.
	const auto& pfd = (medResPath.path.empty() && lowResPath.path.empty()) ? multiPath.peDef : rangedGoalDef;
	const IPath::SearchResult result = maxResPF->GetPath(*multiPath.moveDef, pfd, owner, startPos, maxResPath, MAX_SEARCHED_NODES_ON_REFINE);

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		maxResPath.pathGoal = goalPos;
	}
}

// converts part of a low-res path into a med-res path
void CPathManager::LowRes2MedRes(MultiPath& multiPath, const float3& startPos, const CSolidObject* owner, bool synced) const
{
	assert(IsFinalized());

	IPath::Path& medResPath = multiPath.medResPath;
	IPath::Path& lowResPath = multiPath.lowResPath;

	if (lowResPath.path.empty())
		return;

	lowResPath.path.pop_back();

	// remove low-res waypoints until the next one is far enough
	// note: this should normally never consume the entire path!
	while (!lowResPath.path.empty() && startPos.SqDistance2D(lowResPath.path.back()) < Square(MEDRES_SEARCH_DISTANCE_EXT)) {
		lowResPath.path.pop_back();
	}

	// get the goal of the detailed search
	float3 goalPos = lowResPath.pathGoal;
	if (!lowResPath.path.empty())
		goalPos = lowResPath.path.back();

	// define the search
	CCircularSearchConstraint rangedGoalDef(startPos, goalPos, 0.0f, 2.0f, Square(MEDRES_SEARCH_DISTANCE));
	rangedGoalDef.synced = synced;

	// Perform the search.
	// If there is no low-res path left, use original goal.
	const auto& pfd = (lowResPath.path.empty()) ? multiPath.peDef : rangedGoalDef;
	const IPath::SearchResult result = medResPE->GetPath(*multiPath.moveDef, pfd, owner, startPos, medResPath, MAX_SEARCHED_NODES_ON_REFINE);

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		medResPath.pathGoal = goalPos;
	}
}


/*
Removes and return the next waypoint in the multipath corresponding to given id.
*/
float3 CPathManager::NextWayPoint(
	const CSolidObject* owner,
	unsigned int pathID,
	unsigned int numRetries,
	float3 callerPos,
	float radius,
	bool synced
) {
	// in misc since it is called from many points
	SCOPED_TIMER("Misc::Path::NextWayPoint");

	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;

	// 0 indicates the null-path ID
	if (pathID == 0)
		return noPathPoint;

	// find corresponding multipath entry
	MultiPath* multiPath = GetMultiPath(pathID);

	if (multiPath == nullptr)
		return noPathPoint;

	if (numRetries > MAX_PATH_REFINEMENT_DEPTH)
		return (multiPath->finalGoal);

	IPath::Path& maxResPath = multiPath->maxResPath;
	IPath::Path& medResPath = multiPath->medResPath;
	IPath::Path& lowResPath = multiPath->lowResPath;

	if ((callerPos == ZeroVector) && !maxResPath.path.empty())
		callerPos = maxResPath.path.back();

	assert(multiPath->peDef.synced == synced);

	#define EXTEND_PATH_POINTS(curResPts, nxtResPts, dist) ((!curResPts.empty() && (curResPts.back()).SqDistance2D(callerPos) < Square((dist))) || nxtResPts.size() <= 2)
	const bool extendMaxResPath = EXTEND_PATH_POINTS(medResPath.path, maxResPath.path, MAXRES_SEARCH_DISTANCE_EXT);
	const bool extendMedResPath = EXTEND_PATH_POINTS(lowResPath.path, medResPath.path, MEDRES_SEARCH_DISTANCE_EXT);
	#undef EXTEND_PATH_POINTS

	// check whether the max-res path needs extending through
	// recursive refinement of its lower-resolution segments
	// if so, check if the med-res path also needs extending
	if (extendMaxResPath) {
		if (multiPath->caller != nullptr)
			multiPath->caller->UnBlock();

		if (extendMedResPath)
			LowRes2MedRes(*multiPath, callerPos, owner, synced);

		MedRes2MaxRes(*multiPath, callerPos, owner, synced);

		if (multiPath->caller != nullptr)
			multiPath->caller->Block();

		FinalizePath(multiPath, callerPos, multiPath->finalGoal, multiPath->searchResult == IPath::CantGetCloser);
	}

	float3 waypoint = noPathPoint;

	do {
		// eat waypoints from the max-res path until we
		// find one that lies outside the search-radius
		// or equals the goal
		//
		// if this is not possible, then either we are
		// at the goal OR the path could not reach all
		// the way to it (ie. a GoalOutOfRange result)
		// OR we are stuck on an impassable square
		if (maxResPath.path.empty()) {
			if (lowResPath.path.empty() && medResPath.path.empty()) {
				if (multiPath->searchResult == IPath::Ok)
					waypoint = multiPath->finalGoal;

				// [else]
				// reached in the CantGetCloser case for any max-res searches
				// that start within their goal radius (ie. have no waypoints)
				// RequestPath always puts startPos into maxResPath to handle
				// this so waypoint will have been set to it (during previous
				// iteration) if we end up here
			} else {
				waypoint = NextWayPoint(owner, pathID, numRetries + 1, callerPos, radius, synced);
			}

			break;
		} else {
			waypoint = maxResPath.path.back();
			maxResPath.path.pop_back();
		}
	} while ((callerPos.SqDistance2D(waypoint) < Square(radius)) && (waypoint != maxResPath.pathGoal));

	// y=0 indicates this is not a temporary waypoint
	// (the default PFS does not queue path-requests)
	return (waypoint * XZVector);
}


// Tells estimators about changes in or on the map.
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2, unsigned int /*type*/) {
	if (!IsFinalized())
		return;

	medResPE->MapChanged(x1, z1, x2, z2);

	// low-res PE will be informed via (medRes)PE::Update
	if (true && medResPE->nextPathEstimator != nullptr)
		return;

	lowResPE->MapChanged(x1, z1, x2, z2);
}



void CPathManager::Update()
{
	SCOPED_TIMER("Sim::Path");
	assert(IsFinalized());

	pathFlowMap->Update();
	pathHeatMap->Update();

	medResPE->Update();
	lowResPE->Update();
}

// used to deposit heat on the heat-map as a unit moves along its path
void CPathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID)
{
	assert(IsFinalized());

	pathFlowMap->AddFlow(owner);
	pathHeatMap->AddHeat(owner, this, pathID);
}



// get the waypoints in world-coordinates
void CPathManager::GetDetailedPath(unsigned pathID, std::vector<float3>& points) const
{
	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr) {
		points.clear();
		return;
	}

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::path_list_type& maxResPoints = path.path;

	points.clear();
	points.reserve(maxResPoints.size());

	for (auto pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}

void CPathManager::GetDetailedPathSquares(unsigned pathID, std::vector<int2>& squares) const
{
	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr) {
		squares.clear();
		return;
	}

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::square_list_type& maxResSquares = path.squares;

	squares.clear();
	squares.reserve(maxResSquares.size());

	for (auto pvi = maxResSquares.rbegin(); pvi != maxResSquares.rend(); ++pvi) {
		squares.push_back(*pvi);
	}
}



void CPathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	points.clear();
	starts.clear();

	const MultiPath* multiPath = GetMultiPathConst(pathID);

	if (multiPath == nullptr)
		return;

	const IPath::path_list_type& maxResPoints = multiPath->maxResPath.path;
	const IPath::path_list_type& medResPoints = multiPath->medResPath.path;
	const IPath::path_list_type& lowResPoints = multiPath->lowResPath.path;

	points.reserve(maxResPoints.size() + medResPoints.size() + lowResPoints.size());
	starts.reserve(3);
	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = medResPoints.rbegin(); pvi != medResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = lowResPoints.rbegin(); pvi != lowResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}



bool CPathManager::SetNodeExtraCost(unsigned int x, unsigned int z, float cost, bool synced) {
	if (!IsFinalized())
		return 0.0f;

	if (x >= mapDims.mapx) { return false; }
	if (z >= mapDims.mapy) { return false; }

	PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	PathNodeStateBuffer& medResBuf = medResPE->GetNodeStateBuffer();
	PathNodeStateBuffer& lowResBuf = lowResPE->GetNodeStateBuffer();

	maxResBuf.SetNodeExtraCost(x, z, cost, synced);
	medResBuf.SetNodeExtraCost(x, z, cost, synced);
	lowResBuf.SetNodeExtraCost(x, z, cost, synced);
	return true;
}

bool CPathManager::SetNodeExtraCosts(const float* costs, unsigned int sizex, unsigned int sizez, bool synced) {
	if (!IsFinalized())
		return 0.0f;

	if (sizex < 1 || sizex > mapDims.mapx) { return false; }
	if (sizez < 1 || sizez > mapDims.mapy) { return false; }

	PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	PathNodeStateBuffer& medResBuf = medResPE->GetNodeStateBuffer();
	PathNodeStateBuffer& lowResBuf = lowResPE->GetNodeStateBuffer();

	// make all buffers share the same cost-overlay
	maxResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	medResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	lowResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	return true;
}

float CPathManager::GetNodeExtraCost(unsigned int x, unsigned int z, bool synced) const {
	if (!IsFinalized())
		return 0.0f;

	if (x >= mapDims.mapx) { return 0.0f; }
	if (z >= mapDims.mapy) { return 0.0f; }

	const PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	const float cost = maxResBuf.GetNodeExtraCost(x, z, synced);
	return cost;
}

const float* CPathManager::GetNodeExtraCosts(bool synced) const {
	if (!IsFinalized())
		return nullptr;

	const PathNodeStateBuffer& buf = maxResPF->GetNodeStateBuffer();
	const float* costs = buf.GetNodeExtraCosts(synced);
	return costs;
}

int2 CPathManager::GetNumQueuedUpdates() const {
	int2 data;

	if (IsFinalized()) {
		data.x = medResPE->updatedBlocks.size();
		data.y = lowResPE->updatedBlocks.size();
	}

	return data;
}

