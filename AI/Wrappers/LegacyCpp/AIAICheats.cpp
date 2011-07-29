/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIAICheats.h"

#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "AIAICallback.h"
#include "ExternalAI/Interface/AISCommands.h"


static inline int getResourceIndex_Metal(const SSkirmishAICallback* sAICallback, int skirmishAIId) {
	static int resIndMetal = -1;

	if (resIndMetal == -1) {
		resIndMetal = sAICallback->getResourceByName(skirmishAIId, "Metal");
	}

	return resIndMetal;
}
static inline int getResourceIndex_Energy(const SSkirmishAICallback* sAICallback, int skirmishAIId) {
	static int resIndEnergy = -1;

	if (resIndEnergy == -1) {
		resIndEnergy = sAICallback->getResourceByName(skirmishAIId, "Energy");
	}

	return resIndEnergy;
}

springLegacyAI::CAIAICheats::CAIAICheats()
		: IAICheats(), skirmishAIId(-1), sAICallback(NULL), aiCallback(NULL) {}

springLegacyAI::CAIAICheats::CAIAICheats(int skirmishAIId, const SSkirmishAICallback* sAICallback, CAIAICallback* aiCallback)
		: IAICheats(), skirmishAIId(skirmishAIId), sAICallback(sAICallback), aiCallback(aiCallback) {}

void springLegacyAI::CAIAICheats::SetCheatsEnabled(bool enabled) {
	sAICallback->Cheats_setEnabled(skirmishAIId, enabled);
}

void springLegacyAI::CAIAICheats::EnableCheatEvents(bool enable) {

	SetCheatsEnabled(true);
	sAICallback->Cheats_setEventsEnabled(skirmishAIId, enable);
	SetCheatsEnabled(false);
}


void springLegacyAI::CAIAICheats::SetMyHandicap(float handicap) {
	SetCheatsEnabled(true);
	SSetMyIncomeMultiplierCheatCommand cmd = { (handicap / 100.0f) + 1.0f };
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER, &cmd);
	SetCheatsEnabled(false);
}
void springLegacyAI::CAIAICheats::GiveMeMetal(float amount) {
	const static int m = getResourceIndex_Metal(sAICallback, skirmishAIId);

	SetCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {m, amount};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	SetCheatsEnabled(false);
}
void springLegacyAI::CAIAICheats::GiveMeEnergy(float amount) {
	const static int e = getResourceIndex_Energy(sAICallback, skirmishAIId);

	SetCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {e, amount};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	SetCheatsEnabled(false);
}

int springLegacyAI::CAIAICheats::CreateUnit(const char* unitDefName, float3 pos) {
	SetCheatsEnabled(true);
	const int unitDefId = sAICallback->getUnitDefByName(skirmishAIId, unitDefName);
	float pos_f3[3];
	pos.copyInto(pos_f3);
	SGiveMeNewUnitCheatCommand cmd = {unitDefId, pos_f3};
	const int unitId = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE,
			-1, COMMAND_CHEATS_GIVE_ME_NEW_UNIT, &cmd);
	SetCheatsEnabled(false);
	return unitId;
}

const springLegacyAI::UnitDef* springLegacyAI::CAIAICheats::GetUnitDef(int unitId) {
	SetCheatsEnabled(true);
	const UnitDef* unitDef = aiCallback->GetUnitDef(unitId);
	SetCheatsEnabled(false);
	return unitDef;
}



float3 springLegacyAI::CAIAICheats::GetUnitPos(int unitId) {
	SetCheatsEnabled(true);
	const float3 pos = aiCallback->GetUnitPos(unitId);
	SetCheatsEnabled(false);
	return pos;
}

float3 springLegacyAI::CAIAICheats::GetUnitVel(int unitId) {
	SetCheatsEnabled(true);
	const float3 pos = aiCallback->GetUnitVel(unitId);
	SetCheatsEnabled(false);
	return pos;
}



