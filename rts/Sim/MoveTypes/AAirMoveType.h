/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AAIRMOVETYPE_H_
#define AAIRMOVETYPE_H_

#include "MoveType.h"
#include "Sim/Misc/AirBaseHandler.h"

/**
 * Supposed to be an abstract class.
 * Do not create an instance of this class.
 * Use either CTAAirMoveType or CAirMoveType instead.
 */
class AAirMoveType : public AMoveType
{
	CR_DECLARE(AAirMoveType);
public:

	enum AircraftState{
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
		AIRCRAFT_TAKEOFF,
		/// this is what happens to aircraft with dontLand=1 in fbi
		AIRCRAFT_HOVERING
	} aircraftState;

	AAirMoveType(CUnit* unit);
	~AAirMoveType();

	bool UseSmoothMesh() const;
	/// goalpos to resume flying to after landing
	float3 oldGoalPos;
	float3 oldpos;
	float3 reservedLandingPos;

	float wantedHeight;

	/// mods can use this to disable plane collisions
	bool collide;
	/// controls use of smoothGround for determining altitude
	bool useSmoothMesh;
	/// unit found to be dangerously close to our path
	CUnit* lastColWarning;
	/// 1=generally forward of us, 2=directly in path
	int lastColWarningType;

	bool autoLand;

	virtual bool IsFighter() = 0;
	virtual void Takeoff() = 0;
	void ReservePad(CAirBaseHandler::LandingPad* lp);
	void DependentDied(CObject* o);

protected:
	virtual void SetState(AircraftState state) = 0;
};

#endif // AAIRMOVETYPE_H_
