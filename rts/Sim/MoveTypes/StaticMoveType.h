/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STATICMOVETYPE_H
#define STATICMOVETYPE_H

#include "MoveType.h"

class CStaticMoveType : public AMoveType
{
	CR_DECLARE(CStaticMoveType);

public:
	CStaticMoveType(CUnit* unit) : AMoveType(unit) {}
	void StartMoving(float3 pos, float goalRadius) {}
	void StartMoving(float3 pos, float goalRadius, float speed) {}
	void KeepPointingTo(float3 pos, float distance, bool aggressive) {}
	void StopMoving() {}
	bool Update() { return false; }
	void SlowUpdate();
};

#endif // STATICMOVETYPE_H
