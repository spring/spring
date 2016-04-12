/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "PathFinderDef.h"
#include "Sim/MoveTypes/MoveDefHandler.h"


CPathFinderDef::CPathFinderDef(const float3& goalCenter, float goalRadius, float sqGoalDistance)
: goal(goalCenter)
, sqGoalRadius(goalRadius * goalRadius)
, constraintDisabled(false)
, testMobile(true)
, needPath(true)
, exactPath(true)
, dirIndependent(false)
, synced(true)
{
	goalSquareX = goalCenter.x / SQUARE_SIZE;
	goalSquareZ = goalCenter.z / SQUARE_SIZE;

	// make sure that the goal can be reached with 2-square resolution
	sqGoalRadius = std::max(sqGoalRadius, SQUARE_SIZE * SQUARE_SIZE * 2.0f);
	startInGoalRadius = (sqGoalRadius >= sqGoalDistance);
}

// returns true when the goal is within our defined range
bool CPathFinderDef::IsGoal(unsigned int xSquare, unsigned int zSquare) const {
	return (SquareToFloat3(xSquare, zSquare).SqDistance2D(goal) <= sqGoalRadius);
}

// returns distance to goal center in heightmap-squares
float CPathFinderDef::Heuristic(unsigned int xSquare, unsigned int zSquare) const
{
	const float dx = std::abs(int(xSquare) - int(goalSquareX));
	const float dz = std::abs(int(zSquare) - int(goalSquareZ));

	// grid is 8-connected, so use octile distance
	constexpr const float C1 = 1.0f;
	constexpr const float C2 = 1.4142f - (2.0f * C1);
	return ((dx + dz) * C1 + std::min(dx, dz) * C2);
}


// returns if the goal is inaccessable: this is
// true if the goal area is "small" and blocked
bool CPathFinderDef::IsGoalBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& blockMask, const CSolidObject* owner) const {
	const float r0 = SQUARE_SIZE * SQUARE_SIZE * 4.0f; // same as (SQUARE_SIZE*2)^2
	const float r1 = ((moveDef.xsize * SQUARE_SIZE) >> 1) * ((moveDef.zsize * SQUARE_SIZE) >> 1) * 1.5f;

	if (sqGoalRadius >= r0 && sqGoalRadius > r1)
		return false;

	return ((CMoveMath::IsBlocked(moveDef, goal, owner) & blockMask) != 0);
}

int2 CPathFinderDef::GoalSquareOffset(unsigned int blockSize) const {
	const unsigned int blockPixelSize = blockSize * SQUARE_SIZE;

	int2 offset;
		offset.x = (unsigned(goal.x) % blockPixelSize) / SQUARE_SIZE;
		offset.y = (unsigned(goal.z) % blockPixelSize) / SQUARE_SIZE;

	return offset;
}






CCircularSearchConstraint::CCircularSearchConstraint(
	const float3& start,
	const float3& goal,
	float goalRadius,
	float searchSize,
	unsigned int extraSize
): CPathFinderDef(goal, goalRadius, start.SqDistance2D(goal))
{
	// calculate the center and radius of the constrained area
	const unsigned int startX = start.x / SQUARE_SIZE;
	const unsigned int startZ = start.z / SQUARE_SIZE;

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
	unsigned int blockSize
): CPathFinderDef(goalPos, 0.0f, startPos.SqDistance2D(goalPos))
{
	sqGoalRadius = std::max(sqRadius, sqGoalRadius);

	// construct the rectangular areas containing {start,goal}Pos
	// (nodes are constrained to these when a PE uses the max-res
	// PF to cache costs)
	unsigned int startBlockX = startPos.x / SQUARE_SIZE;
	unsigned int startBlockZ = startPos.z / SQUARE_SIZE;
	unsigned int  goalBlockX =  goalPos.x / SQUARE_SIZE;
	unsigned int  goalBlockZ =  goalPos.z / SQUARE_SIZE;
	startBlockX -= startBlockX % blockSize;
	startBlockZ -= startBlockZ % blockSize;
	 goalBlockX -=  goalBlockX % blockSize;
	 goalBlockZ -=  goalBlockZ % blockSize;

	startBlockRect.x1 = startBlockX;
	startBlockRect.z1 = startBlockZ;
	startBlockRect.x2 = startBlockX + blockSize;
	startBlockRect.z2 = startBlockZ + blockSize;

	goalBlockRect.x1 = goalBlockX;
	goalBlockRect.z1 = goalBlockZ;
	goalBlockRect.x2 = goalBlockX + blockSize;
	goalBlockRect.z2 = goalBlockZ + blockSize;
}

