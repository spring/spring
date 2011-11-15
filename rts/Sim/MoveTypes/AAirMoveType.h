/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef A_AIR_MOVE_TYPE_H_
#define A_AIR_MOVE_TYPE_H_

#include "MoveType.h"
#include "Sim/Misc/AirBaseHandler.h"

/**
 * Supposed to be an abstract class.
 * Do not create an instance of this class.
 * Use either CHoverAirMoveType or CStrafeAirMoveType instead.
 */
class AAirMoveType : public AMoveType
{
	CR_DECLARE(AAirMoveType);
public:

	enum AircraftState {
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
		AIRCRAFT_TAKEOFF,
		/// this is what happens to aircraft with dontLand=1 in fbi
		AIRCRAFT_HOVERING
	} aircraftState;

	AAirMoveType(CUnit* unit);
	virtual ~AAirMoveType();

	virtual bool Update();
	virtual bool IsFighter() const = 0;
	virtual void Takeoff() = 0;
	virtual void SetState(AircraftState state) = 0;
	virtual AircraftState GetLandingState() const { return AIRCRAFT_LANDING; }

	bool UseSmoothMesh() const;
	int GetPadStatus() const { return padStatus; }

	void ReservePad(CAirBaseHandler::LandingPad* lp);
	void UnreservePad(CAirBaseHandler::LandingPad* lp);
	void DependentDied(CObject* o);

	CAirBaseHandler::LandingPad* GetReservedPad() { return reservedPad; }

public:
	/// goalpos to resume flying to after landing
	float3 oldGoalPos;
	float3 reservedLandingPos;

	float wantedHeight;
	/// to reset altitude back
	float orgWantedHeight;

	/// mods can use this to disable plane collisions
	bool collide;
	/// controls use of smoothGround for determining altitude
	bool useSmoothMesh;
	bool autoLand;

protected:
	void CheckForCollision();
	bool MoveToRepairPad();
	void UpdateFuel();

	/// unit found to be dangerously close to our path
	CUnit* lastColWarning;
	CAirBaseHandler::LandingPad* reservedPad;

	/// 1=generally forward of us, 2=directly in path
	int lastColWarningType;
	int lastFuelUpdateFrame;
	/// 0: moving toward, 1: landing at, 2: arrived
	int padStatus;
};

#endif // A_AIR_MOVE_TYPE_H_
