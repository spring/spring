// AirCAI.h: Air command AI
///////////////////////////////////////////////////////////////////////////

#ifndef __AIR_CAI_H__
#define __AIR_CAI_H__

#include "MobileCAI.h"

class CAirCAI :
	public CMobileCAI
{
public:
	CAirCAI(CUnit* owner);
	~CAirCAI(void);

	int GetDefaultCmd(CUnit* pointed,CFeature* feature);
	void SlowUpdate();
	void GiveCommand(const Command &c);
	void DrawCommands(void);
	void AddUnit(CUnit* unit);
	void FinishCommand(void);
	void BuggerOff(float3 pos, float radius);
	void StopMove();
	
	void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	
	void ExecuteGuard(Command &c);
	void ExecuteAreaAttack(Command &c);
	void ExecuteAttack(Command &c);
	void ExecuteFight(Command &c);
//	void ExecuteMove(Command &c);

	float3 basePos;
	float3 baseDir;

	int activeCommand;
	int targetAge;
//	unsigned int patrolTime;

	int lastPC1;
	int lastPC2;

protected:
	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}
};

#endif // __AIR_CAI_H__
