/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef A_AIR_MOVE_TYPE_H_
#define A_AIR_MOVE_TYPE_H_

#include "MoveType.h"

/**
 * Supposed to be an abstract class.
 * Do not create an instance of this class.
 * Use either CHoverAirMoveType or CStrafeAirMoveType instead.
 */
class AAirMoveType : public AMoveType
{
	CR_DECLARE(AAirMoveType)
public:
	typedef float(*GetGroundHeightFunc)(float, float);
	typedef void(*EmitCrashTrailFunc)(CUnit*, unsigned int);

	enum AircraftState {
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
		AIRCRAFT_TAKEOFF,
		/// this is what happens to aircraft with dontLand=1 in fbi
		AIRCRAFT_HOVERING
	};
	enum CollisionState {
		COLLISION_NOUNIT = 0,
		COLLISION_DIRECT = 1, // "directly on path"
		COLLISION_NEARBY = 2, // "generally nearby"
	};

	AAirMoveType(CUnit* unit);
	virtual ~AAirMoveType() {}

	virtual bool Update();
	virtual void UpdateLanded();
	virtual void Takeoff() {}
	virtual void Land() {}
	virtual void SetState(AircraftState state) {}
	virtual AircraftState GetLandingState() const { return AIRCRAFT_LANDING; }

	void SetWantedAltitude(float altitude) { wantedHeight = mix(orgWantedHeight, altitude, altitude != 0.0f); }
	void SetDefaultAltitude(float altitude) {
		wantedHeight = altitude;
		orgWantedHeight = altitude;
	}

	bool HaveLandingPos() const { return (reservedLandingPos.x != -1.0f); }

	void LandAt(float3 pos, float distanceSq);
	void ClearLandingPos() { reservedLandingPos = -OnesVector; }
	void UpdateLandingHeight(float newWantedHeight);
	void UpdateLanding();

	bool CanApplyImpulse(const float3&) { return true; }
	bool UseSmoothMesh() const;

	void DependentDied(CObject* o);

protected:
	void CheckForCollision();

public:
	AircraftState aircraftState = AIRCRAFT_LANDED;
	CollisionState collisionState = COLLISION_NOUNIT;

	/// goalpos to resume flying to after landing
	float3 oldGoalPos;
	float3 reservedLandingPos = -OnesVector;

	float landRadiusSq = 0.0f;
	float wantedHeight = 80.0f;
	/// to reset altitude back
	float orgWantedHeight = 0.0f;

	float accRate = 1.0f;
	float decRate = 1.0f;
	float altitudeRate = 3.0f;

	/// mods can use this to disable plane collisions
	bool collide = true;
	bool autoLand = true;
	bool dontLand = false;
	/// controls use of smoothGround for determining altitude
	bool useSmoothMesh = false;
	bool canSubmerge = false;
	bool floatOnWater = false;

protected:
	/// unit found to be dangerously close to our path
	CUnit* lastCollidee = nullptr;

	unsigned int crashExpGenID = -1u;
};

#endif // A_AIR_MOVE_TYPE_H_
