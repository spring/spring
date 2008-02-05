#include "StdAfx.h"
#include "AAirMoveType.h"


CR_BIND_DERIVED_INTERFACE(AAirMoveType, CMoveType);

CR_REG_METADATA(AAirMoveType, (
		CR_MEMBER(goalPos),
		CR_MEMBER(oldGoalPos),
		CR_MEMBER(oldpos),
		CR_MEMBER(reservedLandingPos),
		CR_MEMBER(wantedHeight),
		
		CR_MEMBER(lastColWarning),
		CR_MEMBER(lastColWarningType),
		CR_MEMBER(collide),
		
		CR_MEMBER(repairBelowHealth),
		CR_MEMBER(reservedPad),
		CR_MEMBER(padStatus),
		CR_MEMBER(autoLand),
		
		CR_RESERVED(16)
		));

AAirMoveType::AAirMoveType(CUnit* unit) :
	CMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),
	autoLand(true),
	collide(true),
	goalPos(owner? owner->pos:float3(0, 0, 0)),
	lastColWarning(0),
	lastColWarningType(0),
	oldpos(0,0,0),
	oldGoalPos(owner? owner->pos:float3(0, 0, 0)),
	padStatus(0),
	repairBelowHealth(0.30f),
	reservedLandingPos(-1,-1,-1),
	reservedPad(0),
	wantedHeight(80)
{
}

AAirMoveType::~AAirMoveType()
{
	if (reservedPad) {
		airBaseHandler->LeaveLandingPad(reservedPad);
		reservedPad = 0;
	}
}
/*
void AAirMoveType::SetState(AircraftState state){
	assert(false);
}*/
