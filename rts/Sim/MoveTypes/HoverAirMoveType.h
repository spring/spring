/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HOVER_AIR_MOVE_TYPE_H
#define HOVER_AIR_MOVE_TYPE_H

#include "AAirMoveType.h"

struct float4;

class CHoverAirMoveType: public AAirMoveType
{
	CR_DECLARE_DERIVED(CHoverAirMoveType)
public:
	CHoverAirMoveType(CUnit* owner);

	// MoveType interface
	bool Update() override;
	void SlowUpdate() override;

	void StartMoving(float3 pos, float goalRadius) override;
	void StartMoving(float3 pos, float goalRadius, float speed) override;
	void KeepPointingTo(float3 pos, float distance, bool aggressive) override;
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override;

	bool SetMemberValue(unsigned int memberHash, void* memberValue) override;

	void ForceHeading(short h);
	void SetGoal(const float3& pos, float distance = 0.0f) override;
	void SetState(AircraftState newState) override;
	void SetAllowLanding(bool b);

	AircraftState GetLandingState() const override { return AIRCRAFT_FLYING; }

	// Main state handlers
	void UpdateLanded() override;
	void UpdateTakeoff();
	void UpdateLanding();
	void UpdateFlying();
	void UpdateCircling();
	void UpdateHovering();

	short GetWantedHeading() const { return wantedHeading; }
	short GetForcedHeading() const { return forcedHeading; }

	bool GetAllowLanding() const { return !dontLand; }

private:
	// Helpers for (multiple) state handlers
	void UpdateHeading();
	void UpdateBanking(bool noBanking);
	void UpdateAirPhysics();
	void UpdateMoveRate();

	void UpdateVerticalSpeed(const float4& spd, float curRelHeight, float curVertSpeed) const;

	bool CanLand(bool busy) const { return (!busy && (!dontLand && autoLand)); }
	bool CanLandAt(const float3& pos) const;

	void ExecuteStop();
	void Takeoff() override;
	void Land() override;

	bool HandleCollisions(bool checkCollisions);

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

private:
	float3 wantedSpeed;
	/// Used to determine banking (since it is the current acceleration)
	float3 deltaSpeed;

	float3 circlingPos;
	/// buffets the plane when idling
	float3 randomWind;

	/// force the aircraft to turn toward specific heading (for transports)
	bool forceHeading;

	/// TODO: Seems odd to use heading in unit, since we have toggled useHeading to false..
	short wantedHeading;
	short forcedHeading;

	/// need to pause between circling steps
	int waitCounter;
	/// Scripts expect moverate functions to be called
	int lastMoveRate;
};

#endif // TA_AIR_MOVE_TYPE_H
