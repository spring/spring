/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "PathManager.h"
#include "PathConstants.h"
#include "PathFinder.h"
#include "PathEstimator.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"

CPathManager* pathManager = NULL;

CPathManager::CPathManager(): nextPathId(0)
{
	maxResPF = new CPathFinder();
	medResPE = new CPathEstimator(maxResPF,  8, "pe",  mapInfo->map.name);
	lowResPE = new CPathEstimator(maxResPF, 32, "pe2", mapInfo->map.name);

	logOutput.Print("[CPathManager] pathing data checksum: %08x\n", GetPathCheckSum());

	#ifdef SYNCDEBUG
	// clients may have a non-writable cache directory (which causes
	// the estimator path-file checksum to remain zero), so we can't
	// update the sync-checker with this in normal builds
	// NOTE: better to just checksum the in-memory data and broadcast
	// that instead of relying on the zip-file CRC?
	{ SyncedUint tmp(GetPathCheckSum()); }
	#endif
}

CPathManager::~CPathManager()
{
	delete lowResPE;
	delete medResPE;
	delete maxResPF;
}



/*
Help-function.
Turns a start->goal-request into a well-defined request.
*/
unsigned int CPathManager::RequestPath(
	const MoveData* moveData,
	const float3& startPos,
	const float3& goalPos,
	float goalRadius,
	CSolidObject* caller,
	bool synced
) {
	float3 sp(startPos); sp.CheckInBounds();
	float3 gp(goalPos); gp.CheckInBounds();

	// Create an estimator definition.
	CRangedGoalWithCircularConstraint* pfDef = new CRangedGoalWithCircularConstraint(sp, gp, goalRadius, 3.0f, 2000);

	// Make request.
	return RequestPath(moveData, sp, gp, pfDef, caller, synced);
}

/*
Request a new multipath, store the result and return a handle-id to it.
*/
unsigned int CPathManager::RequestPath(
	const MoveData* md,
	const float3& startPos,
	const float3& goalPos,
	CPathFinderDef* pfDef,
	CSolidObject* caller,
	bool synced
) {
	SCOPED_TIMER("PFS");

	MoveData* moveData = moveinfo->moveData[md->pathType];
	moveData->tempOwner = caller;

	// Creates a new multipath.
	IPath::SearchResult result = IPath::Error;
	MultiPath* newPath = new MultiPath(startPos, pfDef, moveData);
	newPath->finalGoal = goalPos;
	newPath->caller = caller;

	if (caller) {
		caller->UnBlock();
	}

	const int ownerId = caller? caller->id: 0;
	unsigned int retValue = 0;

	// choose the PF or the PE depending on the goal-distance
	const float distanceToGoal = pfDef->Heuristic(int(startPos.x / SQUARE_SIZE), int(startPos.z / SQUARE_SIZE));

	if (distanceToGoal < DETAILED_DISTANCE) {
		result = maxResPF->GetPath(*moveData, startPos, *pfDef, newPath->maxResPath, true, false, MAX_SEARCHED_NODES_PF >> 2, true, ownerId, synced);

		pfDef->DisableConstraint(true);

		// fallback (note: uses PE, unconstrained PF queries are too expensive on average)
		if (result != IPath::Ok) {
			result = medResPE->GetPath(*moveData, startPos, *pfDef, newPath->medResPath, MAX_SEARCHED_NODES_PE, synced);
			// note: we don't need to clear newPath->maxResPath from the previous
			// maxResPF->GetPath() call (the med-to-max conversion takes care of it)
			MedRes2MaxRes(*newPath, startPos, ownerId, synced);
		}

		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			retValue = Store(newPath);
		} else {
			delete newPath;
		}
	} else if (distanceToGoal < ESTIMATE_DISTANCE) {
		result = medResPE->GetPath(*moveData, startPos, *pfDef, newPath->medResPath, MAX_SEARCHED_NODES_PE, synced);

		pfDef->DisableConstraint(true);

		// fallback
		if (result != IPath::Ok) {
			result = medResPE->GetPath(*moveData, startPos, *pfDef, newPath->medResPath, MAX_SEARCHED_NODES_PE, synced);
		}

		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			MedRes2MaxRes(*newPath, startPos, ownerId, synced);
			retValue = Store(newPath);
		} else {
			delete newPath;
		}
	} else {
		result = lowResPE->GetPath(*moveData, startPos, *pfDef, newPath->lowResPath, MAX_SEARCHED_NODES_PE, synced);

		pfDef->DisableConstraint(true);

		// fallback
		if (result != IPath::Ok) {
			result = lowResPE->GetPath(*moveData, startPos, *pfDef, newPath->lowResPath, MAX_SEARCHED_NODES_PE, synced);
		}

		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			LowRes2MedRes(*newPath, startPos, ownerId, synced);
			MedRes2MaxRes(*newPath, startPos, ownerId, synced);

			retValue = Store(newPath);
		} else {
			delete newPath;
		}
	}

	if (caller) {
		caller->Block();
	}

	moveData->tempOwner = NULL;
	return retValue;
}


