/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_AIR_MOVE_TYPE_H
#define TA_AIR_MOVE_TYPE_H

#include "AAirMoveType.h"

class CTAAirMoveType : public AAirMoveType
{
	CR_DECLARE(CTAAirMoveType);
public:
	enum FlyState {
		FLY_CRUISING,
		FLY_CIRCLING,
		FLY_ATTACKING,
		FLY_LANDING
	} flyState;

	/**
	 * needed to get transport close enough to what is going to be transported.
	 * better way ?
	 */
	bool dontCheckCol;
	bool bankingAllowed;

	/// to reset altitude back
	float orgWantedHeight;

	float3 circlingPos;
	/// Used when circling something
	float goalDistance;
	/// need to pause between circling steps
	int waitCounter;
	/// Set to true on StopMove, to be able to not stop if a new order comes directly after
	bool wantToStop;

	/// TODO: Seems odd to use heading in unit, since we have toggled useHeading to false..
	short wantedHeading;

	float3 wantedSpeed;
	/// Used to determine banking (since it is the current acceleration)
	float3 deltaSpeed;

	float currentBank;
	float currentPitch;

	// Provided by the unitloader
	float turnRate;
	float accRate;
	float decRate;
	float altitudeRate;

	/// Distance needed to come to a full stop when going at max speed
	float brakeDistance;
	/// Set to true when transporting stuff
	bool dontLand;
	/// Scripts expect moverate functions to be called
	int lastMoveRate;

	/// force the aircraft to turn toward specific heading (for transports)
	bool forceHeading;
	short forceHeadingTo;

	float maxDrift;

	/// buffets the plane when idling
	float3 randomWind;


	CTAAirMoveType(CUnit* owner);
	~CTAAirMoveType();

	// MoveType interface
	void Update();
	void SlowUpdate();
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void StopMoving();
	void Idle();

	// Main state handlers
	void UpdateLanded();
	void UpdateTakeoff();
	void UpdateLanding();
	void UpdateFlying();
	void UpdateCircling();
	void UpdateHovering();

	// Helpers for (multiple) state handlers
	void UpdateHeading();
	void UpdateBanking(bool noBanking);
	void UpdateAirPhysics();
	void UpdateMoveRate();

	void SetGoal(float3 newPos, float distance);
	void SetState(AircraftState newState);

	bool CanLandAt(const float3& pos) const;
	void ExecuteStop();
	void ForceHeading(short h);
	void SetWantedAltitude(float altitude);
	void SetDefaultAltitude(float altitude);
	void CheckForCollision();
	void DependentDied(CObject* o);

	void Takeoff();
	bool IsFighter() const;
};

#endif // TA_AIR_MOVE_TYPE_H
