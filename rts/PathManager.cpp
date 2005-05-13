#include "PathManager.h"
#include "InfoConsole.h"
#include "MoveInfo.h"
#include "GroundMoveMath.h"
#include "HoverMoveMath.h"
#include "ShipMoveMath.h"
#include "myGL.h"
#include "TimeProfiler.h"
#include <vector>

const float ESTIMATE_DISTANCE = 90;
const float MIN_ESTIMATE_DISTANCE = 65;
const float DETAILED_DISTANCE = 60;
const float MIN_DETAILED_DISTANCE = 40;
const unsigned int MAX_SEARCHED_NODES_ON_REFINE = 2000;
const int FRAMES_TO_KEEP_PATH = 9000;	//9000frames == 5min

CPathManager* pathManager;


/*
Constructor
Creates pathfinder and estimators.
*/
CPathManager::CPathManager() {
	//--TODO: Move to creation of MoveData!--//
	//Create MoveMaths.
	ground = new CGroundMoveMath();
	hover = new CHoverMoveMath();
	sea = new CShipMoveMath();

	//Address movemath and pathtype to movedata.
	int counter = 0;
	std::vector<MoveData*>::iterator mi;
	for(mi = moveinfo->moveData.begin(); mi < moveinfo->moveData.end(); mi++) {
		(*mi)->pathType = counter;
//		(*mi)->crushStrength = 0;
		switch((*mi)->moveType) {
			case MoveData::Ground_Move:
				(*mi)->moveMath = ground;
				break;
			case MoveData::Hover_Move:
				(*mi)->moveMath = hover;
				break;
			case MoveData::Ship_Move:
				(*mi)->moveMath = sea;
				break;
		}
		counter++;
	}
	//---------------------------------------//

	//Create pathfinder and estimators.
	pf = new CPathFinder();
	pe = new CPathEstimator(pf, 8, CMoveMath::CHECK_STRUCTURES, "pe");
	pe2 = new CPathEstimator(pf, 32, CMoveMath::CHECK_STRUCTURES, "pe2");

	//Reset id-counter.
	nextPathId = 0;
}


/*
Destructor
Free used memory.
*/
CPathManager::~CPathManager() {
	delete pe2;
	delete pe;
	delete pf;

	delete ground;
	delete hover;
	delete sea;
}


/*
Help-function.
Turns a start->goal-request into a will defined request.
*/
unsigned int CPathManager::RequestPath(const MoveData* moveData, float3 startPos, float3 goalPos, float goalRadius,CSolidObject* caller) {
	startPos.CheckInBounds();
	goalPos.CheckInBounds();
	if(startPos.z>gs->mapx*SQUARE_SIZE-5)
		startPos.z=gs->mapx*SQUARE_SIZE-5;
	if(goalPos.z>gs->mapx*SQUARE_SIZE-5)
		goalPos.z=gs->mapx*SQUARE_SIZE-5;

	//Create a estimator definition.
	CRangedGoalPED* rangedGoalPED = new CRangedGoalPED(goalPos, goalRadius);

	//Make request.
	return RequestPath(moveData, startPos, rangedGoalPED,goalPos,caller);
}


/*
Request a new multipath, store the result and return an handle-id to it.
*/
unsigned int CPathManager::RequestPath(const MoveData* moveData, float3 startPos, CPathEstimatorDef* peDef,float3 goalPos,CSolidObject* caller) {
//	static int calls = 0;
//	*info << "RequestPath() called: " << (++calls) << "\n";	//Debug

	START_TIME_PROFILE;

	//Creates a new multipath.
	MultiPath* newPath = new MultiPath(startPos, peDef, moveData);
	newPath->finalGoal=goalPos;
	newPath->caller=caller;

	if(caller)
		caller->UnBlock();

	unsigned int retValue=0;
	//Choose finder dependent on distance to goal.
	float distanceToGoal = peDef->Heuristic(int(startPos.x / SQUARE_SIZE), int(startPos.z / SQUARE_SIZE));
	if(distanceToGoal < DETAILED_DISTANCE) {
		//Get a detailed path.
		IPath::SearchResult result = pf->GetPath(*moveData, startPos, *peDef, newPath->detailedPath);
		if(result == IPath::Ok || result == IPath::GoalOutOfRange) {
			retValue=Store(newPath);
		} else {
			delete newPath;
		}
	} else if(distanceToGoal < ESTIMATE_DISTANCE) {
		//Get an estimate path.
		IPath::SearchResult result = pe->GetPath(*moveData, startPos, *peDef, newPath->estimatedPath);
		if(result == IPath::Ok || result == IPath::GoalOutOfRange) {
			//Turn a part of it into detailed path.
			EstimateToDetailed(*newPath, startPos);
			//Store the path.
			retValue=Store(newPath);
		} else {
			delete newPath;
		}
	} else {
		//Get a low-res. estimate path.
		IPath::SearchResult result = pe2->GetPath(*moveData, startPos, *peDef, newPath->estimatedPath2);
		if(result == IPath::Ok || result == IPath::GoalOutOfRange) {
			//Turn a part of it into hi-res. estimate path.
			Estimate2ToEstimate(*newPath, startPos);
			//And estimate into detailed.
			EstimateToDetailed(*newPath, startPos);
			//Store the path.
			retValue=Store(newPath);
		} else {
			delete newPath;
		}
	}
	if(caller)
		caller->Block();
	END_TIME_PROFILE("AI:PFS");
	return retValue;
}


