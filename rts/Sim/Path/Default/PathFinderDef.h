/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PATHFINDERDEF_HDR
#define PATHFINDERDEF_HDR

#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/Rectangle.h"

struct MoveDef;
class CPathFinderDef {
public:
	CPathFinderDef(const float3& goalCenter, float goalRadius, float sqGoalDistance);
	virtual ~CPathFinderDef() {}

	virtual bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const { return true; }
	virtual void DisableConstraint(bool b) { constraintDisabled = b; }

	bool IsGoal(unsigned int xSquare, unsigned int zSquare) const;
	float Heuristic(unsigned int xSquare, unsigned int zSquare) const;
	bool GoalIsBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& blockMask, const CSolidObject* owner) const;
	int2 GoalSquareOffset(unsigned int blockSize) const;


	// world-space goal position
	float3 goal;

	float sqGoalRadius;

	// if true, do not need to generate any waypoints
	bool startInGoalRadius;
	bool constraintDisabled;

	unsigned int goalSquareX;
	unsigned int goalSquareZ;
};



class CCircularSearchConstraint: public CPathFinderDef {
public:
	CCircularSearchConstraint(
		const float3& start,
		const float3& goal,
		float goalRadius,
		float searchSize,
		unsigned int extraSize
	);

	// tests if a square is inside is the circular constrained area
	// defined by the start and goal positions (note that this only
	// saves CPU under certain conditions and destroys admissibility)
	bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const {
		const int dx = halfWayX - xSquare;
		const int dz = halfWayZ - zSquare;

		return (constraintDisabled || ((dx * dx + dz * dz) <= searchRadiusSq));
	}

private:
	unsigned int halfWayX;
	unsigned int halfWayZ;
	unsigned int searchRadiusSq;
};



class CRectangularSearchConstraint: public CPathFinderDef {
public:
	// note: startPos and goalPos are in world-space
	CRectangularSearchConstraint(
		const float3 startPos,
		const float3 goalPos,
		unsigned int blockSize
	);

	bool WithinConstraints(unsigned int xSquare, unsigned int zSquare) const {
		if (parentBlockRect.Inside(int2(xSquare, zSquare))) return true;
		if ( childBlockRect.Inside(int2(xSquare, zSquare))) return true;
		return (constraintDisabled);
	}

private:
	SRectangle parentBlockRect;
	SRectangle  childBlockRect;
};

#endif

