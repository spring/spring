/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDERDEF_HDR
#define PATHFINDERDEF_HDR

#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/float3.h"
#include "System/Vec2.h"

struct MoveDef;
class CPathFinderDef {
public:
	CPathFinderDef(const float3& goalCenter, float goalRadius, float sqGoalDistance);
	virtual ~CPathFinderDef() {}

	virtual bool WithinConstraints(int xSquare, int Square) const { return true; }
	virtual void DisableConstraint(bool) {}

	bool IsGoal(int xSquare, int zSquare) const;
	float Heuristic(int xSquare, int zSquare) const;
	bool GoalIsBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& moveMathOptions, const CSolidObject *owner) const;
	int2 GoalSquareOffset(int blockSize) const;

	float3 goal;
	float sqGoalRadius;
	bool startInGoalRadius;
	int goalSquareX;
	int goalSquareZ;
};



class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(const float3& start, const float3& goal, float goalRadius, float searchSize, int extraSize);
	~CRangedGoalWithCircularConstraint() {}

	bool WithinConstraints(int xSquare, int zSquare) const;
	void DisableConstraint(bool b) { disabled = b; }

private:
	bool disabled;

	int halfWayX;
	int halfWayZ;
	int searchRadiusSq;
};

#endif
