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

	virtual bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const { return true; }

	bool IsGoal(unsigned int xSquare, unsigned int zSquare) const;
	float Heuristic(unsigned int xSquare, unsigned int zSquare) const;
	bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	int2 GoalSquareOffset(unsigned int blockSize) const;

	float3 goal;
	float sqGoalRadius;
	unsigned int goalSquareX;
	unsigned int goalSquareZ;
};



class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(const float3& start, const float3& goal, float goalRadius, float searchSize, unsigned int extraSize);
	~CRangedGoalWithCircularConstraint() {}

	bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const;

private:
	unsigned int halfWayX;
	unsigned int halfWayZ;
	unsigned int searchRadiusSq;
};

#endif
