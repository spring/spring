/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "IPathController.hpp"
#include "Sim/Units/Unit.h"
#include "System/SpringMath.h"

float GMTDefaultPathController::GetDeltaSpeed(
	unsigned int pathID,
	float targetSpeed, // unsigned
	float currentSpeed, // unsigned
	float maxAccRate, // unsigned
	float maxDecRate, // unsigned
	bool wantReverse,
	bool isReversing
) const {
	// Sign(0) is negative which we do not want
	const int  targetSpeedSign = Sign(int(!wantReverse) * 2 - 1);
	const int currentSpeedSign = Sign(int(!isReversing) * 2 - 1);

	// if reversing, target and current are swapped
	const float speeds[] = {targetSpeed * targetSpeedSign, currentSpeed * currentSpeedSign};

	// all possible cases
	//   tar=[6,+1] && cur=[4,+1] --> dif=+2, delta= accRate*+1 (fwd-acc from +4 to +6)
	//   tar=[4,+1] && cur=[6,+1] --> dif=-2, delta=-decRate*+1 (fwd-dec from +6 to +4)
	//   tar=[6,-1] && cur=[4,-1] --> dif=+2, delta= accRate*-1 (rev-acc from -4 to -6)
	//   tar=[4,-1] && cur=[6,-1] --> dif=-2, delta=-decRate*-1 (rev-dec from -6 to -4)
	//   tar=[4,-1] && cur=[4,+1] --> dif=-8, delta=-decRate*+1 (fwd-dec from +5 to 0, rev-acc from 0 to -5)
	//   tar=[4,+1] && cur=[4,-1] --> dif=-8, delta=-decRate*-1 (rev-dec from -5 to 0, fwd-acc from 0 to +5)
	//
	const float rawSpeedDiff = speeds[isReversing] - speeds[1 - isReversing];
	const float absSpeedDiff = std::max(rawSpeedDiff, -rawSpeedDiff);

	// need to clamp, rates can be much larger than |speedDiff|
	const float modAccRate = std::min(absSpeedDiff, maxAccRate);
	const float modDecRate = std::min(absSpeedDiff, maxDecRate);
	const float deltaSpeed = mix(modAccRate, -modDecRate, (rawSpeedDiff < 0.0f));

	// no acceleration changes if not on ground
	//
	// note: not 100% correct, depends on MoveDef::speedModClass
	// ships should always use (1 - IsInWater()), hovercraft
	// should use (1 - IsInWater()) but only when over water,
	// tanks should also test if IsInWater() && !IsOnGround()
	return ((deltaSpeed * currentSpeedSign) * (1 - owner->IsInAir()));
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
	return (deltaHeading * (1 - owner->IsInAir()));
}

short GMTDefaultPathController::GetDeltaHeading(
	unsigned int pathID,
	short newHeading,
	short oldHeading,
	float maxTurnSpeed,
	float maxTurnAccel,
	float turnBrakeDist,
	float* curTurnSpeedPtr
) const {
	float curTurnSpeed = *curTurnSpeedPtr;
	float absTurnSpeed = math::fabs(curTurnSpeed);

	// negative delta represents a RH turn, positive a LH turn
	// add lookahead term to avoid overshooting target heading
	// note that turnBrakeDist is always positive
	const short brakeDistFactor = (absTurnSpeed >= maxTurnAccel);
	const short stopTurnHeading = oldHeading + (turnBrakeDist * Sign(curTurnSpeed) * brakeDistFactor);
	const short curDeltaHeading = newHeading - stopTurnHeading;

	if (brakeDistFactor == 0) {
		curTurnSpeed  = (Sign(curDeltaHeading) * std::min(math::fabs(curDeltaHeading * 1.0f), maxTurnAccel));
	} else {
		curTurnSpeed += (Sign(curDeltaHeading) * std::min(math::fabs(curDeltaHeading * 1.0f), maxTurnAccel));
	}

	// less realistic to nullify speed in air, but saves headaches
	// (high-turnrate units leaving the ground do not behave well)
	return (*curTurnSpeedPtr = Clamp(curTurnSpeed * (1 - owner->IsInAir()), -maxTurnSpeed, maxTurnSpeed));
}

bool GMTDefaultPathController::IgnoreTerrain(const MoveDef& md, const float3& pos) const {
	return (owner->IsInAir());
}