/*
Store a new multipath into the pathmap.
*/
unsigned int CPathManager::Store(MultiPath* path) {
	//Store the end of the path as the final goal.
/*	if(path->estimatedPath2.path.size() > 0)
		path->finalGoal = path->estimatedPath2.path.front();
	else if(path->estimatedPath.path.size() > 0)
		path->finalGoal = path->estimatedPath.path.front();
	else
		path->finalGoal = path->detailedPath.path.front();
*/
	//Set time to live.
	path->timeToLive = gs->frameNum + FRAMES_TO_KEEP_PATH;

	//Store the path.
	pathMap[++nextPathId] = path;
	return nextPathId;
}


/*
Turns a part of the estimate path into detailed path.
*/
void CPathManager::EstimateToDetailed(MultiPath& path, float3 startPos) {
	//If there is no estimate path, nothing could be done.
	if(path.estimatedPath.path.empty())
		return;

	//Remove estimate waypoints until
	//the next one is far enought.
	while(!path.estimatedPath.path.empty()
	&& path.estimatedPath.path.back().distance2D(startPos) < DETAILED_DISTANCE * SQUARE_SIZE)
		path.estimatedPath.path.pop_back();

	//Get the goal of the detailed search.
	float3 goalPos;
	if(path.estimatedPath.path.empty())
		goalPos = path.estimatedPath.pathGoal;
	else
		goalPos = path.estimatedPath.path.back();

	//Define the search.
	CRangedGoalPFD rangedGoalPFD(goalPos, 0);

	//Perform the search.
	//If this is the final improvement of the path, then use the original goal.
	IPath::SearchResult result;
	if(path.estimatedPath.path.empty() && path.estimatedPath2.path.empty())
		result = pf->GetPath(*path.moveData, startPos, *path.peDef, path.detailedPath, CMoveMath::CHECK_STRUCTURES | CMoveMath::CHECK_MOBILE, false, MAX_SEARCHED_NODES_ON_REFINE);
	else
		result = pf->GetPath(*path.moveData, startPos, rangedGoalPFD, path.detailedPath, CMoveMath::CHECK_STRUCTURES | CMoveMath::CHECK_MOBILE, false, MAX_SEARCHED_NODES_ON_REFINE);

	//If no refined path could be found, set goal as desired goal.
	if(result == IPath::CantGetCloser || result == IPath::Error) {
		path.detailedPath.pathGoal = goalPos;
	}
}


/*
Turns a part of the estimate2 path into estimate path.
*/
void CPathManager::Estimate2ToEstimate(MultiPath& path, float3 startPos) {
	//If there is no estimate2 path, nothing could be done.
	if(path.estimatedPath2.path.empty())
		return;

	//Remove estimate2 waypoints until
	//the next one is far enought.
	while(!path.estimatedPath2.path.empty()
	&& path.estimatedPath2.path.back().distance2D(startPos) < ESTIMATE_DISTANCE * SQUARE_SIZE)
		path.estimatedPath2.path.pop_back();

	//Get the goal of the detailed search.
	float3 goalPos;
	if(path.estimatedPath2.path.empty())
		goalPos = path.estimatedPath2.pathGoal;
	else
		goalPos = path.estimatedPath2.path.back();

	//Define the search.
	CRangedGoalPED rangedGoalPED(goalPos, 0);

	//Perform the search.
	//If there is no estimate2 path left, use original goal.
	IPath::SearchResult result;
	if(path.estimatedPath2.path.empty())
		result = pe->GetPath(*path.moveData, startPos, *path.peDef, path.estimatedPath, MAX_SEARCHED_NODES_ON_REFINE);
	else {
		result = pe->GetPath(*path.moveData, startPos, rangedGoalPED, path.estimatedPath, MAX_SEARCHED_NODES_ON_REFINE);
	}

	//If no refined path could be found, set goal as desired goal.
	if(result == IPath::CantGetCloser || result == IPath::Error) {
		path.estimatedPath.pathGoal = goalPos;
	}
}


