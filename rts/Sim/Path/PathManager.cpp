/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "PathManager.h"

#include <vector>
#include <boost/cstdint.hpp>

#include "PathFinder.h"
#include "PathEstimator.h"
#include "Map/MapInfo.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/LogOutput.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"

const float ESTIMATE_DISTANCE = 55;
const float MIN_ESTIMATE_DISTANCE = 40;
const float DETAILED_DISTANCE = 25;
const float MIN_DETAILED_DISTANCE = 12;
const unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;
const unsigned int CPathManager::PATH_RESOLUTION = CPathFinder::PATH_RESOLUTION;



CPathManager* pathManager = 0;

CPathManager::CPathManager()
{
	// Create pathfinder and estimators.
	pf  = new CPathFinder();
	pe  = new CPathEstimator(pf,  8, CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN, "pe", mapInfo->map.name);
	pe2 = new CPathEstimator(pf, 32, CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN, "pe2", mapInfo->map.name);

	// Reset id-counter.
	nextPathId = 0;

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
	delete pe2;
	delete pe;
	delete pf;
}



/*
Help-function.
Turns a start->goal-request into a well-defined request.
*/
unsigned int CPathManager::RequestPath(const MoveData* moveData, float3 startPos,
		float3 goalPos, float goalRadius, CSolidObject* caller, bool synced) {
	startPos.CheckInBounds();
	goalPos.CheckInBounds();

	if (startPos.x > gs->mapx * SQUARE_SIZE - 5) { startPos.x = gs->mapx * SQUARE_SIZE - 5; }
	if (goalPos.z > gs->mapy * SQUARE_SIZE - 5) { goalPos.z = gs->mapy * SQUARE_SIZE - 5; }

	// Create an estimator definition.
	CRangedGoalWithCircularConstraint* rangedGoalPED = new CRangedGoalWithCircularConstraint(startPos,goalPos, goalRadius, 3, 2000);

	// Make request.
	return RequestPath(moveData, startPos, rangedGoalPED, goalPos, caller, synced);
}