/*
Store a new multipath into the pathmap.
*/
unsigned int CPathManager::Store(MultiPath* path)
{
	pathMap[++nextPathId] = path;
	return nextPathId;
}


// converts part of a med-res path into a high-res path
void CPathManager::MedRes2MaxRes(MultiPath& multiPath, const float3& startPos, int ownerId, bool synced) const
{
	IPath::Path& maxResPath = multiPath.maxResPath;
	IPath::Path& medResPath = multiPath.medResPath;
	IPath::Path& lowResPath = multiPath.lowResPath;

	if (medResPath.path.empty())
		return;

	medResPath.path.pop_back();

	// Remove estimate waypoints until
	// the next one is far enough.
	while (!medResPath.path.empty() &&
		medResPath.path.back().SqDistance2D(startPos) < Square(DETAILED_DISTANCE * SQUARE_SIZE))
		medResPath.path.pop_back();

	// get the goal of the detailed search
	float3 goalPos;

	if (medResPath.path.empty()) {
		goalPos = medResPath.pathGoal;
	} else {
		goalPos = medResPath.path.back();
	}

	// define the search
	CRangedGoalWithCircularConstraint rangedGoalPFD(startPos, goalPos, 0.0f, 2.0f, 1000);

	// Perform the search.
	// If this is the final improvement of the path, then use the original goal.
	IPath::SearchResult result = IPath::Error;

	if (medResPath.path.empty() && lowResPath.path.empty()) {
		result = maxResPF->GetPath(*multiPath.moveData, startPos, *multiPath.peDef, maxResPath, true, false, MAX_SEARCHED_NODES_PF >> 2, true, ownerId, synced);
	} else {
		result = maxResPF->GetPath(*multiPath.moveData, startPos, rangedGoalPFD, maxResPath, true, false, MAX_SEARCHED_NODES_PF >> 2, true, ownerId, synced);
	}

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		maxResPath.pathGoal = goalPos;
	}
}

