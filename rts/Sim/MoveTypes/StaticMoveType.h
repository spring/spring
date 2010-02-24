/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STATICMOVETYPE_H
#define STATICMOVETYPE_H

#include "MoveType.h"

class CStaticMoveType : public AMoveType
{
	CR_DECLARE(CStaticMoveType);

public:
	CStaticMoveType(CUnit* unit) : AMoveType(unit){};
	virtual void StartMoving(float3 pos, float goalRadius){};
	virtual void StartMoving(float3 pos, float goalRadius, float speed){};
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive){};
	virtual void StopMoving(){};
	virtual void Update(){};
	void SlowUpdate();
};

#endif // STATICMOVETYPE_H
