/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "PathManager.h"
#include "PathConstants.h"
#include "PathFinder.h"
#include "PathEstimator.h"
#include "PathFlowMap.hpp"
#include "PathHeatMap.hpp"
#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"

#define PM_UNCONSTRAINED_MAXRES_FALLBACK_SEARCH 0
#define PM_UNCONSTRAINED_MEDRES_FALLBACK_SEARCH 1
#define PM_UNCONSTRAINED_LOWRES_FALLBACK_SEARCH 1



CPathManager::CPathManager(): nextPathID(0)
{
	CPathFinder::InitDirectionVectorsTable();
	CPathFinder::InitDirectionCostsTable();
	CPathEstimator::InitDirectionVectorsTable();

	maxResPF = NULL;
	medResPE = NULL;
	lowResPE = NULL;

	pathFlowMap = PathFlowMap::GetInstance();
	pathHeatMap = PathHeatMap::GetInstance();
}

CPathManager::~CPathManager()
{
	delete lowResPE; lowResPE = NULL;
	delete medResPE; medResPE = NULL;
	delete maxResPF; maxResPF = NULL;

	PathHeatMap::FreeInstance(pathHeatMap);
	PathFlowMap::FreeInstance(pathFlowMap);
}

boost::int64_t CPathManager::Finalize() {
	const spring_time t0 = spring_gettime();

	{
		maxResPF = new CPathFinder();
		medResPE = new CPathEstimator(maxResPF, MEDRES_PE_BLOCKSIZE, "pe",  mapInfo->map.name);
		lowResPE = new CPathEstimator(maxResPF, LOWRES_PE_BLOCKSIZE, "pe2", mapInfo->map.name);

		#ifdef SYNCDEBUG
		// clients may have a non-writable cache directory (which causes
		// the estimator path-file checksum to remain zero), so we can't
		// update the sync-checker with this in normal builds
		// NOTE: better to just checksum the in-memory data and broadcast
		// that instead of relying on the zip-file CRC?
		{ SyncedUint tmp(GetPathCheckSum()); }
		#endif
	}

	const spring_time t1 = spring_gettime();
	const spring_time dt = t1 - t0;

	return (dt.toMilliSecsi());
}



/*
Help-function.
Turns a start->goal-request into a well-defined request.
*/
unsigned int CPathManager::RequestPath(
	CSolidObject* caller,
	const MoveDef* moveDef,
	const float3& startPos,
	const float3& goalPos,
	float goalRadius,
	bool synced
) {
	float3 sp(startPos); sp.ClampInBounds();
	float3 gp(goalPos); gp.ClampInBounds();

	// Create an estimator definition.
	CCircularSearchConstraint* pfDef = new CCircularSearchConstraint(sp, gp, goalRadius, 3.0f, 2000);

	// Make request.
	return (RequestPath(moveDef, sp, gp, pfDef, caller, synced));
}

