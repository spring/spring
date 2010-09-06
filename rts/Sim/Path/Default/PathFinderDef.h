/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDERDEF_HDR
#define PATHFINDERDEF_HDR

class CPathFinderDef {
public:
	CPathFinderDef(float3 goalCenter, float goalRadius);
	~CPathFinderDef();

	bool IsGoal(int xSquare, int zSquare) const;
	float Heuristic(int xSquare, int zSquare) const;
	bool GoalIsBlocked(const MoveData& moveData, unsigned int moveMathOptions) const;
	virtual bool WithinConstraints(int xSquare, int Square) const {return true;}
	int2 GoalSquareOffset(int blockSize) const;

	float3 goal;
	float sqGoalRadius;
	int goalSquareX;
	int goalSquareZ;
};

class CRangedGoalWithCircularConstraint : public CPathFinderDef {
public:
	CRangedGoalWithCircularConstraint(float3 start, float3 goal, float goalRadius, float searchSize, int extraSize);
	virtual bool WithinConstraints(int xSquare, int zSquare) const;
	virtual ~CRangedGoalWithCircularConstraint();

private:
	int halfWayX;
	int halfWayZ;
	int searchRadiusSq;
};

#endif