// converts part of a low-res path into a med-res path
void CPathManager::LowRes2MedRes(MultiPath& multiPath, const float3& startPos, int ownerId, bool synced) const
{
	IPath::Path& medResPath = multiPath.medResPath;
	IPath::Path& lowResPath = multiPath.lowResPath;

	if (lowResPath.path.empty())
		return;

	lowResPath.path.pop_back();

	// Remove estimate2 waypoints until
	// the next one is far enough
	while (!lowResPath.path.empty() &&
		lowResPath.path.back().SqDistance2D(startPos) < Square(ESTIMATE_DISTANCE * SQUARE_SIZE)) {
		lowResPath.path.pop_back();
	}

	//Get the goal of the detailed search.
	float3 goalPos;

	if (lowResPath.path.empty()) {
		goalPos = lowResPath.pathGoal;
	} else {
		goalPos = lowResPath.path.back();
	}

	// define the search
	CRangedGoalWithCircularConstraint rangedGoal(startPos, goalPos, 0.0f, 2.0f, 20);

	// Perform the search.
	// If there is no estimate2 path left, use original goal.
	IPath::SearchResult result = IPath::Error;

	if (lowResPath.path.empty()) {
		result = medResPE->GetPath(*multiPath.moveData, startPos, *multiPath.peDef, medResPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	} else {
		result = medResPE->GetPath(*multiPath.moveData, startPos, rangedGoal, medResPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	}

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		medResPath.pathGoal = goalPos;
	}
}


/*
Removes and return the next waypoint in the multipath corresponding to given id.
*/
float3 CPathManager::NextWaypoint(
	unsigned int pathId,
	float3 callerPos,
	float minDistance,
	int numRetries,
	int ownerId,
	bool synced
) const {
	SCOPED_TIMER("PFS");

	// 0 indicates a no-path id
	if (pathId == 0)
		return float3(-1.0f, -1.0f, -1.0f);

	if (numRetries > 4)
		return float3(-1.0f, -1.0f, -1.0f);

	// Find corresponding multipath.
	const std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);

	if (pi == pathMap.end())
		return float3(-1.0f, -1.0f, -1.0f);

	MultiPath* multiPath = pi->second;

	if (callerPos == ZeroVector) {
		if (!multiPath->maxResPath.path.empty())
			callerPos = multiPath->maxResPath.path.back();
	}

	// check if detailed path needs bettering
	if (!multiPath->medResPath.path.empty() &&
		(multiPath->medResPath.path.back().SqDistance2D(callerPos) < Square(MIN_DETAILED_DISTANCE * SQUARE_SIZE) ||
		multiPath->maxResPath.path.size() <= 2)) {

		if (!multiPath->lowResPath.path.empty() &&  // if so, check if estimated path also needs bettering
			(multiPath->lowResPath.path.back().SqDistance2D(callerPos) < Square(MIN_ESTIMATE_DISTANCE * SQUARE_SIZE) ||
			multiPath->medResPath.path.size() <= 2)) {

			LowRes2MedRes(*multiPath, callerPos, ownerId, synced);
		}

		if (multiPath->caller) {
			multiPath->caller->UnBlock();
		}

		MedRes2MaxRes(*multiPath, callerPos, ownerId, synced);

		if (multiPath->caller) {
			multiPath->caller->Block();
		}
	}

	float3 waypoint;
	do {
		// get the next waypoint from the high-res path
		//
		// if this is not possible, then either we are
		// at the goal OR the path could not reach all
		// the way to it (ie. a GoalOutOfRange result)
		if (multiPath->maxResPath.path.empty()) {
			if (multiPath->lowResPath.path.empty() && multiPath->medResPath.path.empty()) {
				if (multiPath->searchResult == IPath::Ok) {
					return multiPath->finalGoal;
				} else {
					return float3(-1.0f, -1.0f, -1.0f);
				}
			} else {
				return NextWaypoint(pathId, callerPos, minDistance, numRetries + 1, ownerId, synced);
			}
		} else {
			waypoint = multiPath->maxResPath.path.back();
			multiPath->maxResPath.path.pop_back();
		}
	} while (callerPos.SqDistance2D(waypoint) < Square(minDistance) && waypoint != multiPath->maxResPath.pathGoal);

	return waypoint;
}


// Delete a given multipath from the collection.
void CPathManager::DeletePath(unsigned int pathId) {
	// 0 indicate a no-path id.
	if (pathId == 0)
		return;

	const std::map<unsigned int, MultiPath*>::iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end())
		return;

	const MultiPath* multiPath = pi->second;

	pathMap.erase(pathId);
	delete multiPath;
}



