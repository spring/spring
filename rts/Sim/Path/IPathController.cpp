/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "IPathController.hpp"
#include "Sim/Units/Unit.h"
#include "System/myMath.h"

IPathController* IPathController::GetInstance(CUnit* owner) {
	return (new GMTDefaultPathController(owner));
}

void IPathController::FreeInstance(IPathController* instance) {
	delete instance;
}



float GMTDefaultPathController::GetDeltaSpeed(
	unsigned int pathID,
	float targetSpeed,
	float currentSpeed,
	float maxAccRate,
	float maxDecRate,
	bool wantReverse,
	bool isReversing
) const {
	const int targetSpeedSign = int(!wantReverse) * 2 - 1;
	const int currentSpeedSign = int(!isReversing) * 2 - 1;

	const float rawSpeedDiff = (targetSpeed * targetSpeedSign) - (currentSpeed * currentSpeedSign);
	const float absSpeedDiff = math::fabs(rawSpeedDiff);
	// need to clamp, game-supplied values can be much larger than |speedDiff|
	const float modAccRate = std::min(absSpeedDiff, maxAccRate);
	const float modDecRate = std::min(absSpeedDiff, maxDecRate);

	const float deltaSpeed = (rawSpeedDiff < 0.0f)? -modDecRate: modAccRate;

	// no acceleration changes if not on ground
	return (deltaSpeed * (1 - int(owner->inAir)));
}

short GMTDefaultPathController::GetDeltaHeading(
	unsigned int pathID,
	short newHeading,
	short oldHeading,
	float maxTurnRate
) const {
	short deltaHeading = newHeading - oldHeading;

	if (deltaHeading > 0) {
		deltaHeading = std::min(deltaHeading, short( maxTurnRate));
	} else {
		deltaHeading = std::max(deltaHeading, short(-maxTurnRate));
	}

	// no orientation changes if not on ground
	return (deltaHeading * (1 - int(owner->inAir)));
}

bool GMTDefaultPathController::IgnoreTerrain(const MoveDef& md, const float3& pos) const {
	return (owner->inAir);
}