/*
Removes and return the next waypoint in the multipath corresponding to given id.
*/
float3 CPathManager::NextWaypoint(unsigned int pathId, float3 callerPos, float minDistance) {
	#ifdef PROFILE_TIME
		LARGE_INTEGER starttime;
		QueryPerformanceCounter(&starttime);
	#endif

	//0 indicate a no-path id.
	if(pathId == 0)
		return float3(-1,-1,-1);

	//Find corresponding multipath.
	map<unsigned int, MultiPath*>::iterator pi = pathMap.find(pathId);
	if(pi == pathMap.end())
		return float3(-1,-1,-1);
	MultiPath* multiPath = pi->second;
	
	//Check if increased resolution is needed.
	if(!multiPath->estimatedPath2.path.empty()
	&& (multiPath->estimatedPath2.path.back().distance2D(callerPos) < MIN_ESTIMATE_DISTANCE * SQUARE_SIZE
	|| multiPath->estimatedPath.path.size() <= 2))
		Estimate2ToEstimate(*multiPath, callerPos);
	if(!multiPath->estimatedPath.path.empty()
	&& (multiPath->estimatedPath.path.back().distance2D(callerPos) < MIN_DETAILED_DISTANCE * SQUARE_SIZE
	|| multiPath->detailedPath.path.size() <= 2)){
		if(multiPath->caller)
			multiPath->caller->UnBlock();
		EstimateToDetailed(*multiPath, callerPos);
		if(multiPath->caller)
			multiPath->caller->Block();
	}

	//Repeat until a waypoint distant enought are found.
	float3 waypoint;
	do {
		//Get next waypoint.
		if(multiPath->detailedPath.path.empty()) {
			if(multiPath->estimatedPath2.path.empty() && multiPath->estimatedPath.path.empty())
				return multiPath->finalGoal;
			else 
				return float3(-1,-1,-1);
		} else {
			waypoint = multiPath->detailedPath.path.back();
			multiPath->detailedPath.path.pop_back();
		}
	} while(callerPos.distance2D(waypoint) < minDistance && waypoint != multiPath->detailedPath.pathGoal);

	//Keep path alive.
	multiPath->timeToLive = gs->frameNum + FRAMES_TO_KEEP_PATH;

	//Return the result.
	#ifdef PROFILE_TIME
		LARGE_INTEGER stop;
		QueryPerformanceCounter(&stop);
#ifndef NO_WINSTUFF		
		profiler.AddTime("AI:PFS",stop.QuadPart - starttime.QuadPart);
#else
		profiler.AddTime("AI:PFS",stop - starttime);
#endif
	
	#endif
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
	map<unsigned int, MultiPath*>::iterator pi = pathMap.find(pathId);
	if(pi == pathMap.end())
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
	TerrainChange(upperCorner.x / SQUARE_SIZE, upperCorner.z / SQUARE_SIZE, lowerCorner.x / SQUARE_SIZE, lowerCorner.z / SQUARE_SIZE);
}


/*
Tells estimators about changes in or on the map.
*/
void CPathManager::TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2) {
//	*info << "Terrain changed: (" << int(x1) << int(z1) << int(x2) << int(z2) << "\n";	//Debug
	pe->MapChanged(x1, z1, x2, z2);
	pe2->MapChanged(x1, z1, x2, z2);
}


/*
Runned every 1/30sec during runtime.
*/
void CPathManager::Update() {
START_TIME_PROFILE;
	pe->Update();
	pe2->Update();

	//Check for timed out paths.
	list<unsigned int> toBeDeleted;
	if(gs->frameNum % 30 == 0) {
		map<unsigned int, MultiPath*>::iterator mi;
		for(mi = pathMap.begin(); mi != pathMap.end(); mi++)
			if(mi->second->timeToLive <= gs->frameNum) {
				toBeDeleted.push_back(mi->first);
			}
	}
	//Delete timed out paths.
	list<unsigned int>::iterator li;
	for(li = toBeDeleted.begin(); li != toBeDeleted.end(); li++)
		DeletePath(*li);

END_TIME_PROFILE("AI:PFS:Update");
}


/*
Draw path-lines for each path in pathmap.
*/
void CPathManager::Draw() {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	map<unsigned int, MultiPath*>::iterator pi;
	for(pi = pathMap.begin(); pi != pathMap.end(); pi++) {
		MultiPath* path = pi->second;
		list<float3>::iterator pvi;
		
		//Start drawing a line.
		glBegin(GL_LINE_STRIP);

		//Drawing estimatePath2.
		glColor4f(0,0,1,1);
		for(pvi = path->estimatedPath2.path.begin(); pvi != path->estimatedPath2.path.end(); pvi++) {
			float3 pos = *pvi;
			pos.y += 5;
			glVertexf3(pos);
		}

		//Drawing estimatePath.
		glColor4f(0,1,0,1);
		for(pvi = path->estimatedPath.path.begin(); pvi != path->estimatedPath.path.end(); pvi++) {
			float3 pos = *pvi;
			pos.y += 5;
			glVertexf3(pos);
		}

		//Drawing detailPath
		glColor4f(1,0,0,1);
		for(pvi = path->detailedPath.path.begin(); pvi != path->detailedPath.path.end(); pvi++) {
			float3 pos = *pvi;
			pos.y += 5;
			glVertexf3(pos);
		}

		//Stop drawing a line.
		glEnd();

		//Tell the CPathFinderDef to visualise it self.
		path->peDef->Draw();
	}
}
