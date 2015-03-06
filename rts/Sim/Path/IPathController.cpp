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
	const int targetSpeedSign = Sign(int(!wantReverse));
	const int currentSpeedSign = Sign(int(!isReversing));

	const float rawSpeedDiff = (targetSpeed * targetSpeedSign) - (currentSpeed * currentSpeedSign);
	const float absSpeedDiff = math::fabs(rawSpeedDiff);
	// need to clamp, game-supplied values can be much larger than |speedDiff|
	const float modAccRate = std::min(absSpeedDiff, maxAccRate);
	const float modDecRate = std::min(absSpeedDiff, maxDecRate);

	const float deltaSpeed = mix(modAccRate, -modDecRate, (rawSpeedDiff < 0.0f));

	// no acceleration changes if not on ground
	//
	// note: not 100% correct, depends on MoveDef::speedModClass
	// ships should always use (1 - IsInWater()), hovercraft
	// should use (1 - IsInWater()) but only when over water,
	// tanks should also test if IsInWater() && !IsOnGround()
	return (deltaSpeed * (1 - owner->IsInAir()));
}

#if 1
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
	return (deltaHeading * (1 - owner->IsInAir()));
}
#endif



static float TurnAccelerationSign(float turnBrakeDist, short curDeltaHeading, short newDeltaHeading) {
	const bool b0 = (turnBrakeDist >= std::abs(curDeltaHeading));
	const bool b1 = (std::abs(newDeltaHeading) <= std::abs(curDeltaHeading));
	const bool b2 = (Sign(curDeltaHeading) != Sign(newDeltaHeading));

	return (mix(1.0f, -1.0f, b0 && (b1 || b2)));
}

short GMTDefaultPathController::GetDeltaHeading(
	unsigned int pathID,
	short newHeading,
	short oldHeading,
	float maxTurnSpeed,
	float maxTurnAccel,
	float turnBrakeDist,
	float* curTurnSpeed
) const {
	// negative --> RH turn, positive --> LH turn
	// add lookahead term to avoid micro-overshoots
	const short curDeltaHeading = newHeading - short(oldHeading + (*curTurnSpeed) * (maxTurnAccel / maxTurnSpeed));

	const float minTurnAccel = std::min(float(std::abs(curDeltaHeading)), maxTurnAccel);
	const float rawTurnAccel = Clamp(Sign(curDeltaHeading) * maxTurnAccel, -minTurnAccel, minTurnAccel);
	const float newTurnSpeed = Clamp((*curTurnSpeed) + rawTurnAccel * (1 - owner->IsInAir()), -maxTurnSpeed, maxTurnSpeed);

	// predict the new angular difference
	const short newDeltaHeading = newHeading - short(oldHeading + newTurnSpeed);

	// flip acceleration sign when overshooting
	const float modTurnAccel = rawTurnAccel * TurnAccelerationSign(turnBrakeDist, curDeltaHeading, newDeltaHeading);

	(*curTurnSpeed) += (modTurnAccel * (1 - owner->IsInAir()));
	(*curTurnSpeed) = Clamp((*curTurnSpeed) * 0.99f, -maxTurnSpeed, maxTurnSpeed);

	return (*curTurnSpeed);
}

bool GMTDefaultPathController::IgnoreTerrain(const MoveDef& md, const float3& pos) const {
	return (owner->IsInAir());
}

