/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_AI_CHEATS_H
#define _AI_AI_CHEATS_H

#include "IAICheats.h"

struct SSkirmishAICallback;


namespace springLegacyAI {

class CAIAICallback;

/**
 * The AI side wrapper over the C AI interface for IAICheats.
 */
class CAIAICheats : public IAICheats {
public:
	CAIAICheats();
	CAIAICheats(int skirmishAIId, const SSkirmishAICallback* sAICallback,
			CAIAICallback* aiCallback);


	void SetMyHandicap(float handicap);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, float3 pos);

	const UnitDef* GetUnitDef(int unitid);

	float3 GetUnitPos(int unitid);
	float3 GetUnitVel(int unitid);

	int GetEnemyUnits(int* unitIds, int unitIds_max);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);
	int GetNeutralUnits(int* unitIds, int unitIds_max);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);

	int GetFeatures(int *features, int max);
	int GetFeatures(int *features, int max, const float3& pos, float radius);

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

	void EnableCheatEvents(bool enable);

	bool GetProperty(int id, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);

private:
	int skirmishAIId;
	const SSkirmishAICallback* sAICallback;
	CAIAICallback* aiCallback;

	/**
	 * Used to temporarily enable cheating for regular callbacks
	 * that require (extra) priviledges.
	 */
	void SetCheatsEnabled(bool enable);
};

} // namespace springLegacyAI

#endif // _AI_AI_CHEATS_H
