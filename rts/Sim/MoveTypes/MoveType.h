#ifndef MOVETYPE_H
#define MOVETYPE_H

#include "Object.h"
#include "Sim/Units/Unit.h"

class CMoveType :
	public CObject
{
	CR_DECLARE(CMoveType);
public:
	CMoveType(CUnit* owner);
	virtual ~CMoveType(void);

	virtual void StartMoving(const float3& pos, float goalRadius){};
	virtual void StartMoving(const float3& pos, float goalRadius, float speed){};
	virtual void KeepPointingTo(const float3& pos, float distance, bool aggressive) {};
	virtual void KeepPointingTo(CUnit* unit, float distance, bool aggressive);
	virtual void StopMoving(){};
	virtual void Idle(unsigned int frames){};
	virtual void Idle(){};
	virtual void DeIdle(){};
	virtual void ImpulseAdded(void);

//	virtual float GetSpeedMod(int square){return 1;};
//	virtual float GetSpeedMod(float avrHeight, float maxHeight, float maxDepth, float avrSlope, float maxSlope) {return 1;};

	virtual void SetGoal(const float3& pos){};
	virtual void SetMaxSpeed(float speed);
	virtual void SetWantedMaxSpeed(float speed);
	virtual void LeaveTransport(void);

	virtual void Update(){};
	virtual void SlowUpdate();

	int forceTurn;
	int forceTurnTo;

	CUnit* owner;

	float maxSpeed;
	float maxWantedSpeed;

	bool useHeading;		//probably should move the code in cunit that reads this into the movementclasses

	enum ProgressState {
		Done   = 0,
		Active = 1,
		Failed = 2
	};
	ProgressState progressState;
};


#endif /* MOVETYPE_H */