/*
Request a new multipath, store the result and return a handle-id to it.
*/
unsigned int CPathManager::RequestPath(const MoveData* md, float3 startPos,
		CPathFinderDef* peDef, float3 goalPos, CSolidObject* caller, bool synced) {
	SCOPED_TIMER("PFS");

	MoveData* moveData = moveinfo->moveData[md->pathType];
	moveData->tempOwner = caller;

	// Creates a new multipath.
	MultiPath* newPath = new MultiPath(startPos, peDef, moveData);
	newPath->finalGoal = goalPos;
	newPath->caller = caller;

	if (caller) {
		caller->UnBlock();
	}

	const int ownerId = caller? caller->id: 0;
	unsigned int retValue = 0;
	// Choose finder dependent on distance to goal.
	float distanceToGoal = peDef->Heuristic(int(startPos.x / SQUARE_SIZE), int(startPos.z / SQUARE_SIZE));

	if (distanceToGoal < DETAILED_DISTANCE) {
		// Get a detailed path.
		IPath::SearchResult result =
			pf->GetPath(*moveData, startPos, *peDef, newPath->detailedPath, true, false, CPathFinder::MAX_SEARCHED_SQUARES, true, ownerId);
		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			retValue = Store(newPath);
		} else {
			delete newPath;
		}
	} else if (distanceToGoal < ESTIMATE_DISTANCE) {
		// Get an estimate path.
		IPath::SearchResult result = pe->GetPath(*moveData, startPos, *peDef, newPath->estimatedPath, CPathEstimator::MAX_SEARCHED_BLOCKS, synced);
		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			// Turn a part of it into detailed path.
			EstimateToDetailed(*newPath, startPos, ownerId);
			// Store the path.
			retValue = Store(newPath);
		} else {
			// if we fail see if it can work find a better block to start from
			float3 sp = pe->FindBestBlockCenter(moveData, startPos);

			if (sp.x != 0 &&
				(((int) sp.x) / (SQUARE_SIZE * 8) != ((int) startPos.x) / (SQUARE_SIZE * 8) ||
				((int) sp.z) / (SQUARE_SIZE * 8) != ((int) startPos.z) / (SQUARE_SIZE * 8))) {
				IPath::SearchResult result = pe->GetPath(*moveData, sp, *peDef, newPath->estimatedPath, CPathEstimator::MAX_SEARCHED_BLOCKS, synced);
				newPath->searchResult = result;

				if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
					EstimateToDetailed(*newPath, startPos, ownerId);
					retValue = Store(newPath);
				} else {
					delete newPath;
				}
			} else {
				delete newPath;
			}
		}
	} else {
		// Get a low-res. estimate path.
		IPath::SearchResult result = pe2->GetPath(*moveData, startPos, *peDef, newPath->estimatedPath2, CPathEstimator::MAX_SEARCHED_BLOCKS, synced);
		newPath->searchResult = result;

		if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
			// Turn a part of it into hi-res. estimate path.
			Estimate2ToEstimate(*newPath, startPos, ownerId, synced);
			// And estimate into detailed.
			EstimateToDetailed(*newPath, startPos, ownerId);
			// Store the path.
			retValue = Store(newPath);
		} else {
			// sometimes the 32*32 squares can be wrong so if it fails to get a path also try with 8*8 squares
			IPath::SearchResult result = pe->GetPath(*moveData, startPos, *peDef, newPath->estimatedPath, CPathEstimator::MAX_SEARCHED_BLOCKS, synced);
			newPath->searchResult = result;

			if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
				EstimateToDetailed(*newPath, startPos, ownerId);
				retValue = Store(newPath);
			} else {
				// 8*8 can also fail rarely, so see if we can find a better 8*8 to start from
				float3 sp = pe->FindBestBlockCenter(moveData, startPos);

				if (sp.x != 0 &&
					(((int) sp.x) / (SQUARE_SIZE * 8) != ((int) startPos.x) / (SQUARE_SIZE * 8) ||
					((int) sp.z) / (SQUARE_SIZE * 8) != ((int) startPos.z) / (SQUARE_SIZE * 8))) {
					IPath::SearchResult result = pe->GetPath(*moveData, sp, *peDef, newPath->estimatedPath, CPathEstimator::MAX_SEARCHED_BLOCKS, synced);

					if (result == IPath::Ok || result == IPath::GoalOutOfRange) {
						EstimateToDetailed(*newPath, startPos, ownerId);
						retValue = Store(newPath);
					} else {
						delete newPath;
					}
				} else {
					delete newPath;
				}
			}
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
	//Store the path.
	pathMap[++nextPathId] = path;
	return nextPathId;
}


/*
Turns a part of the estimate path into detailed path.
*/
void CPathManager::EstimateToDetailed(MultiPath& path, float3 startPos, int ownerId) const
{
	//If there is no estimate path, nothing could be done.
	if(path.estimatedPath.path.empty())
		return;

	path.estimatedPath.path.pop_back();
	//Remove estimate waypoints until
	//the next one is far enought.
	while(!path.estimatedPath.path.empty()
	&& path.estimatedPath.path.back().SqDistance2D(startPos) < Square(DETAILED_DISTANCE * SQUARE_SIZE))
		path.estimatedPath.path.pop_back();

	//Get the goal of the detailed search.
	float3 goalPos;
	if(path.estimatedPath.path.empty())
		goalPos = path.estimatedPath.pathGoal;
	else
		goalPos = path.estimatedPath.path.back();

	//Define the search.
	CRangedGoalWithCircularConstraint rangedGoalPFD(startPos,goalPos, 0,2,1000);

	//Perform the search.
	//If this is the final improvement of the path, then use the original goal.
	IPath::SearchResult result;
	if (path.estimatedPath.path.empty() && path.estimatedPath2.path.empty())
		result = pf->GetPath(*path.moveData, startPos, *path.peDef, path.detailedPath, true, false, CPathFinder::MAX_SEARCHED_SQUARES, true, ownerId);
	else
		result = pf->GetPath(*path.moveData, startPos, rangedGoalPFD, path.detailedPath, true, false, CPathFinder::MAX_SEARCHED_SQUARES, true, ownerId);

	//If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		path.detailedPath.pathGoal = goalPos;
	}
}


