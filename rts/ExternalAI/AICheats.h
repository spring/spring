#ifndef AICHEATS_H
#define AICHEATS_H

#include "IAICheats.h"
class CSkirmishAIWrapper;

class CAICheats: public IAICheats
{
	CSkirmishAIWrapper* ai;
public:
	CAICheats(CSkirmishAIWrapper* ai);
	~CAICheats(void);

	void SetMyHandicap(float handicap);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, float3 pos);

	const UnitDef* GetUnitDef(int unitId);
	float3 GetUnitPos(int unitId);
	int GetEnemyUnits(int* units);
	int GetEnemyUnits(int* units, const float3& pos, float radius);
	int GetNeutralUnits(int* units);
	int GetNeutralUnits(int* units, const float3& pos, float radius);

	int GetUnitTeam(int unitId);
	int GetUnitAllyTeam(int unitId);
	float GetUnitHealth(int unitId);
	float GetUnitMaxHealth(int unitId);
	float GetUnitPower(int unitId);
	float GetUnitExperience(int unitId);
	bool IsUnitActivated(int unitId);
	bool UnitBeingBuilt(int unitId);
	bool IsUnitNeutral(int unitId);
	bool GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo);
	const CCommandQueue* GetCurrentUnitCommands(int unitId);

	int GetBuildingFacing(int unitId);
	bool IsUnitCloaked(int unitId);
	bool IsUnitParalyzed(int unitId);

	bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	bool GetProperty(int unit, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);
};

#endif
