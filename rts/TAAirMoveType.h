#pragma once

#include "stdafx.h"
#include "movetype.h"

class CTAAirMoveType :
	public CMoveType
{
public:
	enum AircraftState{
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
		AIRCRAFT_TAKEOFF,
	} aircraftState;

	enum FlyState {
		FLY_CRUISING,
		FLY_CIRCLING,
		FLY_ATTACKING,
		FLY_LANDING
	} flyState;

	bool dontCheckCol;			//needed to get transport close enough to what is going to be transported, better way ?

	float3 goalPos;
	float3 oldpos;
	float wantedHeight;
	float orgWantedHeight;	//to reset altitude back
	
	float3 reservedLandingPos;
	float3 circlingPos;
	float goalDistance;			//Used when circling something
	int waitCounter;			//need to pause between circling steps
	bool wantToStop;			//Set to true on StopMove, to be able to not stop if a new order comes directly after
	
	//Seems odd to use heading in unit, since we have toggled useHeading to false..
	short wantedHeading;

	float3 wantedSpeed;
	float3 deltaSpeed;			//Used to determine banking (since it is the current acceleration)

	float currentBank;

	//Provided by the unitloader
	float turnRate;
	float accRate;
	float decRate;
	float altitudeRate;

	float breakDistance;		//Distance needed to come to a full stop when going at max speed
	bool dontLand;				//Set to true when transporting stuff
	int lastMoveRate;			//Scripts expect moverate functions to be called

	bool forceHeading;			//force the aircraft to turn toward specific heading (for transports)
	short forceHeadingTo;

	float maxDrift;

	CUnit* lastColWarning;		//unit found to be dangerously close to our path
	int lastColWarningType;		//1=generally forward of us,2=directly in path

	CTAAirMoveType(CUnit* owner);
	~CTAAirMoveType(void);

	//MoveType interface	
	virtual void Update();
	virtual void SlowUpdate();
	virtual void StartMoving(float3 pos, float goalRadius);
	virtual void StartMoving(float3 pos, float goalRadius, float speed);
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive);
	virtual void StopMoving();
	virtual void Idle();

	//Main state handlers
	void UpdateLanded();
	void UpdateTakeoff();
	void UpdateLanding();
	void UpdateFlying();
	void UpdateCircling();

	//Helpers for (multiple) state handlers
	void UpdateHeading();
	void UpdateBanking();
	void UpdateAirPhysics();
	void UpdateMoveRate();
	
	void SetGoal(float3 newPos, float distance);
	void SetState(AircraftState newState);

	bool CanLandAt(float3 pos);
	bool FindLandingSpot(float3 curPos, float3 &newPos);
	void ExecuteStop();
	void ForceHeading(short h);
	void SetWantedAltitude(float altitude);
	void CheckForCollision(void);
	void DependentDied(CObject* o);
};
