/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __AIR_CAI_H__
#define __AIR_CAI_H__

#include "MobileCAI.h"

class CUnit;
class CFeature;
struct Command;

class CAirCAI : public CMobileCAI
{
public:
	CR_DECLARE(CAirCAI);
	CAirCAI(CUnit* owner);
	CAirCAI();
	~CAirCAI();

	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();
	void GiveCommandReal(const Command& c);
	void DrawCommands();
	void AddUnit(CUnit* unit);
	void FinishCommand();
	void BuggerOff(float3 pos, float radius);
//	void StopMove();
	
	void SetGoal(const float3& pos, const float3& curPos, float goalRadius = SQUARE_SIZE);
	
	void ExecuteGuard(Command& c);
	void ExecuteAreaAttack(Command& c);
	void ExecuteAttack(Command& c);
	void ExecuteFight(Command& c);
//	void ExecuteMove(Command& c);

	bool IsValidTarget(const CUnit* enemy) const;

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
