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
	CPathFinderDef(const float3& startPos, const float3& goalPos, float goalRadius, float sqGoalDistance);
	virtual ~CPathFinderDef() {}

	virtual bool WithinConstraints(const int2 square) const { return (WithinConstraints(square.x, square.y)); }
	virtual bool WithinConstraints(uint32_t xSquare, uint32_t zSquare) const = 0;

	void DisableConstraint(bool b) { constraintDisabled = b; }
	void AllowRawPathSearch(bool b) { allowRawPath = b; }
	void AllowDefPathSearch(bool b) { allowDefPath = b; }

	bool IsGoal(uint32_t squareX, uint32_t squareZ) const;
	bool IsGoalBlocked(const MoveDef& moveDef, const CMoveMath::BlockType& blockMask, const CSolidObject* owner) const;

	float Heuristic(uint32_t srcSquareX, uint32_t srcSquareZ, uint32_t tgtSquareX, uint32_t tgtSquareZ, uint32_t blockSize) const;
	float Heuristic(uint32_t srcSquareX, uint32_t srcSquareZ, uint32_t blockSize) const {
		return (Heuristic(srcSquareX, srcSquareZ, goalSquareX, goalSquareZ, blockSize));
	}

	int2 GoalSquareOffset(uint32_t blockSize) const;

public:
	// world-space start and goal positions
	float3 wsStartPos;
	float3 wsGoalPos;

	float sqGoalRadius;
	float maxRawPathLen;
	float minRawSpeedMod;

	// if true, do not need to generate any waypoints
	bool startInGoalRadius;
	bool constraintDisabled;
	bool skipSubSearches;

	bool testMobile;
	bool needPath;
	bool exactPath;
	bool allowRawPath;
	bool allowDefPath;
	bool dirIndependent;
	bool synced;

	// heightmap-coors
	uint32_t startSquareX;
	uint32_t startSquareZ;
	uint32_t goalSquareX;
	uint32_t goalSquareZ;
};



class CCircularSearchConstraint: public CPathFinderDef {
public:
	CCircularSearchConstraint(
		const float3& start = ZeroVector,
		const float3& goal = ZeroVector,
		float goalRadius = 0.0f,
		float searchSize = 0.0f,
		uint32_t extraSize = 0
	);

	// tests if a square is inside is the circular constrained area
	// defined by the start and goal positions (note that this only
	// saves CPU under certain conditions and destroys admissibility)
	bool WithinConstraints(uint32_t xSquare, uint32_t zSquare) const {
		const int dx = halfWayX - xSquare;
		const int dz = halfWayZ - zSquare;

		return (constraintDisabled || ((dx * dx + dz * dz) <= searchRadiusSq));
	}

private:
	uint32_t halfWayX;
	uint32_t halfWayZ;
	uint32_t searchRadiusSq;
};



class CRectangularSearchConstraint: public CPathFinderDef {
public:
	// note: startPos and goalPos are in world-space
	CRectangularSearchConstraint(
		const float3 startPos,
		const float3 goalPos,
		float sqRadius,
		uint32_t blockSize
	);

	bool WithinConstraints(uint32_t xSquare, uint32_t zSquare) const {
		if (startBlockRect.Inside(int2(xSquare, zSquare))) return true;
		if ( goalBlockRect.Inside(int2(xSquare, zSquare))) return true;
		return (constraintDisabled);
	}

private:
	SRectangle startBlockRect;
	SRectangle  goalBlockRect;
};

#endif

