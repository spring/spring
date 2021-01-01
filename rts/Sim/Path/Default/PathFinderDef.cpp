/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "PathFinderDef.h"
#include "PathConstants.h"
#include "Sim/MoveTypes/MoveDefHandler.h"

constexpr float MAX_RAW_PATH_LEN = 1.8e19;  //math::sqrt(std::numeric_limits<float>::max())

CPathFinderDef::CPathFinderDef(const float3& startPos, const float3& goalPos, float goalRadius, float sqGoalDistance)
: wsStartPos(startPos)
, wsGoalPos(goalPos)

, sqGoalRadius(goalRadius * goalRadius)
, maxRawPathLen(MAX_RAW_PATH_LEN)
, minRawSpeedMod(0.0f)

, constraintDisabled(false)
, skipSubSearches(false)

, testMobile(true)
, needPath(true)
// if true, units will not even try to move if their max-res
// PF goal is inside (or surrounded by) an impassable region
// if false, units will get as close as possible and may end
// up stuck or flip-flop between waypoints from backtracking
//
// false mirrors the PE behavior when requesting (not while
// regenerating after a terrain change) paths, so prefer to
// keep PF and PE in sync
, exactPath(false)
, allowRawPath(false)
, allowDefPath(true)
, dirIndependent(false)
, synced(true)
{
	startSquareX = wsStartPos.x / SQUARE_SIZE;
	startSquareZ = wsStartPos.z / SQUARE_SIZE;
	goalSquareX = wsGoalPos.x / SQUARE_SIZE;
	goalSquareZ = wsGoalPos.z / SQUARE_SIZE;

	// make sure that the goal can be reached with 2-square resolution
	sqGoalRadius = std::max(sqGoalRadius, SQUARE_SIZE * SQUARE_SIZE * 2.0f);
	startInGoalRadius = (sqGoalRadius >= sqGoalDistance);
}

// returns true when the goal is within our defined range
bool CPathFinderDef::IsGoal(uint32_t squareX, uint32_t squareZ) const {
	return (SquareToFloat3(squareX, squareZ).SqDistance2D(wsGoalPos) <= sqGoalRadius);
}

// returns distance to goal center in heightmap-squares
float CPathFinderDef::Heuristic(
	uint32_t srcSquareX,
	uint32_t srcSquareZ,
	uint32_t tgtSquareX,
	uint32_t tgtSquareZ,
	uint32_t blockSize
) const {
	(void) blockSize;

	const float dx = std::abs(int(srcSquareX) - int(tgtSquareX));
	const float dz = std::abs(int(srcSquareZ) - int(tgtSquareZ));

	// grid is 8-connected, so use octile distance metric
	constexpr const float C1 = (1.0f    / PATH_NODE_SPACING);
	constexpr const float C2 = (1.4142f / PATH_NODE_SPACING) - (2.0f * C1);
	return ((dx + dz) * C1 + std::min(dx, dz) * C2);
}


// returns if the goal is inaccessable: this is
// true if the goal area is "small" and blocked
bool CPathFinderDef::IsGoalBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& blockMask, const CSolidObject* owner) const {
	const float r0 = SQUARE_SIZE * SQUARE_SIZE * 4.0f; // same as (SQUARE_SIZE*2)^2
	const float r1 = ((moveDef.xsize * SQUARE_SIZE) >> 1) * ((moveDef.zsize * SQUARE_SIZE) >> 1) * 1.5f;

	if (sqGoalRadius >= r0 && sqGoalRadius > r1)
		return false;

	return ((CMoveMath::IsBlocked(moveDef, wsGoalPos, owner) & blockMask) != 0);
}

int2 CPathFinderDef::GoalSquareOffset(uint32_t blockSize) const {
	const uint32_t blockPixelSize = blockSize * SQUARE_SIZE;

	int2 offset;
		offset.x = (unsigned(wsGoalPos.x) % blockPixelSize) / SQUARE_SIZE;
		offset.y = (unsigned(wsGoalPos.z) % blockPixelSize) / SQUARE_SIZE;

	return offset;
}






CCircularSearchConstraint::CCircularSearchConstraint(
	const float3& start,
	const float3& goal,
	float goalRadius,
	float searchSize,
	uint32_t extraSize
): CPathFinderDef(start, goal, goalRadius, start.SqDistance2D(goal))
{
	// calculate the center and radius of the constrained area
	const uint32_t startX = start.x / SQUARE_SIZE;
	const uint32_t startZ = start.z / SQUARE_SIZE;

	const float3 halfWay = (start + goal) * 0.5f;

	halfWayX = halfWay.x / SQUARE_SIZE;
	halfWayZ = halfWay.z / SQUARE_SIZE;

	const int dx = startX - halfWayX;
	const int dz = startZ - halfWayZ;

	searchRadiusSq  = dx * dx + dz * dz;
	searchRadiusSq *= (searchSize * searchSize);
	searchRadiusSq += extraSize;
}



CRectangularSearchConstraint::CRectangularSearchConstraint(
	const float3 startPos,
	const float3 goalPos,
	float sqRadius,
	uint32_t blockSize
): CPathFinderDef(startPos, goalPos, 0.0f, startPos.SqDistance2D(goalPos))
{
	sqGoalRadius = std::max(sqRadius, sqGoalRadius);

	// construct the rectangular areas containing {start,goal}Pos
	// (nodes are constrained to these when a PE uses the max-res
	// PF to cache costs)
	uint32_t startBlockX = startPos.x / SQUARE_SIZE;
	uint32_t startBlockZ = startPos.z / SQUARE_SIZE;
	uint32_t  goalBlockX =  goalPos.x / SQUARE_SIZE;
	uint32_t  goalBlockZ =  goalPos.z / SQUARE_SIZE;

	// align to PE-grid
	startBlockX -= (startBlockX % blockSize);
	startBlockZ -= (startBlockZ % blockSize);
	 goalBlockX -= ( goalBlockX % blockSize);
	 goalBlockZ -= ( goalBlockZ % blockSize);

	startBlockRect.x1 = startBlockX;
	startBlockRect.z1 = startBlockZ;
	startBlockRect.x2 = startBlockX + blockSize;
	startBlockRect.z2 = startBlockZ + blockSize;

	goalBlockRect.x1 = goalBlockX;
	goalBlockRect.z1 = goalBlockZ;
	goalBlockRect.x2 = goalBlockX + blockSize;
	goalBlockRect.z2 = goalBlockZ + blockSize;
}

