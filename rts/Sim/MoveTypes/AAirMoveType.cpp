#include "StdAfx.h"
#include "AAirMoveType.h"
#include "Sim/Units/Unit.h"


CR_BIND_DERIVED_INTERFACE(AAirMoveType, AMoveType);

CR_REG_METADATA(AAirMoveType, (
		CR_MEMBER(oldGoalPos),
		CR_MEMBER(oldpos),
		CR_MEMBER(reservedLandingPos),

		CR_MEMBER(wantedHeight),

		CR_MEMBER(collide),
		CR_MEMBER(lastColWarning),
		CR_MEMBER(lastColWarningType),

		CR_MEMBER(autoLand),

		CR_RESERVED(16)
		));

AAirMoveType::AAirMoveType(CUnit* unit) :
	AMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),
	oldGoalPos(owner? owner->pos:float3(0, 0, 0)),
	oldpos(0,0,0),
	reservedLandingPos(-1,-1,-1),
	wantedHeight(80),
	collide(true),
	lastColWarning(0),
	lastColWarningType(0),
	autoLand(true)
{
	useHeading = false;
}

AAirMoveType::~AAirMoveType()
{
	if (reservedPad) {
		airBaseHandler->LeaveLandingPad(reservedPad);
		reservedPad = 0;
	}
}

void AAirMoveType::ReservePad(CAirBaseHandler::LandingPad* lp) {
	oldGoalPos = goalPos;
	AMoveType::ReservePad(lp);
	Takeoff();
}

void AAirMoveType::DependentDied(CObject* o)
{
	AMoveType::DependentDied(o);
	if(o == reservedPad){
		SetState(AIRCRAFT_FLYING);
		goalPos=oldGoalPos;
	}
}
