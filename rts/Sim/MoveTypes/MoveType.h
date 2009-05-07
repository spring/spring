#ifndef MOVETYPE_H
#define MOVETYPE_H

#include "creg/creg_cond.h"
#include "Object.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "float3.h"

class CUnit;

class AMoveType : public CObject
{
	CR_DECLARE(AMoveType);

public:
	AMoveType(CUnit* owner);
	virtual ~AMoveType(void);

	virtual void StartMoving(float3 pos, float goalRadius) = 0;
	virtual void StartMoving(float3 pos, float goalRadius, float speed) = 0;
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive) = 0;
	virtual void KeepPointingTo(CUnit* unit, float distance, bool aggressive);
	virtual void StopMoving() = 0;
	virtual void ImpulseAdded(void);
	virtual void ReservePad(CAirBaseHandler::LandingPad* lp);

//	virtual float GetSpeedMod(int square){return 1;};
//	virtual float GetSpeedMod(float avrHeight, float maxHeight, float maxDepth, float avrSlope, float maxSlope) {return 1;};

	virtual void SetGoal(float3 pos);
	virtual void SetMaxSpeed(float speed);
	virtual void SetWantedMaxSpeed(float speed);
	virtual void LeaveTransport(void);

	virtual void Update() = 0;
	virtual void SlowUpdate();

	int forceTurn;
	int forceTurnTo;

	CUnit* owner;

	float3 goalPos;

	float maxSpeed;
	float maxWantedSpeed;

	CAirBaseHandler::LandingPad* reservedPad;
	/// 0 moving toward,1 landing at,2 arrived
	int padStatus;
	float repairBelowHealth;

	/// TODO: probably should move the code in CUnit that reads this into the movement classes
	bool useHeading;

	enum ProgressState {
		Done   = 0,
		Active = 1,
		Failed = 2
	};
	ProgressState progressState;
protected:
	void DependentDied(CObject* o);
};

class CMoveType : public AMoveType
{
	CR_DECLARE(CMoveType);

public:
	CMoveType(CUnit* unit) : AMoveType(unit){};
	virtual void StartMoving(float3 pos, float goalRadius){};
	virtual void StartMoving(float3 pos, float goalRadius, float speed){};
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive){};
	virtual void StopMoving(){};
	virtual void Update(){};
};


#endif // MOVETYPE_H
