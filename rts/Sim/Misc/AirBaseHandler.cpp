#include "StdAfx.h"
#include "mmgr.h"
#include "AirBaseHandler.h"

#include "GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "creg/STL_List.h"

CAirBaseHandler* airBaseHandler = 0;

CR_BIND(CAirBaseHandler, )
CR_REG_METADATA(CAirBaseHandler,(
	CR_MEMBER(freeBases),
	CR_MEMBER(bases),
	CR_RESERVED(16)
	));

CR_BIND_DERIVED(CAirBaseHandler::LandingPad, CObject, (NULL, 0, NULL));
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


CAirBaseHandler::CAirBaseHandler(void) : freeBases(teamHandler->ActiveAllyTeams()), bases(teamHandler->ActiveAllyTeams())
{
}


CAirBaseHandler::~CAirBaseHandler(void)
{
	//shouldnt be any bases left here...
	for (int a = 0; a < teamHandler->ActiveAllyTeams(); ++a) {
		for (airBaseLstIt bi = bases[a].begin(); bi != bases[a].end(); ++bi) {
			for (padLstIt pi = (*bi)->pads.begin(); pi !=(*bi)->pads.end(); ++pi) {
				delete *pi;
			}
			delete *bi;
		}
	}
}

void CAirBaseHandler::RegisterAirBase(CUnit* base)
{
	AirBase* ab = new AirBase(base);
	std::vector<int> args;

	base->script->QueryLandingPads(/*out*/ args);

	// FIXME: use a set to avoid multiple bases per piece?
	for (int p = 0; p < (int)args.size(); p++) {
		const int piece = args[p];
		if (!base->script->PieceExists(piece)) {
			continue;
		}
		LandingPad* pad = new LandingPad(base, piece, ab);

		ab->pads.push_back(pad);
		ab->freePads.push_back(pad);
	}

	freeBases[base->allyteam].push_back(ab);
	bases[base->allyteam].push_back(ab);
}


void CAirBaseHandler::DeregisterAirBase(CUnit* base)
{
	for (airBaseLstIt bi = freeBases[base->allyteam].begin(); bi != freeBases[base->allyteam].end(); ++bi) {
		if ((*bi)->unit == base) {
			freeBases[base->allyteam].erase(bi);
			break;
		}
	}
	for (airBaseLstIt bi = bases[base->allyteam].begin(); bi != bases[base->allyteam].end(); ++bi) {
		if ((*bi)->unit == base) {
			for (padLstIt pi = (*bi)->pads.begin(); pi != (*bi)->pads.end(); ++pi) {
				// the unit that has reserved a pad is responsible to see if the pad is gone so just delete it
				delete *pi;
			}
			delete *bi;
			bases[base->allyteam].erase(bi);
			break;
		}
	}
}

/** @brief Try to find an airbase and reserve it if one can be found
Caller must call LeaveLandingPad if it gets one and is finished with it or dies
it's the callers responsibility to detect if the base dies while its reserved. */
CAirBaseHandler::LandingPad* CAirBaseHandler::FindAirBase(CUnit* unit, float minPower)
{
	float closest = 1e30f;
	padLstIt foundPad;
	airBaseLstIt foundBase = freeBases[unit->allyteam].end();

	for (airBaseLstIt bi = freeBases[unit->allyteam].begin(); bi != freeBases[unit->allyteam].end(); ++bi) {
		CUnit* baseUnit = (*bi)->unit;

		if (unit == baseUnit) {
			// don't pick ourselves as a landing pad
			continue;
		}

		if (baseUnit->pos.SqDistance(unit->pos) >= closest || baseUnit->unitDef->buildSpeed < minPower) {
			continue;
		}

		closest = baseUnit->pos.SqDistance(unit->pos);
		for (padLstIt pi = (*bi)->freePads.begin(); pi != (*bi)->freePads.end(); ++pi) {
			foundPad = pi;
			foundBase = bi;
		}
	}


	if (foundBase != freeBases[unit->allyteam].end()) {
		LandingPad* found = *foundPad;
		(*foundBase)->freePads.erase(foundPad);
		return found;
	}

	return 0;
}


void CAirBaseHandler::LeaveLandingPad(LandingPad* pad)
{
	pad->GetBase()->freePads.push_back(pad);
}


/** @brief Try to find the closest airbase even if its reserved */
float3 CAirBaseHandler::FindClosestAirBasePos(CUnit* unit, float minPower)
{
	float closest = 1e30f;
	airBaseLstIt foundBase = freeBases[unit->allyteam].end();

	for (airBaseLstIt bi = freeBases[unit->allyteam].begin(); bi != freeBases[unit->allyteam].end(); ++bi) {
		CUnit* baseUnit = (*bi)->unit;

		if (unit == baseUnit) {
			// don't pick ourselves as a landing pad
			continue;
		}

		if (baseUnit->pos.SqDistance(unit->pos) >= closest || baseUnit->unitDef->buildSpeed < minPower) {
			continue;
		}

		closest = baseUnit->pos.SqDistance(unit->pos);
		foundBase = bi;
	}

	if (foundBase != freeBases[unit->allyteam].end()) {
		return (*foundBase)->unit->pos;
	}

	return ZeroVector;
}