/*
Turns a part of the estimate2 path into estimate path.
*/
void CPathManager::Estimate2ToEstimate(MultiPath& path, float3 startPos, int ownerId, bool synced) const
{
	//If there is no estimate2 path, nothing could be done.
	if (path.estimatedPath2.path.empty())
		return;

	path.estimatedPath2.path.pop_back();
	//Remove estimate2 waypoints until
	//the next one is far enought.
	while (!path.estimatedPath2.path.empty() &&
		path.estimatedPath2.path.back().SqDistance2D(startPos) < Square(ESTIMATE_DISTANCE * SQUARE_SIZE)) {
		path.estimatedPath2.path.pop_back();
	}

	//Get the goal of the detailed search.
	float3 goalPos;
	if (path.estimatedPath2.path.empty())
		goalPos = path.estimatedPath2.pathGoal;
	else
		goalPos = path.estimatedPath2.path.back();

	//Define the search.
	CRangedGoalWithCircularConstraint rangedGoal(startPos,goalPos, 0,2,20);

	// Perform the search.
	// If there is no estimate2 path left, use original goal.
	IPath::SearchResult result;
	if (path.estimatedPath2.path.empty())
		result = pe->GetPath(*path.moveData, startPos, *path.peDef, path.estimatedPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	else {
		result = pe->GetPath(*path.moveData, startPos, rangedGoal, path.estimatedPath, MAX_SEARCHED_NODES_ON_REFINE, synced);
	}

	// If no refined path could be found, set goal as desired goal.
	if (result == IPath::CantGetCloser || result == IPath::Error) {
		path.estimatedPath.pathGoal = goalPos;
	}
}


/*
Removes and return the next waypoint in the multipath corresponding to given id.
*/
float3 CPathManager::NextWaypoint(unsigned int pathId, float3 callerPos, float minDistance,
		int numRetries, int ownerId, bool synced) const
{
	SCOPED_TIMER("PFS");

	// 0 indicates a no-path id
	if (pathId == 0)
		return float3(-1.0f, -1.0f, -1.0f);

	if (numRetries > 4)
		return float3(-1.0f, -1.0f, -1.0f);

	//Find corresponding multipath.
	std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end())
		return float3(-1.0f, -1.0f, -1.0f);
	MultiPath* multiPath = pi->second;

	if (callerPos == ZeroVector) {
		if (!multiPath->detailedPath.path.empty())
			callerPos = multiPath->detailedPath.path.back();
	}

	// check if detailed path needs bettering
	if (!multiPath->estimatedPath.path.empty() &&
		(multiPath->estimatedPath.path.back().SqDistance2D(callerPos) < Square(MIN_DETAILED_DISTANCE * SQUARE_SIZE) ||
		multiPath->detailedPath.path.size() <= 2)) {

		if (!multiPath->estimatedPath2.path.empty() &&  // if so, check if estimated path also needs bettering
			(multiPath->estimatedPath2.path.back().SqDistance2D(callerPos) < Square(MIN_ESTIMATE_DISTANCE * SQUARE_SIZE) ||
			multiPath->estimatedPath.path.size() <= 2)) {

			Estimate2ToEstimate(*multiPath, callerPos, ownerId, synced);
		}

		if (multiPath->caller) {
			multiPath->caller->UnBlock();
		}

		EstimateToDetailed(*multiPath, callerPos, ownerId);

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
		if (multiPath->detailedPath.path.empty()) {
			if (multiPath->estimatedPath2.path.empty() && multiPath->estimatedPath.path.empty()) {
				if (multiPath->searchResult == IPath::Ok) {
					return multiPath->finalGoal;
				} else {
					return float3(-1.0f, -1.0f, -1.0f);
				}
			} else {
				return NextWaypoint(pathId, callerPos, minDistance, numRetries + 1, ownerId, synced);
			}
		} else {
			waypoint = multiPath->detailedPath.path.back();
			multiPath->detailedPath.path.pop_back();
		}
	} while (callerPos.SqDistance2D(waypoint) < Square(minDistance) && waypoint != multiPath->detailedPath.pathGoal);

	return waypoint;
}


/*
Delete a given multipath from the collection.
*/
void CPathManager::DeletePath(unsigned int pathId) {
	//0 indicate a no-path id.
	if(pathId == 0)
		return;

	//Find the multipath.
	std::map<unsigned int, MultiPath*>::iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end())
		return;
	MultiPath* multiPath = pi->second;

	//Erase and delete the multipath.
	pathMap.erase(pathId);
	delete multiPath;
}