int springLegacyAI::CAIAICheats::GetEnemyUnits(int* unitIds, int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetEnemyUnits(unitIds, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int springLegacyAI::CAIAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetEnemyUnits(unitIds, pos, radius, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int springLegacyAI::CAIAICheats::GetNeutralUnits(int* unitIds, int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetNeutralUnits(unitIds, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int springLegacyAI::CAIAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetNeutralUnits(unitIds, pos, radius, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}

int springLegacyAI::CAIAICheats::GetFeatures(int* featureIds, int featureIds_max) {
	SetCheatsEnabled(true);
	const int numFeatures = aiCallback->GetFeatures(featureIds, featureIds_max);
	SetCheatsEnabled(false);
	return numFeatures;
}
int springLegacyAI::CAIAICheats::GetFeatures(int* featureIds, int featureIds_max, const float3& pos, float radius) {
	SetCheatsEnabled(true);
	const int numFeatures = aiCallback->GetFeatures(featureIds, featureIds_max, pos, radius);
	SetCheatsEnabled(false);
	return numFeatures;
}

int springLegacyAI::CAIAICheats::GetUnitTeam(int unitId) {
	SetCheatsEnabled(true);
	const int t = aiCallback->GetUnitTeam(unitId);
	SetCheatsEnabled(false);
	return t;
}
int springLegacyAI::CAIAICheats::GetUnitAllyTeam(int unitId) {
	SetCheatsEnabled(true);
	const int allyTeam = aiCallback->GetUnitAllyTeam(unitId);
	SetCheatsEnabled(false);
	return allyTeam;
}
float springLegacyAI::CAIAICheats::GetUnitHealth(int unitId) {
	SetCheatsEnabled(true);
	const float health = aiCallback->GetUnitHealth(unitId);
	SetCheatsEnabled(false);
	return health;
}
float springLegacyAI::CAIAICheats::GetUnitMaxHealth(int unitId) {
	SetCheatsEnabled(true);
	const float health = aiCallback->GetUnitMaxHealth(unitId);
	SetCheatsEnabled(false);
	return health;
}
float springLegacyAI::CAIAICheats::GetUnitPower(int unitId) {
	SetCheatsEnabled(true);
	const float power = aiCallback->GetUnitPower(unitId);
	SetCheatsEnabled(false);
	return power;
}
float springLegacyAI::CAIAICheats::GetUnitExperience(int unitId) {
	SetCheatsEnabled(true);
	const float experience = aiCallback->GetUnitExperience(unitId);
	SetCheatsEnabled(false);
	return experience;
}
bool springLegacyAI::CAIAICheats::IsUnitActivated(int unitId) {
	SetCheatsEnabled(true);
	const bool activated = aiCallback->IsUnitActivated(unitId);
	SetCheatsEnabled(false);
	return activated;
}
bool springLegacyAI::CAIAICheats::UnitBeingBuilt(int unitId) {
	SetCheatsEnabled(true);
	const bool isBeingBuilt = aiCallback->UnitBeingBuilt(unitId);
	SetCheatsEnabled(false);
	return isBeingBuilt;
}
bool springLegacyAI::CAIAICheats::IsUnitNeutral(int unitId) {
	SetCheatsEnabled(true);
	const bool neutral = aiCallback->IsUnitNeutral(unitId);
	SetCheatsEnabled(false);
	return neutral;
}
bool springLegacyAI::CAIAICheats::GetUnitResourceInfo(int unitId,
		UnitResourceInfo* resourceInfo) {
	SetCheatsEnabled(true);
	const bool fetchOk = aiCallback->GetUnitResourceInfo(unitId, resourceInfo);
	SetCheatsEnabled(false);
	return fetchOk;
}
const springLegacyAI::CCommandQueue* springLegacyAI::CAIAICheats::GetCurrentUnitCommands(int unitId) {
	SetCheatsEnabled(true);
	const CCommandQueue* cc = aiCallback->GetCurrentUnitCommands(unitId);
	SetCheatsEnabled(false);
	return cc;
}

int springLegacyAI::CAIAICheats::GetBuildingFacing(int unitId) {
	SetCheatsEnabled(true);
	const int facing = aiCallback->GetBuildingFacing(unitId);
	SetCheatsEnabled(false);
	return facing;
}
bool springLegacyAI::CAIAICheats::IsUnitCloaked(int unitId) {
	SetCheatsEnabled(true);
	const bool cloaked = aiCallback->IsUnitCloaked(unitId);
	SetCheatsEnabled(false);
	return cloaked;
}
bool springLegacyAI::CAIAICheats::IsUnitParalyzed(int unitId) {
	SetCheatsEnabled(true);
	const bool paralyzed = aiCallback->IsUnitParalyzed(unitId);
	SetCheatsEnabled(false);
	return paralyzed;
}


bool springLegacyAI::CAIAICheats::GetProperty(int id, int property, void* dst) {
//	SetCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetProperty(id, property, dst);
//	SetCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}
bool springLegacyAI::CAIAICheats::GetValue(int id, void* dst) {
//	SetCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetValue(id, dst);
//	SetCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}

int springLegacyAI::CAIAICheats::HandleCommand(int commandId, void* data) {
	SetCheatsEnabled(true);
	const int ret = aiCallback->HandleCommand(commandId, data);
	SetCheatsEnabled(false);
	return ret;
}
