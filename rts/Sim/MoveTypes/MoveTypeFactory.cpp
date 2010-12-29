/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveTypeFactory.h"
#include "MoveInfo.h"
#include "AirMoveType.h"
#include "TAAirMoveType.h"
#include "GroundMoveType.h"
#include "StaticMoveType.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

AMoveType* MoveTypeFactory::GetMoveType(CUnit* unit, const UnitDef* ud) {
	AMoveType* mt = NULL;

	if (ud->WantsMoveType()) {
		if (!ud->canfly) {
			// ground-mobility
			assert(ud->movedata != NULL);
			assert(unit->mobility == NULL);

			unit->mobility = new MoveData(ud->movedata);

			CGroundMoveType* gmt = new CGroundMoveType(unit);
			gmt->maxSpeed = ud->speed / GAME_SPEED;
			gmt->maxReverseSpeed = ud->rSpeed / GAME_SPEED;
			gmt->maxWantedSpeed = ud->speed / GAME_SPEED;
			gmt->turnRate = ud->turnRate;
			gmt->accRate = std::max(0.01f, ud->maxAcc);
			gmt->decRate = std::max(0.01f, ud->maxDec);

			mt = gmt;
		} else {
			// air-mobility
			assert(ud->movedata == NULL);

			if (!ud->builder && !ud->IsTransportUnit() && (ud->IsFighterUnit() || ud->IsBomberUnit())) {
				CAirMoveType* amt = new CAirMoveType(unit);

				amt->isFighter = (ud->IsFighterUnit());
				amt->collide = ud->collide;

				amt->wingAngle = ud->wingAngle;
				amt->crashDrag = 1.0f - ud->crashDrag;
				amt->invDrag = 1.0f - ud->drag;
				amt->frontToSpeed = ud->frontToSpeed;
				amt->speedToFront = ud->speedToFront;
				amt->myGravity = ud->myGravity;

				amt->maxBank = ud->maxBank;
				amt->maxPitch = ud->maxPitch;
				amt->turnRadius = ud->turnRadius;
				amt->wantedHeight =
					(ud->wantedHeight * 1.5f) +
					((gs->randFloat() - 0.3f) * 15.0f * (amt->isFighter? 2.0f: 1.0f));

				// same as {Ground, TAAir}MoveType::accRate
				amt->maxAcc = ud->maxAcc;

				amt->maxAileron = ud->maxAileron;
				amt->maxElevator = ud->maxElevator;
				amt->maxRudder = ud->maxRudder;

				amt->useSmoothMesh = ud->useSmoothMesh;

				mt = amt;
			} else {
				CTAAirMoveType* taamt = new CTAAirMoveType(unit);

				taamt->turnRate = ud->turnRate;
				taamt->maxSpeed = ud->speed / GAME_SPEED;
				taamt->accRate = std::max(0.01f, ud->maxAcc);
				taamt->decRate = std::max(0.01f, ud->maxDec);
				taamt->wantedHeight = ud->wantedHeight + gs->randFloat() * 5.0f;
				taamt->orgWantedHeight = taamt->wantedHeight;
				taamt->dontLand = ud->DontLand();
				taamt->collide = ud->collide;
				taamt->altitudeRate = ud->verticalSpeed;
				taamt->bankingAllowed = ud->bankingAllowed;
				taamt->useSmoothMesh = ud->useSmoothMesh;

				mt = taamt;
			}
		}
	} else {
		assert(ud->speed <= 0.0f);
		assert(ud->movedata == NULL);

		mt = new CStaticMoveType(unit);
	}

	return mt;
}
