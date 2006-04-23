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
	void SetGoal(float3& pos,float3& curPos, float goalRadius = SQUARE_SIZE);
	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();
	void GiveCommand(Command &c);
	void DrawCommands(void);
	void BuggerOff(float3 pos, float radius);
	void NonMoving(void);
	void FinishCommand(void);
	void IdleCheck(void);

	float3 goalPos;
	float3 patrolGoal;
	float3 lastUserGoal;

	int activeCommand;
	int lastIdleCheck;
	bool tempOrder;

	unsigned int patrolTime;

	float maxWantedSpeed;

	int lastBuggerOffTime;
	float3 buggerOffPos;
	float buggerOffRadius;

	float3 commandPos1;			//used to avoid stuff in maneuvre mode moving to far away from patrol path
	float3 commandPos2;
};


#endif /* MOBILECAI_H */