/*
Convert a 2xfloat3-defined rectangle into a square-based rectangle.
*/
void CPathManager::TerrainChange(float3 upperCorner, float3 lowerCorner) {
	TerrainChange((unsigned int)(upperCorner.x / SQUARE_SIZE), (unsigned int)(upperCorner.z / SQUARE_SIZE), (unsigned int)(lowerCorner.x / SQUARE_SIZE), (int)(lowerCorner.z / SQUARE_SIZE));
}


/*
Tells estimators about changes in or on the map.
*/
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
	pe->MapChanged(x1, z1, x2, z2);
	pe2->MapChanged(x1, z1, x2, z2);
}



void CPathManager::Update()
{
	SCOPED_TIMER("PFS Update");
	pf->UpdateHeatMap();
	pe->Update();
	pe2->Update();
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



void CPathManager::SetHeatMappingEnabled(bool enabled) { pf->SetHeatMapState(enabled); }
bool CPathManager::GetHeatMappingEnabled() { return pf->GetHeatMapState(); }

void CPathManager::SetHeatOnSquare(int x, int y, int value, int ownerId) { pf->UpdateHeatValue(x, y, value, ownerId); }
const int CPathManager::GetHeatOnSquare(int x, int y) { return pf->GetHeatValue(x, y); }



void CPathManager::GetDetailedPath(unsigned pathId, std::vector<float3>& points) const
{
	points.clear();

	std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}

	const MultiPath* path = pi->second;
	points.reserve(path->detailedPath.path.size());

	IPath::path_list_type::const_reverse_iterator pvi;

	const IPath::path_list_type& dtlPoints = path->detailedPath.path;
	for (pvi = dtlPoints.rbegin(); pvi != dtlPoints.rend(); pvi++) {
		points.push_back(*pvi);
	}
}


void CPathManager::GetDetailedPathSquares(unsigned pathId, std::vector<int2>& points) const
{
	points.clear();

	std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}

	const MultiPath* path = pi->second;
	points.reserve(path->detailedPath.path.size());

	IPath::square_list_type::const_reverse_iterator pvi;

	const IPath::square_list_type& dtlPoints = path->detailedPath.squares;
	for (pvi = dtlPoints.rbegin(); pvi != dtlPoints.rend(); pvi++) {
		points.push_back(*pvi);
	}
}


void CPathManager::GetEstimatedPath(unsigned int pathId,
	std::vector<float3>& points,
	std::vector<int>& starts) const
{
	points.clear();
	starts.clear();

	std::map<unsigned int, MultiPath*>::const_iterator pi = pathMap.find(pathId);
	if (pi == pathMap.end()) {
		return;
	}
	const MultiPath* path = pi->second;
	points.reserve(path->detailedPath.path.size()
			+ path->estimatedPath.path.size()
			+ path->estimatedPath2.path.size());
	starts.reserve(3);

	IPath::path_list_type::const_reverse_iterator pvi;

	starts.push_back(points.size());
	const IPath::path_list_type& dtlPoints = path->detailedPath.path;
	for (pvi = dtlPoints.rbegin(); pvi != dtlPoints.rend(); pvi++) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());
	const IPath::path_list_type& estPoints = path->estimatedPath.path;
	for (pvi = estPoints.rbegin(); pvi != estPoints.rend(); pvi++) {
		points.push_back(*pvi);
	}

	starts.push_back(points.size());
	const IPath::path_list_type& est2Points = path->estimatedPath2.path;
	for (pvi = est2Points.rbegin(); pvi != est2Points.rend(); pvi++) {
		points.push_back(*pvi);
	}
}



boost::uint32_t CPathManager::GetPathCheckSum()
{
	return (pe->GetPathChecksum() + pe2->GetPathChecksum());
}


CPathManager::MultiPath::MultiPath(const float3 start, const CPathFinderDef* peDef, const MoveData* moveData) :
	start(start),
	peDef(peDef),
	moveData(moveData)
{
}

CPathManager::MultiPath::~MultiPath()
{
	delete peDef;
}
