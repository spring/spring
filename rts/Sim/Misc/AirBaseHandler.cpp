/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AirBaseHandler.h"

#include "GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"

#include <limits>

CAirBaseHandler* airBaseHandler = NULL;

CR_BIND(CAirBaseHandler, )
CR_REG_METADATA(CAirBaseHandler,(
	CR_MEMBER(bases),
	CR_MEMBER(airBaseIDs),
	CR_RESERVED(16)
));

CR_BIND_DERIVED(CAirBaseHandler::LandingPad, CObject, (0, 0, NULL));
CR_REG_METADATA_SUB(CAirBaseHandler, LandingPad, (
	CR_MEMBER(unit),
	CR_MEMBER(piece),
	CR_MEMBER(base),
	CR_RESERVED(8)
));

CR_BIND(CAirBaseHandler::AirBase, (NULL))
CR_REG_METADATA_SUB(CAirBaseHandler, AirBase, (
	CR_MEMBER(unit),
	CR_MEMBER(freePads),
	CR_MEMBER(pads),
	CR_RESERVED(8)
));


CAirBaseHandler::CAirBaseHandler() : bases(teamHandler->ActiveAllyTeams())
{
}


CAirBaseHandler::~CAirBaseHandler()
{
	// should not be any bases left here...
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		for (AirBaseLstIt bi = bases[a].begin(); bi != bases[a].end(); ++bi) {
			for (PadLstIt pi = (*bi)->pads.begin(); pi != (*bi)->pads.end(); ++pi) {
				delete *pi;
			}
			delete *bi;
		}
	}
}

void CAirBaseHandler::RegisterAirBase(CUnit* owner)
{
	// prevent a unit from being registered as a base more than
	// once (this can happen eg. when an airbase unit is damaged
	// and then repaired back to full health, which causes it to
	// re-register itself via FinishedBuilding())
	if (airBaseIDs.find(owner->id) != airBaseIDs.end()) {
		return;
	}

	AirBase* ab = new AirBase(owner);
	std::vector<int> args;

	owner->script->QueryLandingPads(/*out*/ args);

	// FIXME: use a set to avoid multiple bases per piece?
	for (unsigned int p = 0; p < args.size(); p++) {
		const int piece = args[p];

		if (!owner->script->PieceExists(piece)) {
			continue;
		}

		LandingPad* pad = new LandingPad(piece, owner, ab);

		ab->pads.push_back(pad);
		ab->freePads.push_back(pad);
	}

	bases[owner->allyteam].push_back(ab);
	airBaseIDs.insert(owner->id);
}


void CAirBaseHandler::DeregisterAirBase(CUnit* base)
{
	if (airBaseIDs.find(base->id) == airBaseIDs.end()) {
		return;
	}

	for (AirBaseLstIt bi = bases[base->allyteam].begin(); bi != bases[base->allyteam].end(); ++bi) {
		if ((*bi)->unit == base) {
			for (PadLstIt pi = (*bi)->pads.begin(); pi != (*bi)->pads.end(); ++pi) {
				// the unit that has reserved a pad is responsible to see if the pad is gone so just delete it
				delete *pi;
			}
			delete *bi;
			bases[base->allyteam].erase(bi);
			break;
		}
	}

	airBaseIDs.erase(base->id);
}

void CAirBaseHandler::LeaveLandingPad(LandingPad* pad)
{
	pad->GetBase()->freePads.push_back(pad);
}



CAirBaseHandler::LandingPad* CAirBaseHandler::FindAirBase(CUnit* unit, float minPower, bool wantFreePad)
{
	float minDist = std::numeric_limits<float>::max();

	AirBaseLstIt foundBaseIt = bases[unit->allyteam].end();

	for (AirBaseLstIt bi = bases[unit->allyteam].begin(); bi != bases[unit->allyteam].end(); ++bi) {
		AirBase* base = *bi;
		CUnit* baseUnit = base->unit;

		if (unit == baseUnit) {
			// do not pick ourselves as a landing pad
			continue;
		}
		if (baseUnit->beingBuilt || baseUnit->IsStunned()) {
			continue;
		}

		if (baseUnit->pos.SqDistance(unit->pos) >= minDist || baseUnit->unitDef->buildSpeed < minPower) {
			continue;
		}

		if (wantFreePad && base->freePads.empty()) {
			continue;
		}

		minDist = baseUnit->pos.SqDistance(unit->pos);
		foundBaseIt = bi;
	}

	if (foundBaseIt != bases[unit->allyteam].end()) {
		AirBase* foundBase = *foundBaseIt;

		if (wantFreePad) {
			LandingPad* foundPad = foundBase->freePads.front();
			foundBase->freePads.pop_front();
			return foundPad;
		} else {
			if (!foundBase->pads.empty())
				return foundBase->pads.front();
		}
	}

	return NULL;
}

float3 CAirBaseHandler::FindClosestAirBasePos(CUnit* unit, float minPower)
{
	const CAirBaseHandler::LandingPad* pad = FindAirBase(unit, minPower, false);

	if (pad == NULL)
		return ZeroVector;

	const AirBase* base = pad->GetBase();
	const CUnit* baseUnit = base->unit;
	return baseUnit->pos;
}

