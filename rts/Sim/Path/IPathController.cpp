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
	float deltaSpeed = 0.0f;

	const int targetSpeedSign = int(!wantReverse) * 2 - 1;
	const int currentSpeedSign = int(!isReversing) * 2 - 1;

	const float rawSpeedDiff = (targetSpeed * targetSpeedSign) - (currentSpeed * currentSpeedSign);
	const float absSpeedDiff = math::fabs(rawSpeedDiff);
	// need to clamp, game-supplied values can be much larger than |speedDiff|
	const float modAccRate = std::min(absSpeedDiff, maxAccRate);
	const float modDecRate = std::min(absSpeedDiff, maxDecRate);

	if (isReversing) {
		// speed-sign in GMT::UpdateOwnerPos is negative
		//   --> to go faster in reverse gear, we need to add +decRate
		//   --> to go slower in reverse gear, we need to add -accRate
		deltaSpeed = (rawSpeedDiff < 0.0f)?  modDecRate: -modAccRate;
	} else {
		// speed-sign in GMT::UpdateOwnerPos is positive
		//   --> to go faster in forward gear, we need to add +accRate
		//   --> to go slower in forward gear, we need to add -decRate
		deltaSpeed = (rawSpeedDiff < 0.0f)? -modDecRate:  modAccRate;
	}

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

