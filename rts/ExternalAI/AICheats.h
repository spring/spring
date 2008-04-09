#ifndef AICHEATS_H
#define AICHEATS_H

#include "IAICheats.h"
class CGlobalAI;

class CAICheats: public IAICheats
{
	CGlobalAI* ai;
public:
	CAICheats(CGlobalAI* ai);
	~CAICheats(void);

	void SetMyHandicap(float handicap);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, float3 pos);

	const UnitDef* GetUnitDef(int unitid);
	float3 GetUnitPos(int unitid);
	int GetEnemyUnits(int* units);
	int GetEnemyUnits(int* units, const float3& pos, float radius);
	int GetNeutralUnits(int* units);
	int GetNeutralUnits(int* units, const float3& pos, float radius);

	int GetUnitTeam(int unitid);
	int GetUnitAllyTeam(int unitid);
	float GetUnitHealth(int unitid);
	float GetUnitMaxHealth(int unitid);
	float GetUnitPower(int unitid);
	float GetUnitExperience(int unitid);
	bool IsUnitActivated(int unitid);
	bool UnitBeingBuilt(int unitid);
	bool IsUnitNeutral(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);
	const CCommandQueue* GetCurrentUnitCommands(int unitid);

	int GetBuildingFacing(int unitid);
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);

	bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	bool GetProperty(int unit, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);
};

#endif
