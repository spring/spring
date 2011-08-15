/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MoveTypeFactory.h"
#include "MoveInfo.h"
#include "StrafeAirMoveType.h"
#include "HoverAirMoveType.h"
#include "GroundMoveType.h"
#include "StaticMoveType.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

AMoveType* MoveTypeFactory::GetMoveType(CUnit* unit, const UnitDef* ud) {
	if (ud->IsGroundUnit()) {
		// mobile ground-unit
		assert(!ud->canfly);
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

		return gmt;
	}

	if (ud->IsAirUnit()) {
		// mobile air-unit
		assert(ud->canfly);
		assert(ud->movedata == NULL);

		if (!ud->builder && !ud->IsTransportUnit() && ud->IsNonHoveringAirUnit()) {
			CStrafeAirMoveType* sAMT = new CStrafeAirMoveType(unit);

			sAMT->isFighter = (ud->IsFighterUnit());
			sAMT->collide = ud->collide;

			sAMT->wingAngle = ud->wingAngle;
			sAMT->crashDrag = 1.0f - ud->crashDrag;
			sAMT->invDrag = 1.0f - ud->drag;
			sAMT->frontToSpeed = ud->frontToSpeed;
			sAMT->speedToFront = ud->speedToFront;
			sAMT->myGravity = ud->myGravity;

			sAMT->maxBank = ud->maxBank;
			sAMT->maxPitch = ud->maxPitch;
			sAMT->turnRadius = ud->turnRadius;
			sAMT->wantedHeight =
				(ud->wantedHeight * 1.5f) +
				((gs->randFloat() - 0.3f) * 15.0f * (sAMT->isFighter? 2.0f: 1.0f));

			// same as {Ground, HoverAir}MoveType::accRate
			sAMT->maxAcc = ud->maxAcc;
			sAMT->maxAileron = ud->maxAileron;
			sAMT->maxElevator = ud->maxElevator;
			sAMT->maxRudder = ud->maxRudder;

			sAMT->useSmoothMesh = ud->useSmoothMesh;

			return sAMT;
		} else {
			// flying builders, transports, gunships
			CHoverAirMoveType* hAMT = new CHoverAirMoveType(unit);

			hAMT->turnRate = ud->turnRate;
			hAMT->maxSpeed = ud->speed / GAME_SPEED;
			hAMT->accRate = std::max(0.01f, ud->maxAcc);
			hAMT->decRate = std::max(0.01f, ud->maxDec);
			hAMT->wantedHeight = ud->wantedHeight + gs->randFloat() * 5.0f;
			hAMT->orgWantedHeight = hAMT->wantedHeight;
			hAMT->dontLand = ud->DontLand();
			hAMT->collide = ud->collide;
			hAMT->altitudeRate = ud->verticalSpeed;
			hAMT->bankingAllowed = ud->bankingAllowed;
			hAMT->useSmoothMesh = ud->useSmoothMesh;

			return hAMT;
		}
	}

	// unit is immobile (but not necessarily a structure)
	assert(ud->IsImmobileUnit());
	return (new CStaticMoveType(unit));
}
