/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_AIR_MOVE_TYPE_H
#define TA_AIR_MOVE_TYPE_H

#include "AAirMoveType.h"

class CHoverAirMoveType: public AAirMoveType
{
	CR_DECLARE(CHoverAirMoveType);
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
	bool loadingUnits;
	bool bankingAllowed;
	bool airStrafe;

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


	CHoverAirMoveType(CUnit* owner);

	// MoveType interface
	bool Update();
	void SlowUpdate();
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void StopMoving(bool callScript = false, bool hardStop = false);

	void ForceHeading(short h);
	void SetGoal(const float3& pos, float distance = 0.0f);
	void SetState(AircraftState newState);
	AircraftState GetLandingState() const { return AIRCRAFT_FLYING; }
	void SetWantedAltitude(float altitude);
	void SetDefaultAltitude(float altitude);

	// Main state handlers
	void UpdateLanded();
	void UpdateTakeoff();
	void UpdateLanding();
	void UpdateFlying();
	void UpdateCircling();
	void UpdateHovering();

private:
	// Helpers for (multiple) state handlers
	void UpdateHeading();
	void UpdateBanking(bool noBanking);
	void UpdateAirPhysics();
	void UpdateMoveRate();

	bool CanLandAt(const float3& pos) const;
	void ExecuteStop();

	void Takeoff();
	void Land();
	bool IsFighter() const { return false; }

	bool HandleCollisions();
};

#endif // TA_AIR_MOVE_TYPE_H
