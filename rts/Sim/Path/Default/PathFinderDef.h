/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDERDEF_HDR
#define PATHFINDERDEF_HDR

#include "System/float3.h"
#include "System/Vec2.h"

struct MoveData;
class CPathFinderDef {
public:
	CPathFinderDef(const float3& goalCenter, float goalRadius);
	virtual ~CPathFinderDef() {}

	virtual bool WithinConstraints(int xSquare, int Square) const { return true; }

	bool IsGoal(int xSquare, int zSquare) const;
	float Heuristic(int xSquare, int zSquare) const;
	bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	int2 GoalSquareOffset(int blockSize) const;

	float3 goal;
	float sqGoalRadius;
	int goalSquareX;
	int goalSquareZ;
};



class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(const float3& start, const float3& goal, float goalRadius, float searchSize, int extraSize);
	~CRangedGoalWithCircularConstraint() {}

	bool WithinConstraints(int xSquare, int zSquare) const;

private:
	int halfWayX;
	int halfWayZ;
	int searchRadiusSq;
};

#endif
