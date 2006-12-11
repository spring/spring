#ifndef MOBILECAI_H
#define MOBILECAI_H

#include "CommandAI.h"

class CMobileCAI :
	public CCommandAI
{
public:
	CMobileCAI(CUnit* owner);
	virtual ~CMobileCAI(void);

	void StopMove();
	virtual void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();
	void GiveCommand(const Command &c);
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

	virtual void Execute();
	virtual void ExecuteGuard(Command &c);
	virtual void ExecuteFight(Command &c);
	virtual void ExecutePatrol(Command &c);
	virtual void ExecuteMove(Command &c);
	virtual void ExecuteSetWantedMaxSpeed(Command &c);

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


protected:
	int cancelDistance;
	bool slowGuard;
	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}
};


#endif /* MOBILECAI_H */