/*
Request a new multipath, store the result and return a handle-id to it.
*/
unsigned int CPathManager::RequestPath(
	const MoveDef* moveDef,
	const float3& startPos,
	const float3& goalPos,
	CPathFinderDef* pfDef,
	CSolidObject* caller,
	bool synced
) {
	SCOPED_TIMER("PathManager::RequestPath");

	if (!IsFinalized())
		return 0;

	assert(moveDef == moveDefHandler->GetMoveDefByPathType(moveDef->pathType));

	// Creates a new multipath.
	IPath::SearchResult result = IPath::Error;
	MultiPath* newPath = new MultiPath(startPos, pfDef, moveDef);
	newPath->finalGoal = goalPos;
	newPath->caller = caller;

	if (caller != NULL) {
		caller->UnBlock();
	}

	unsigned int pathID = 0;

	// choose the PF or the PE depending on the projected 2D goal-distance
	// NOTE: this distance can be far smaller than the actual path length!
	// NOTE: take height difference into consideration for "special" cases
	// (unit at top of cliff, goal at bottom or vv.)
	const float heuristicGoalDist2D = pfDef->Heuristic(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE) + math::fabs(goalPos.y - startPos.y) / SQUARE_SIZE;

	if (heuristicGoalDist2D < MAXRES_SEARCH_DISTANCE) {
		result = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, MAX_SEARCHED_NODES_PF >> 3, true, false, true, false, synced);

		#if (PM_UNCONSTRAINED_MAXRES_FALLBACK_SEARCH == 1)
		// unnecessary so long as a fallback path exists within the
		// {med, low}ResPE's restricted search region (in many cases
		// where it does not, the goal position is unreachable anyway)
		pfDef->DisableConstraint(true);
		#endif

		// fallback (note that this uses the estimators as backup,
		// unconstrained PF queries are too expensive on average)
		if (result != IPath::Ok) {
			result = medResPE->GetPath(*moveDef, *pfDef, startPos, newPath->medResPath, MAX_SEARCHED_NODES_PE >> 3, synced);
		}
		if (result != IPath::Ok) {
			result = lowResPE->GetPath(*moveDef, *pfDef, startPos, newPath->lowResPath, MAX_SEARCHED_NODES_PE >> 3, synced);
		}
	} else if (heuristicGoalDist2D < MEDRES_SEARCH_DISTANCE) {
		result = medResPE->GetPath(*moveDef, *pfDef, startPos, newPath->medResPath, MAX_SEARCHED_NODES_PE >> 3, synced);

		// CantGetCloser may be a false positive due to PE approximations and large goalRadius
		if (result == IPath::CantGetCloser && (startPos - goalPos).SqLength2D() > pfDef->sqGoalRadius) {
			result = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, MAX_SEARCHED_NODES_PF >> 3, true, false, true, false, synced);
		}

		#if (PM_UNCONSTRAINED_MEDRES_FALLBACK_SEARCH == 1)
		pfDef->DisableConstraint(true);
		#endif

		// fallback
		if (result != IPath::Ok) {
			result = medResPE->GetPath(*moveDef, *pfDef, startPos, newPath->medResPath, MAX_SEARCHED_NODES_PE >> 3, synced);
		}
	} else {
		result = lowResPE->GetPath(*moveDef, *pfDef, startPos, newPath->lowResPath, MAX_SEARCHED_NODES_PE >> 3, synced);

		// CantGetCloser may be a false positive due to PE approximations and large goalRadius
		if (result == IPath::CantGetCloser && (startPos - goalPos).SqLength2D() > pfDef->sqGoalRadius) {
			result = medResPE->GetPath(*moveDef, *pfDef, startPos, newPath->medResPath, MAX_SEARCHED_NODES_PE >> 3, synced);

			#if 0
			if (result == IPath::CantGetCloser) // Same thing again
				result = maxResPF->GetPath(*moveDef, *pfDef, caller, startPos, newPath->maxResPath, MAX_SEARCHED_NODES_PF >> 3, true, false, true, false, synced);
			#endif
		}

		#if (PM_UNCONSTRAINED_LOWRES_FALLBACK_SEARCH == 1)
		pfDef->DisableConstraint(true);
		#endif

		// fallback
		if (result != IPath::Ok) {
			result = lowResPE->GetPath(*moveDef, *pfDef, startPos, newPath->lowResPath, MAX_SEARCHED_NODES_PE >> 3, synced);
		}
	}

	if (result != IPath::Error) {
		if (result != IPath::CantGetCloser) {
			LowRes2MedRes(*newPath, startPos, caller, synced);
			MedRes2MaxRes(*newPath, startPos, caller, synced);
		} else {
			// add one dummy waypoint so that the calling MoveType
			// does not consider this request a failure, which can
			// happen when startPos is very close to goalPos
			//
			// otherwise, code relying on MoveType::progressState
			// (eg. BuilderCAI::MoveInBuildRange) would misbehave
			// (eg. reject build orders)
			if (newPath->maxResPath.path.empty()) {
				newPath->maxResPath.path.push_back(startPos);
				newPath->maxResPath.squares.push_back(int2(startPos.x / SQUARE_SIZE, startPos.z / SQUARE_SIZE));
			}
		}

		newPath->searchResult = result;
		pathID = Store(newPath);
	} else {
		delete newPath;
	}

	if (caller != NULL) {
		caller->Block();
	}

	return pathID;
}


