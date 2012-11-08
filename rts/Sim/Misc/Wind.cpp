/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Wind.h"
#include "GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_Map.h"
#include "System/myMath.h"

CR_BIND(CWind, );

CR_REG_METADATA(CWind, (
	CR_MEMBER(maxWind),
	CR_MEMBER(minWind),

	CR_MEMBER(curWind),
	CR_MEMBER(curStrength),
	CR_MEMBER(curDir),

	CR_MEMBER(newWind),
	CR_MEMBER(oldWind),
	CR_MEMBER(status),

	CR_MEMBER(windGens),
	CR_RESERVED(12)
));


static const int UpdateRate = 15 * GAME_SPEED; //! update all 15sec

CWind wind;


CWind::CWind()
{
	curDir = float3(1.0f, 0.0f, 0.0f);
	curStrength = 0.0f;

	curWind = ZeroVector;
	newWind = curWind;
	oldWind = curWind;

	maxWind = 100.0f;
	minWind =   0.0f;

	status = 0;
}

CWind::~CWind()
{
	windGens.clear();
}



bool CWind::AddUnit(CUnit* u) {
	std::map<int, CUnit*>::iterator it = windGens.find(u->id);

	if (it != windGens.end()) {
		return false;
	}

	windGens[u->id] = u;
	// start pointing in direction of wind
	u->UpdateWind(curDir.x, curDir.z, curStrength);
	return true;
}

bool CWind::DelUnit(CUnit* u) {
	std::map<int, CUnit*>::iterator it = windGens.find(u->id);

	if (it == windGens.end()) {
		return false;
	}

	windGens.erase(it);
	return true;
}



void CWind::LoadWind(float minw, float maxw)
{
	minWind = std::min(minw, maxw);
	maxWind = std::max(minw, maxw);
	curWind = float3(minWind, 0.0f, 0.0f);
	oldWind = curWind;
}


void CWind::Update()
{
	//! zero-strength wind does not need updates
	if (maxWind <= 0.0f)
		return;

	if (status == 0) {
		oldWind = curWind;
		newWind = oldWind;

		//! generate new wind direction
		float len;
		do {
			newWind.x -= (gs->randFloat() - 0.5f) * maxWind;
			newWind.z -= (gs->randFloat() - 0.5f) * maxWind;
			len = newWind.Length();
		} while (len == 0.0f);

		//! clamp: windMin <= strength <= windMax
		newWind /= len;
		len = Clamp(len, minWind, maxWind);
		newWind *= len;

		status++;
	} else if (status <= UpdateRate) {
		float mod = status / float(UpdateRate);
		mod = smoothstep(0.0f, 1.0f, mod);

		//! blend between old & new wind directions
		#define blend(x,y,a) x * (1.0f - a) + y * a
		curWind = blend(oldWind, newWind, mod);
		curStrength = curWind.Length();
		if (curStrength != 0.0f) {
			curDir = curWind / curStrength;
		}

		status++;
	} else {
		status = 0;
	}
	
	if (status == UpdateRate / 3) {
		//! update units
		const float newStrength = newWind.Length();
		for (std::map<int, CUnit*>::iterator it = windGens.begin(); it != windGens.end(); ++it) {
			(it->second)->UpdateWind(newWind.x, newWind.z, newStrength);
		}
	}
}
