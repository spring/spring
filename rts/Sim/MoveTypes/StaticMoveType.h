/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STATICMOVETYPE_H
#define STATICMOVETYPE_H

#include "MoveType.h"

class CStaticMoveType : public AMoveType
{
	CR_DECLARE_DERIVED(CStaticMoveType)

public:
	CStaticMoveType(CUnit* unit) : AMoveType(unit) {
		useWantedSpeed[false] = false;
		useWantedSpeed[ true] = false;
	}

	void StartMoving(float3 pos, float goalRadius) override {}
	void StartMoving(float3 pos, float goalRadius, float speed) override {}
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override {}

	void SetMaxSpeed(float speed) override { /* override AMoveType (our maxSpeed IS allowed to be 0) */ }
	void KeepPointingTo(float3 pos, float distance, bool aggressive) override {}

	bool Update() override { return false; }
	void SlowUpdate() override;
};

#endif // STATICMOVETYPE_H
