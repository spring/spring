/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TA_AIR_MOVE_TYPE_H
#define TA_AIR_MOVE_TYPE_H

#include "AAirMoveType.h"

struct float4;

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

	bool bankingAllowed;
	bool airStrafe;
	/// Set to true on StopMove, to be able to not stop if a new order comes directly after
	bool wantToStop;

	/// Used when circling something
	float goalDistance;

	float currentBank;
	float currentPitch;

	float turnRate;

	float maxDrift;
	float maxTurnAngle;

public:
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
	void SetAllowLanding(bool b);

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

	short GetWantedHeading() const { return wantedHeading; }
	short GetForcedHeading() const { return forceHeadingTo; }

	bool GetAllowLanding() const { return !dontLand; }

private:
	// Helpers for (multiple) state handlers
	void UpdateHeading();
	void UpdateBanking(bool noBanking);
	void UpdateAirPhysics();
	void UpdateMoveRate();

	void UpdateVerticalSpeed(const float4& spd, float curRelHeight, float curVertSpeed) const;

	bool CanLand(bool busy) const { return (!busy && ((!dontLand && autoLand) || (reservedPad != NULL))); }
	bool CanLandAt(const float3& pos) const;

	void ExecuteStop();
	void Takeoff();
	void Land();

	bool HandleCollisions(bool checkCollisions);

private:
	float3 wantedSpeed;
	/// Used to determine banking (since it is the current acceleration)
	float3 deltaSpeed;

	float3 circlingPos;
	/// buffets the plane when idling
	float3 randomWind;

	/// force the aircraft to turn toward specific heading (for transports)
	bool forceHeading;
	/// Set to true when transporting stuff
	bool dontLand;

	/// TODO: Seems odd to use heading in unit, since we have toggled useHeading to false..
	short wantedHeading;
	short forceHeadingTo;

	/// need to pause between circling steps
	int waitCounter;
	/// Scripts expect moverate functions to be called
	int lastMoveRate;
};

#endif // TA_AIR_MOVE_TYPE_H