// Tells estimators about changes in or on the map.
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
	medResPE->MapChanged(x1, z1, x2, z2);
	lowResPE->MapChanged(x1, z1, x2, z2);
}



void CPathManager::Update()
{
	SCOPED_TIMER("PFS Update");
	maxResPF->UpdateHeatMap();
	medResPE->Update();
	lowResPE->Update();
}

// used to deposit heat on the heat-map as a unit moves along its path
void CPathManager::UpdatePath(const CSolidObject* owner, unsigned int pathId)
{
	if (!pathId) {
		return;
	}

#ifndef USE_GML
	static std::vector<int2> points;
#else
	std::vector<int2> points;
#endif

	GetDetailedPathSquares(pathId, points);

	float scale = 1.0f / points.size();
	unsigned int i = points.size();

	for (std::vector<int2>::const_iterator it = points.begin(); it != points.end(); ++it) {
		SetHeatOnSquare(it->x, it->y, i * scale * owner->mobility->heatProduced, owner->id); i--;
	}
}



void CPathManager::SetHeatMappingEnabled(bool enabled) { maxResPF->SetHeatMapState(enabled); }
bool CPathManager::GetHeatMappingEnabled() { return maxResPF->GetHeatMapState(); }

void CPathManager::SetHeatOnSquare(int x, int y, int value, int ownerId) { maxResPF->UpdateHeatValue(x, y, value, ownerId); }
const int CPathManager::GetHeatOnSquare(int x, int y) { return maxResPF->GetHeatValue(x, y); }



// get the waypoints in world-coordinates
void CPathManager::GetDetailedPath(unsigned pathId, std::vector<float3>& points) const
{
	points.clear();

	const std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}

	const MultiPath* multiPath = pi->second;
	const IPath::path_list_type& maxResPoints = multiPath->maxResPath.path;

	points.reserve(maxResPoints.size());

	for (IPath::path_list_type::const_reverse_iterator pvi = maxResPoints.rbegin(); pvi != maxResPoints.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}

void CPathManager::GetDetailedPathSquares(unsigned pathId, std::vector<int2>& points) const
{
	points.clear();

	const std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}

	const MultiPath* multiPath = pi->second;
	const IPath::square_list_type& maxResSquares = multiPath->maxResPath.squares;

	points.reserve(maxResSquares.size());

	for (IPath::square_list_type::const_reverse_iterator pvi = maxResSquares.rbegin(); pvi != maxResSquares.rend(); ++pvi) {
		points.push_back(*pvi);
	}
}



void CPathManager::GetEstimatedPath(
	unsigned int pathId,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	points.clear();
	starts.clear();

	const std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}

	const MultiPath* multiPath = pi->second;
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
	return (medResPE->GetPathChecksum() + lowResPE->GetPathChecksum());
}



bool CPathManager::SetNodeExtraCost(unsigned int x, unsigned int z, float cost, bool synced) {
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
	if (sizex < 1 || sizex > gs->mapx) { return false; }
	if (sizez < 1 || sizez > gs->mapy) { return false; }

	PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	PathNodeStateBuffer& medResBuf = medResPE->GetNodeStateBuffer();
	PathNodeStateBuffer& lowResBuf = lowResPE->GetNodeStateBuffer();

	maxResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	medResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	lowResBuf.SetNodeExtraCosts(costs, sizex, sizez, synced);
	return true;
}

float CPathManager::GetNodeExtraCost(unsigned int x, unsigned int z, bool synced) const {
	if (x >= gs->mapx) { return 0.0f; }
	if (z >= gs->mapy) { return 0.0f; }

	PathNodeStateBuffer& maxResBuf = maxResPF->GetNodeStateBuffer();
	return (maxResBuf.GetNodeExtraCost(x, z, synced));
}

const float* CPathManager::GetNodeExtraCosts(bool synced) const {
	const PathNodeStateBuffer& buf = maxResPF->GetNodeStateBuffer();
	return (buf.GetNodeExtraCosts(synced));
}
