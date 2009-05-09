#ifndef TAAIRMOVETYPE_H
#define TAAIRMOVETYPE_H

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
	float breakDistance;
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
	~CTAAirMoveType(void);

	// MoveType interface
	virtual void Update();
	virtual void SlowUpdate();
	virtual void StartMoving(float3 pos, float goalRadius);
	virtual void StartMoving(float3 pos, float goalRadius, float speed);
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive);
	virtual void StopMoving();
	virtual void Idle();

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

	bool CanLandAt(float3 pos);
	void ExecuteStop();
	void ForceHeading(short h);
	void SetWantedAltitude(float altitude);
	void CheckForCollision(void);
	void DependentDied(CObject* o);

	void Takeoff();
	bool IsFighter();
};

#endif // TAAIRMOVETYPE_H
