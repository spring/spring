/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AIR_CAI_H
#define AIR_CAI_H

#include "MobileCAI.h"

class CUnit;
class CFeature;
class CStrafeAirMoveType;
struct Command;
class AAirMoveType;

class CAirCAI : public CMobileCAI
{
public:
	CR_DECLARE(CAirCAI)
	CAirCAI(CUnit* owner);
	CAirCAI();

	int GetDefaultCmd(const CUnit* pointed, const CFeature* feature);
	void SlowUpdate();
	void GiveCommandReal(const Command& c, bool fromSynced = true);
	void AddUnit(CUnit* unit);
	void FinishCommand();
	void BuggerOff(const float3& pos, float radius);
//	void StopMove();

	void ExecuteGuard(Command& c);
	void ExecuteAreaAttack(Command& c);
	void ExecuteAttack(Command& c);
	void ExecuteFight(Command& c);
	void ExecuteMove(Command& c);

	bool IsValidTarget(const CUnit* enemy, CWeapon* weapon) const override;

private:
	bool AirAutoGenerateTarget(AAirMoveType*);
	bool SelectNewAreaAttackTargetOrPos(const Command& ac);
	void PushOrUpdateReturnFight() {
		CCommandAI::PushOrUpdateReturnFight(commandPos1, commandPos2);
	}

	float3 basePos;
	float3 baseDir;

	int activeCommand;
	int targetAge;
//	unsigned int patrolTime;

	int lastPC1;
	int lastPC2;
};

#endif // AIR_CAI_H
