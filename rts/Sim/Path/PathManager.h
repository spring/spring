/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHMANAGER_H
#define PATHMANAGER_H

#include <map>
#include "IPath.h"
#include "PathFinder.h"
#include <boost/cstdint.hpp> /* Replace with <stdint.h> if appropriate */

class CSolidObject;
class CPathEstimator;
class CPathFinderDef;
struct MoveData;
class CMoveMath;

class CPathManager {
public:
	CPathManager();
	~CPathManager();

	/*
	Generate a path from startPos to the target defined by either peDef or (goalPos, goalRadius).
	If no complete path from startPos to the path target could be found, then a path getting as
	"close" as possible to target is generated.
	Only if no path getting "closer" to the target could be found no path is created.
	If a path could be created, then a none-zero path-id is returned.
	If no path could be created, then 0 is returned as a indication of a failure.
	Param:
		moveData
			Defining the footprint to use the path.

		startPos
			The starting location of the requested path.

		peDef
			A CPathEstimatorDef-object defining the target of the search.

		goalPos
			The center of the path target area.

		goalRadius
			Use goalRadius to define a goal area within any square could be accepted as path target.
			If a singular goal position is wanted, then use goalRadius = 0.
	*/
	unsigned int RequestPath(
		const MoveData* moveData,
		float3 startPos, float3 goalPos, float goalRadius = 8,
		CSolidObject* caller = 0,
		bool synced = true
	);

	unsigned int RequestPath(
		const MoveData* moveData,
		float3 startPos,
		CPathFinderDef* peDef,
		float3 goalPos,
		CSolidObject* caller,
		bool synced = true
	);


	/*
	Gives the next waypoint of the path.
	Gives (-1,-1,-1) in case no new waypoint could be found.
	Param:
		pathId
			The path-id returned by RequestPath.

		callerPos
			The current position of the user of the path.
			This extra information is needed to keep the path connected to it's user.

		minDistance
			Could be used to set a minimum required distance between callerPos and
			the returned waypoint.
		numRetries
			Dont set this, used internally

	*/
	float3 NextWaypoint(unsigned int pathId, float3 callerPos, float minDistance = 0,
			int numRetries=0, int ownerId = 0, bool synced = true) const;


	/**
	Returns current detail path waypoints. For the full path, @see GetEstimatedPath.

	Param:
		pathId
			The path-id returned by RequestPath.
		points
			The list of detail waypoints.
	*/
	void GetDetailedPath(unsigned pathId, std::vector<float3>& points) const;


	/**
	Returns current detail path waypoints as a square list. For the full path, @see GetEstimatedPath.

	Param:
		pathId
			The path-id returned by RequestPath.
		points
			The list of detail waypoints.
	*/
	void GetDetailedPathSquares(unsigned pathId, std::vector<int2>& points) const;


	/*
	Returns current estimated waypoints sorted by estimation levels
	Param:
		pathId
			The path-id returned by RequestPath.
		points
			The list of estimated waypoints.
		starts
			The list of starting indices for the different estimation levels
	*/
	void GetEstimatedPath(unsigned int pathId,
		std::vector<float3>& points, std::vector<int>& starts) const;


	/*
	When a path are no longer used, please call this function to release it from memory.
	Param:
		pathId
			The path-id returned by RequestPath.
	*/
	void DeletePath(unsigned int pathId);


	/*
	Whenever there are any changes in the terrain (ex. explosions, new buildings, etc.)
	this function need to be called to keep the estimator a jour.
	Param:
		upperCorner, lowerCorner
			The two corners defining the rectangular area affected by the changes.

		x1, z1, x2, z2
			Square coordinates defining the rectangular area affected by the changes.
	*/
	void TerrainChange(float3 upperCorner, float3 lowerCorner);
	void TerrainChange(unsigned int x1, unsigned int z1, unsigned int x2, unsigned int z2);


	/*
	Shall be called every 1/30sec during runtime.
	*/
	void Update();



	boost::uint32_t GetPathCheckSum();

	/** Enable/disable heat mapping */
	void SetHeatMappingEnabled(bool enabled);
	bool GetHeatMappingEnabled();

	void SetHeatOnSquare(int x, int y, int value, int ownerId);
	void SetHeatOnPos(float3, int value, int ownerId);

	const int GetHeatOnSquare(int x, int y) { return pf->GetHeatValue(x, y); };

	//Minimum distance between two waypoints.
	static const unsigned int PATH_RESOLUTION;

private:
	struct MultiPath {
		MultiPath(const float3 start, const CPathFinderDef* peDef, const MoveData* moveData);
		~MultiPath();

		//Paths
		IPath::Path estimatedPath2;
		IPath::Path estimatedPath;
		IPath::Path detailedPath;

		//Request definition
		const float3 start;
		const CPathFinderDef* peDef;
		const MoveData* moveData;

		//Additional information.
		float3 finalGoal;
		CSolidObject* caller;
	};


	unsigned int Store(MultiPath* path);
	void Estimate2ToEstimate(MultiPath& path, float3 startPos, int ownerId, bool synced) const;
	void EstimateToDetailed(MultiPath& path, float3 startPos, int ownerId) const;


	CPathFinder* pf;
	CPathEstimator* pe;
	CPathEstimator* pe2;

	std::map<unsigned int, MultiPath*> pathMap;
	unsigned int nextPathId;
};

extern CPathManager *pathManager;

#endif
