#ifndef MOBILECAI_H
#define MOBILECAI_H

#include "CommandAI.h"

class CMobileCAI :
	public CCommandAI
{
public:
	CMobileCAI(CUnit* owner);
	~CMobileCAI(void);

	void StopMove();
	void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();
	void GiveCommand(const Command &c);
	void DrawCommands(void);
	void BuggerOff(float3 pos, float radius);
	void NonMoving(void);
	void FinishCommand(void);
	void IdleCheck(void);
	bool CanSetMaxSpeed() const { return true; }

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
	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}
};


#endif /* MOBILECAI_H */
