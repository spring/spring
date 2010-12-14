/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_CHEATS_H
#define AI_CHEATS_H

#include "ExternalAI/AILegacySupport.h"
#include "float3.h"

#include <string>
#include <vector>
#include <map>

struct Command;
struct UnitDef;
struct FeatureDef;
struct WeaponDef;
struct CommandDescription;
class CCommandQueue;
class CGroupHandler;
class CGroup;
class CUnit;
class CSkirmishAIWrapper;

class CAICheats
{
	CSkirmishAIWrapper* ai;

	// utility methods

	/// Returns the unit if the ID is valid
	CUnit* GetUnit(int unitId) const;

public:
	CAICheats(CSkirmishAIWrapper* ai);
	~CAICheats();

	void SetMyIncomeMultiplier(float incomeMultiplier);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, const float3& pos);

	const UnitDef* GetUnitDef(int unitId);
	float3 GetUnitPos(int unitId);
	float3 GetUnitVelocity(int unitId);
	int GetEnemyUnits(int* unitIds, int unitIds_max = -1);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);

	int GetFeatures(int* features, int max) const;
	int GetFeatures(int* features, int max, const float3& pos, float radius) const;

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

	static bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	bool GetProperty(int unit, int property, void* dst);
	bool GetValue(int id, void* dst) const;
	int HandleCommand(int commandId, void* data);
};

#endif // AI_CHEATS_H
