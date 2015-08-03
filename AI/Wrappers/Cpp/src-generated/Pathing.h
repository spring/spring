/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_PATHING_H
#define _CPPWRAPPER_PATHING_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Pathing {

public:
	virtual ~Pathing(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * The following functions allow the AI to use the built-in path-finder.
	 * 
	 * - call InitPath and you get a pathId back
	 * - use this to call GetNextWaypoint to get subsequent waypoints;
	 *   the waypoints are centered on 8*8 squares
	 * - note that the pathfinder calculates the waypoints as needed,
	 *   so do not retrieve them until they are needed
	 * - the waypoint's x and z coordinates are returned in x and z,
	 *   while y is used for status codes:
	 *   y =  0: legal path waypoint IFF x >= 0 and z >= 0
	 *   y = -1: temporary waypoint, path not yet available
	 * - for pathType, @see UnitDef_MoveData_getPathType()
	 * - goalRadius defines a goal area within which any square could be accepted as
	 *   path target. If a singular goal position is wanted, use 0.0f.
	 *   default: 8.0f
	 * @param start_posF3  The starting location of the requested path
	 * @param end_posF3  The goal location of the requested path
	 * @param pathType  For what type of unit should the path be calculated
	 * @param goalRadius  default: 8.0f
	 */
public:
	virtual int InitPath(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) = 0;

	/**
	 * Returns the approximate path cost between two points.
	 * - for pathType @see UnitDef_MoveData_getPathType()
	 * - goalRadius defines a goal area within which any square could be accepted as
	 *   path target. If a singular goal position is wanted, use 0.0f.
	 *   default: 8.0f
	 * @param start_posF3  The starting location of the requested path
	 * @param end_posF3  The goal location of the requested path
	 * @param pathType  For what type of unit should the path be calculated
	 * @param goalRadius  default: 8.0f
	 */
public:
	virtual float GetApproximateLength(const springai::AIFloat3& start, const springai::AIFloat3& end, int pathType, float goalRadius) = 0;

public:
	virtual springai::AIFloat3 GetNextWaypoint(int pathId) = 0;

public:
	virtual void FreePath(int pathId) = 0;

}; // class Pathing

}  // namespace springai

#endif // _CPPWRAPPER_PATHING_H

