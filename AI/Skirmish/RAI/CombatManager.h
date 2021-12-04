// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_COMBAT_MANAGER_H
#define RAI_COMBAT_MANAGER_H

class cCombatManager;

#include "RAI.h"
//#include "LogFile.h"
//#include "ExternalAI/IAICallback.h"

class cCombatManager
{
public:
	cCombatManager(IAICallback* callback, cRAI* Global);
	~cCombatManager();

	void UnitIdle(const int& unit, UnitInfo *U);
	void UnitDamaged(const int& unitID, UnitInfo* U, const int& attackerID, EnemyInfo* A, float3& dir);
	bool CommandDGun(const int& unitID, UnitInfo* U);
	bool CommandCapture(const int& unitID, UnitInfo* U, const float& EDis);
//	bool CommandTrap(const int& unitID, UnitInfo* U, const float& EDis);
	bool CommandManeuver(const int& unitID, UnitInfo* U, const float& EDis);
	void CommandRun(const int& unitID, UnitInfo* U, float3& EPos);
	int GetClosestEnemy(float3 Pos, UnitInfo* U);
	float3 GetEnemyPosition(const int& enemyID, EnemyInfo* E);
	float GetNextUpdate(const float& Distance, UnitInfo* U);
	sWeaponEfficiency* CanAttack(UnitInfo* U, EnemyInfo* E, const float3& EPos);
	bool ValidateEnemy(const int& unitID, UnitInfo* U, bool IdleIfInvalid=true);

private:
	int GetClosestThreat(float3 Pos, UnitInfo* U);
	cLogFile *l;
	IAICallback* cb;
	cRAI* G;
};

#endif
