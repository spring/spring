#pragma once
#include "object.h"
#include "unit.h"

class CMoveType :
	public CObject
{
public:
	CMoveType(CUnit* owner);
	virtual ~CMoveType(void);

	virtual void StartMoving(float3 pos, float goalRadius){};
	virtual void StartMoving(float3 pos, float goalRadius, float speed){};
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive) {};
	virtual void StopMoving(){};
	virtual void Idle(unsigned int frames){};
	virtual void Idle(){};
	virtual void DeIdle(){};
	virtual void ImpulseAdded(void);

//	virtual float GetSpeedMod(int square){return 1;};
//	virtual float GetSpeedMod(float avrHeight, float maxHeight, float maxDepth, float avrSlope, float maxSlope) {return 1;};

	virtual void SetGoal(float3 pos){};
	virtual void SetWantedSpeed(float speed){};
	virtual void SetWantedMaxSpeed(float speed);

	virtual void Update(){};
	virtual void SlowUpdate(){};

	int forceTurn;
	int forceTurnTo;

	CUnit* owner;

	float maxSpeed;
	float maxWantedSpeed;

	bool useHeading;		//probably should move the code in cunit that reads this into the movementclasses

	enum ProgressState {
		Done,
		Active,
		Failed
	};
	ProgressState progressState;
};

