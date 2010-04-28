/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOBILECAI_H
#define MOBILECAI_H

#include "CommandAI.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Misc/GlobalConstants.h"

class CMobileCAI :
	public CCommandAI
{
public:
	CR_DECLARE(CMobileCAI);
	CMobileCAI(CUnit* owner);
	CMobileCAI();
	virtual ~CMobileCAI(void);

	void StopMove();
	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius, float speed);
	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();
	void GiveCommandReal(const Command &c, bool fromSynced = true);
	void DrawCommands(void);
	void BuggerOff(float3 pos, float radius);
	void NonMoving(void);
	void FinishCommand(void);
	void IdleCheck(void);
	bool CanSetMaxSpeed() const { return true; }
	void StopSlowGuard();
	void StartSlowGuard(float speed);
	void ExecuteAttack(Command &c);
	void ExecuteDGun(Command &c);
	void ExecuteStop(Command &c);

	bool RefuelIfNeeded();
	bool LandRepairIfNeeded();

	virtual void Execute();
	virtual void ExecuteGuard(Command &c);
	virtual void ExecuteFight(Command &c);
	virtual void ExecutePatrol(Command &c);
	virtual void ExecuteMove(Command &c);
	virtual void ExecuteSetWantedMaxSpeed(Command &c);
	virtual void ExecuteLoadUnits(Command &c);

	float3 goalPos;
	float3 lastUserGoal;

	int lastIdleCheck;
	bool tempOrder;

	int lastPC; //helps avoid infinate loops

//	unsigned int patrolTime;

	float maxWantedSpeed;

	int lastBuggerOffTime;
	float3 buggerOffPos;
	float buggerOffRadius;

	float3 commandPos1;			//used to avoid stuff in maneuvre mode moving to far away from patrol path
	float3 commandPos2;

	int GetCancelDistance() { return cancelDistance; }

	virtual bool IsValidTarget(const CUnit* enemy) const;

protected:
	int cancelDistance;
	int lastCloseInTry;
	bool slowGuard;
	bool moveDir;

	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}

	void CalculateCancelDistance();
};

#endif /* MOBILECAI_H */
