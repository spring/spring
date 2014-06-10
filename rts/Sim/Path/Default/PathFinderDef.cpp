/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstdlib>

#include "PathFinderDef.h"
#include "Map/ReadMap.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Misc/GlobalSynced.h"

CPathFinderDef::CPathFinderDef(const float3& goalCenter, float goalRadius, float sqGoalDistance):
goal(goalCenter),
sqGoalRadius(goalRadius * goalRadius),
constraintDisabled(false)
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
	return (std::max(dx, dz) * 0.5f + std::min(dx, dz) * 0.2f);
}


// returns if the goal is inaccessable: this is
// true if the goal area is "small" and blocked
bool CPathFinderDef::GoalIsBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& blockMask, const CSolidObject* owner) const {
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
	unsigned int blockSize
): CPathFinderDef(goalPos, 0.0f, startPos.SqDistance2D(goalPos))
{
	const unsigned int parentBlockX = startPos.x / (SQUARE_SIZE * blockSize);
	const unsigned int parentBlockZ = startPos.z / (SQUARE_SIZE * blockSize);
	const unsigned int  childBlockX =  goalPos.x / (SQUARE_SIZE * blockSize);
	const unsigned int  childBlockZ =  goalPos.z / (SQUARE_SIZE * blockSize);

	parentBlockRect.x1 = parentBlockX * blockSize;
	parentBlockRect.z1 = parentBlockZ * blockSize;
	parentBlockRect.x2 = parentBlockX * blockSize + blockSize;
	parentBlockRect.z2 = parentBlockZ * blockSize + blockSize;

	childBlockRect.x1 = childBlockX * blockSize;
	childBlockRect.z1 = childBlockZ * blockSize;
	childBlockRect.x2 = childBlockX * blockSize + blockSize;
	childBlockRect.z2 = childBlockZ * blockSize + blockSize;
}