/*
Store a new multipath into the pathmap.
*/
unsigned int CPathManager::Store(MultiPath* path)
{
	pathMap[++nextPathID] = path;
	return nextPathID;
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
	while (!medResPath.path.empty() && (medResPath.path.back()).SqDistance2D(startPos) < Square(MAXRES_SEARCH_DISTANCE * SQUARE_SIZE)) {
		medResPath.path.pop_back();
	}

	// get the goal of the detailed search
	float3 goalPos;

	if (medResPath.path.empty()) {
		goalPos = medResPath.pathGoal;
	} else {
		goalPos = medResPath.path.back();
	}

	// define the search
	CCircularSearchConstraint rangedGoalPFD(startPos, goalPos, 0.0f, 2.0f, 1000);

	// Perform the search.
	// If this is the final improvement of the path, then use the original goal.
	IPath::SearchResult result = IPath::Error;

	if (medResPath.path.empty() && lowResPath.path.empty()) {
		result = maxResPF->GetPath(*multiPath.moveDef, *multiPath.peDef, owner, startPos, maxResPath, MAX_SEARCHED_NODES_PF >> 3, true, false, true, false, synced);
	} else {
		result = maxResPF->GetPath(*multiPath.moveDef, rangedGoalPFD, owner, startPos, maxResPath, MAX_SEARCHED_NODES_PF >> 3, true, false, true, false, synced);
	}

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
	while (!lowResPath.path.empty() && (lowResPath.path.back()).SqDistance2D(startPos) < Square(MEDRES_SEARCH_DISTANCE * SQUARE_SIZE)) {
		lowResPath.path.pop_back();
	}

	// get the goal of the detailed search
	float3 goalPos;

	if (lowResPath.path.empty()) {
		goalPos = lowResPath.pathGoal;
	} else {
		goalPos = lowResPath.path.back();
	}

	// define the search
	CCircularSearchConstraint rangedGoalDef(startPos, goalPos, 0.0f, 2.0f, 20);

	// Perform the search.
	// If there is no low-res path left, use original goal.
	IPath::SearchResult result = IPath::Error;

	if (lowResPath.path.empty()) {
		result = medResPE->GetPath(*multiPath.moveDef, *multiPath.peDef, startPos, medResPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	} else {
		result = medResPE->GetPath(*multiPath.moveDef, rangedGoalDef, startPos, medResPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	}

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
	SCOPED_TIMER("PathManager::NextWayPoint");

	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;

	// 0 indicates the null-path ID
	if (pathID == 0)
		return noPathPoint;

	// find corresponding multipath entry
	MultiPath* multiPath = GetMultiPath(pathID);

	if (multiPath == NULL)
		return noPathPoint;

	if (numRetries > MAX_PATH_REFINEMENT_DEPTH)
		return (multiPath->finalGoal);

	IPath::Path& maxResPath = multiPath->maxResPath;
	IPath::Path& medResPath = multiPath->medResPath;
	IPath::Path& lowResPath = multiPath->lowResPath;

	IPath::path_list_type& maxResPathPoints = maxResPath.path;
	IPath::path_list_type& medResPathPoints = medResPath.path;
	IPath::path_list_type& lowResPathPoints = lowResPath.path;

	if (callerPos == ZeroVector) {
		if (!maxResPathPoints.empty()) {
			callerPos = maxResPathPoints.back();
		}
	}

	#define EXTEND_PATH_POINTS(curResPts, nxtResPts, dist) ((!curResPts.empty() && (curResPts.back()).SqDistance2D(callerPos) < Square((dist))) || nxtResPts.size() <= 2)
	const bool extendMaxResPath = EXTEND_PATH_POINTS(medResPathPoints, maxResPathPoints, MIN_MAXRES_SEARCH_DISTANCE * SQUARE_SIZE);
	const bool extendMedResPath = EXTEND_PATH_POINTS(lowResPathPoints, medResPathPoints, MIN_MEDRES_SEARCH_DISTANCE * SQUARE_SIZE);
	#undef EXTEND_PATH_POINTS

	// check whether the max-res path needs extending through
	// recursive refinement of its lower-resolution segments
	// if so, check if the med-res path also needs extending
	if (extendMaxResPath) {
		if (extendMedResPath) {
			LowRes2MedRes(*multiPath, callerPos, owner, synced);
		}

		if (multiPath->caller != NULL) {
			multiPath->caller->UnBlock();
		}

		MedRes2MaxRes(*multiPath, callerPos, owner, synced);

		if (multiPath->caller != NULL) {
			multiPath->caller->Block();
		}
	}

	float3 waypoint;

	do {
		// get the next waypoint from the max-res path
		//
		// if this is not possible, then either we are
		// at the goal OR the path could not reach all
		// the way to it (ie. a GoalOutOfRange result)
		// OR we are stuck on an impassable square
		if (maxResPathPoints.empty()) {
			if (lowResPathPoints.empty() && medResPath.path.empty()) {
				if (multiPath->searchResult == IPath::Ok) {
					waypoint = multiPath->finalGoal; break;
				} else {
					// note: unreachable?
					waypoint = noPathPoint; break;
				}
			} else {
				waypoint = NextWayPoint(owner, pathID, numRetries + 1, callerPos, radius, synced);
				break;
			}
		} else {
			waypoint = maxResPathPoints.back();
			maxResPathPoints.pop_back();
		}
	} while ((callerPos.SqDistance2D(waypoint) < Square(radius)) && (waypoint != maxResPath.pathGoal));

	// y=0 indicates this is not a temporary waypoint
	// (the default PFS does not queue path-requests)
	return (waypoint * XZVector);
}


// Delete a given multipath from the collection.
void CPathManager::DeletePath(unsigned int pathID) {
	if (pathID == 0)
		return;

	const auto pi = pathMap.find(pathID);

	if (pi == pathMap.end())
		return;

	MultiPath* multiPath = pi->second;
	pathMap.erase(pathID);
	delete multiPath;
}



// Tells estimators about changes in or on the map.
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2, unsigned int /*type*/) {
	SCOPED_TIMER("PathManager::TerrainChange");

	if (!IsFinalized())
		return;

	medResPE->MapChanged(x1, z1, x2, z2);
	lowResPE->MapChanged(x1, z1, x2, z2);
}



void CPathManager::Update()
{
	SCOPED_TIMER("PathManager::Update");
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
	points.clear();

	MultiPath* multiPath = GetMultiPath(pathID);

	if (multiPath == NULL)
		return;

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::path_list_type& maxResPoints = path.path;

	points.reserve(maxResPoints.size());

	for (auto pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}

void CPathManager::GetDetailedPathSquares(unsigned pathID, std::vector<int2>& points) const
{
	points.clear();

	MultiPath* multiPath = GetMultiPath(pathID);

	if (multiPath == NULL)
		return;

	const IPath::Path& path = multiPath->maxResPath;
	const IPath::square_list_type& maxResSquares = path.squares;

	points.reserve(maxResSquares.size());

	for (auto pvi = maxResSquares.rbegin(); pvi != maxResSquares.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}



void CPathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	points.clear();
	starts.clear();

	MultiPath* multiPath = GetMultiPath(pathID);

	if (multiPath == NULL)
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



boost::uint32_t CPathManager::GetPathCheckSum() const {
	assert(IsFinalized());
	return (medResPE->GetPathChecksum() + lowResPE->GetPathChecksum());
}



bool CPathManager::SetNodeExtraCost(unsigned int x, unsigned int z, float cost, bool synced) {
	if (!IsFinalized())
		return 0.0f;

	if (x >= gs->mapx) { return false; }
	if (z >= gs->mapy) { return false; }

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

	if (sizex < 1 || sizex > gs->mapx) { return false; }
	if (sizez < 1 || sizez > gs->mapy) { return false; }

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

	if (x >= gs->mapx) { return 0.0f; }
	if (z >= gs->mapy) { return 0.0f; }

	const PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	const float cost = maxResBuf.GetNodeExtraCost(x, z, synced);
	return cost;
}

const float* CPathManager::GetNodeExtraCosts(bool synced) const {
	if (!IsFinalized())
		return NULL;

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

