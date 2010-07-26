/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Wind.h"
#include "GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_Map.h"

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



void CWind::LoadWind(float min, float max)
{
	minWind = min;
	maxWind = max;
	curWind = float3(minWind, 0.0f, 0.0f);
}


void CWind::Update()
{
	if (status == 0) {
		oldWind = curWind;

		float ns = gs->randFloat() * (maxWind - minWind) + minWind;
		float nd = gs->randFloat() * 2.0f * PI;

		newWind = float3(sin(nd) * ns, 0.0f, cos(nd) * ns);

		for (std::map<int, CUnit*>::iterator it = windGens.begin(); it != windGens.end(); ++it) {
			(it->second)->UpdateWind(newWind.x, newWind.z, newWind.Length());
		}

		status++;
	} else if (status <= (GAME_SPEED * GAME_SPEED)) {
		const float mod = status / float(GAME_SPEED * GAME_SPEED);

		curStrength = oldWind.Length() * (1.0 - mod) + newWind.Length() * mod; // strength changes ~ mod
		curWind = oldWind * (1.0 - mod) + newWind * mod; // dir changes ~ arctan (mod)
		curWind.SafeNormalize();
		curDir = curWind;
		curWind *= curStrength;

		status++;
	} else {
		status = 0;
	}
}
