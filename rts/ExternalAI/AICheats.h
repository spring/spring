/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_CHEATS_H
#define AI_CHEATS_H

#include "ExternalAI/AILegacySupport.h"
#include "System/float3.h"

#include <string>
#include <vector>

struct Command;
struct UnitDef;
struct FeatureDef;
struct WeaponDef;
class CCommandQueue;
class CGroupHandler;
class CGroup;
class CUnit;
class CSkirmishAIWrapper;

class CAICheats
{
	CSkirmishAIWrapper* ai = nullptr;

	// utility methods

	/// Returns the unit if the ID is valid
	CUnit* GetUnit(int unitId) const;

public:
	CAICheats() = default;
	CAICheats(CSkirmishAIWrapper* w): ai(w) {}

	void SetMyIncomeMultiplier(float incomeMultiplier);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, const float3& pos);

	int GetEnemyUnits(int* unitIds, int unitIds_max = -1);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);

	int GetFeatures(int* features, int max) const;
	int GetFeatures(int* features, int max, const float3& pos, float radius) const;

	const UnitDef* GetUnitDef(int unitId) const;
	float3 GetUnitPos(int unitId) const;
	float3 GetUnitVelocity(int unitId) const;
	int GetUnitTeam(int unitId) const;
	int GetUnitAllyTeam(int unitId) const;
	float GetUnitHealth(int unitId) const;
	float GetUnitMaxHealth(int unitId) const;
	float GetUnitPower(int unitId) const;
	float GetUnitExperience(int unitId) const;
	bool IsUnitActivated(int unitId) const;
	bool UnitBeingBuilt(int unitId) const;
	bool IsUnitNeutral(int unitId) const;
	bool GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) const;
	const CCommandQueue* GetCurrentUnitCommands(int unitId) const;

	int GetBuildingFacing(int unitId) const;
	bool IsUnitCloaked(int unitId) const;
	bool IsUnitParalyzed(int unitId) const;

	static bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	bool GetProperty(int unit, int property, void* dst) const;
	bool GetValue(int id, void* dst) const;
	int HandleCommand(int commandId, void* data);
};

#endif // AI_CHEATS_H
