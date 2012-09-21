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

	virtual bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const { return true; }
	virtual void DisableConstraint(bool) {}

	bool IsGoal(unsigned int xSquare, unsigned int zSquare) const;
	float Heuristic(unsigned int xSquare, unsigned int zSquare) const;
	bool GoalIsBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& moveMathOptions, const CSolidObject* owner) const;
	int2 GoalSquareOffset(unsigned int blockSize) const;

	float3 goal;
	float sqGoalRadius;

	bool startInGoalRadius;

	unsigned int goalSquareX;
	unsigned int goalSquareZ;
};



class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(const float3& start, const float3& goal, float goalRadius, float searchSize, unsigned int extraSize);
	~CRangedGoalWithCircularConstraint() {}

	bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const;
	void DisableConstraint(bool b) { disabled = b; }

private:
	unsigned int halfWayX;
	unsigned int halfWayZ;
	unsigned int searchRadiusSq;

	bool disabled;
};

#endif
