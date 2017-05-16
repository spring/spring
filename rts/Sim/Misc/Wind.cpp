/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Wind.h"
#include "GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/ContainerUtil.h"
#include "System/myMath.h"

CR_BIND(CWind, )

CR_REG_METADATA(CWind, (
	CR_MEMBER(maxWind),
	CR_MEMBER(minWind),

	CR_MEMBER(curWind),
	CR_MEMBER(curStrength),
	CR_MEMBER(curDir),

	CR_MEMBER(newWind),
	CR_MEMBER(oldWind),
	CR_MEMBER(status),

	CR_MEMBER(windGenIDs)
))


// update all units every 15 secs
static const int WIND_UPDATE_RATE = 15 * GAME_SPEED;

CWind wind;

void CWind::LoadWind(float minw, float maxw)
{
	minWind = std::min(minw, maxw);
	maxWind = std::max(minw, maxw);
	curWind = float3(minWind, 0.0f, 0.0f);
	oldWind = curWind;
}

void CWind::ResetState()
{
	maxWind = 100.0f;
	minWind = 0.0f;
	curStrength = 0.0f;
	curDir = RgtVector;
	curWind = ZeroVector;
	newWind = ZeroVector;
	oldWind = ZeroVector;
	status = 0;
	windGenIDs.clear();
}


bool CWind::AddUnit(CUnit* u) {
	// duplicates should never happen, no need to check
	spring::VectorInsertUnique(windGenIDs, u->id);

	// start pointing in direction of wind
	u->UpdateWind(curDir.x, curDir.z, curStrength);
	return true;
}

bool CWind::DelUnit(CUnit* u) {
	return (spring::VectorErase(windGenIDs, u->id));
}



void CWind::Update()
{
	// zero-strength wind does not need updates
	if (maxWind <= 0.0f)
		return;

	if (status == 0) {
		oldWind = curWind;
		newWind = oldWind;

		// generate new wind direction
		float newStrength = 0.0f;

		do {
			newWind.x -= (gsRNG.NextFloat() - 0.5f) * maxWind;
			newWind.z -= (gsRNG.NextFloat() - 0.5f) * maxWind;
			newStrength = newWind.Length();
		} while (newStrength == 0.0f);

		// normalize and clamp s.t. minWind <= strength <= maxWind
		newWind /= newStrength;
		newWind *= (newStrength = Clamp(newStrength, minWind, maxWind));

		// update generators
		for (const int unitID: windGenIDs) {
			(unitHandler->GetUnit(unitID))->UpdateWind(newWind.x, newWind.z, newStrength);
		}
	} else {
		const float mod = smoothstep(0.0f, 1.0f, status / float(WIND_UPDATE_RATE));

		// blend between old & new wind directions
		// note: only generators added on simframes when
		// status != 0 receive a snapshot of the blended
		// direction
		curWind = mix(oldWind, newWind, mod);
		curStrength = curWind.LengthNormalize();

		curDir = curWind;
		curWind = curDir * (curStrength = Clamp(curStrength, minWind, maxWind));
	}

	status = (status + 1) % (WIND_UPDATE_RATE + 1);
}

