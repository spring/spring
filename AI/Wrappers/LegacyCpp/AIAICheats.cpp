/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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

CAIAICheats::CAIAICheats()
		: IAICheats(), skirmishAIId(-1), sAICallback(NULL), aiCallback(NULL) {}

CAIAICheats::CAIAICheats(int skirmishAIId, const SSkirmishAICallback* sAICallback, CAIAICallback* aiCallback)
		: IAICheats(), skirmishAIId(skirmishAIId), sAICallback(sAICallback), aiCallback(aiCallback) {}

void CAIAICheats::SetCheatsEnabled(bool enabled) {
	sAICallback->Cheats_setEnabled(skirmishAIId, enabled);
}

void CAIAICheats::EnableCheatEvents(bool enable) {

	SetCheatsEnabled(true);
	sAICallback->Cheats_setEventsEnabled(skirmishAIId, enable);
	SetCheatsEnabled(false);
}


void CAIAICheats::SetMyHandicap(float handicap) {
	SetCheatsEnabled(true);
	SSetMyIncomeMultiplierCheatCommand cmd = { (handicap / 100.0f) + 1.0f };
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER, &cmd);
	SetCheatsEnabled(false);
}
void CAIAICheats::GiveMeMetal(float amount) {
	const static int m = getResourceIndex_Metal(sAICallback, skirmishAIId);

	SetCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {m, amount};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	SetCheatsEnabled(false);
}
void CAIAICheats::GiveMeEnergy(float amount) {
	const static int e = getResourceIndex_Energy(sAICallback, skirmishAIId);

	SetCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {e, amount};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	SetCheatsEnabled(false);
}

int CAIAICheats::CreateUnit(const char* unitDefName, float3 pos) {
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

const UnitDef* CAIAICheats::GetUnitDef(int unitId) {
	SetCheatsEnabled(true);
	const UnitDef* unitDef = aiCallback->GetUnitDef(unitId);
	SetCheatsEnabled(false);
	return unitDef;
}



float3 CAIAICheats::GetUnitPos(int unitId) {
	SetCheatsEnabled(true);
	const float3 pos = aiCallback->GetUnitPos(unitId);
	SetCheatsEnabled(false);
	return pos;
}

float3 CAIAICheats::GetUnitVel(int unitId) {
	SetCheatsEnabled(true);
	const float3 pos = aiCallback->GetUnitVel(unitId);
	SetCheatsEnabled(false);
	return pos;
}



int CAIAICheats::GetEnemyUnits(int* unitIds, int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetEnemyUnits(unitIds, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetEnemyUnits(unitIds, pos, radius, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds, int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetNeutralUnits(unitIds, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	SetCheatsEnabled(true);
	const int numUnits = aiCallback->GetNeutralUnits(unitIds, pos, radius, unitIds_max);
	SetCheatsEnabled(false);
	return numUnits;
}

int CAIAICheats::GetFeatures(int* featureIds, int featureIds_max) {
	SetCheatsEnabled(true);
	const int numFeatures = aiCallback->GetFeatures(featureIds, featureIds_max);
	SetCheatsEnabled(false);
	return numFeatures;
}
int CAIAICheats::GetFeatures(int* featureIds, int featureIds_max, const float3& pos, float radius) {
	SetCheatsEnabled(true);
	const int numFeatures = aiCallback->GetFeatures(featureIds, featureIds_max, pos, radius);
	SetCheatsEnabled(false);
	return numFeatures;
}

int CAIAICheats::GetUnitTeam(int unitId) {
	SetCheatsEnabled(true);
	const int t = aiCallback->GetUnitTeam(unitId);
	SetCheatsEnabled(false);
	return t;
}
int CAIAICheats::GetUnitAllyTeam(int unitId) {
	SetCheatsEnabled(true);
	const int allyTeam = aiCallback->GetUnitAllyTeam(unitId);
	SetCheatsEnabled(false);
	return allyTeam;
}
float CAIAICheats::GetUnitHealth(int unitId) {
	SetCheatsEnabled(true);
	const float health = aiCallback->GetUnitHealth(unitId);
	SetCheatsEnabled(false);
	return health;
}
float CAIAICheats::GetUnitMaxHealth(int unitId) {
	SetCheatsEnabled(true);
	const float health = aiCallback->GetUnitMaxHealth(unitId);
	SetCheatsEnabled(false);
	return health;
}
float CAIAICheats::GetUnitPower(int unitId) {
	SetCheatsEnabled(true);
	const float power = aiCallback->GetUnitPower(unitId);
	SetCheatsEnabled(false);
	return power;
}
float CAIAICheats::GetUnitExperience(int unitId) {
	SetCheatsEnabled(true);
	const float experience = aiCallback->GetUnitExperience(unitId);
	SetCheatsEnabled(false);
	return experience;
}
bool CAIAICheats::IsUnitActivated(int unitId) {
	SetCheatsEnabled(true);
	const bool activated = aiCallback->IsUnitActivated(unitId);
	SetCheatsEnabled(false);
	return activated;
}
bool CAIAICheats::UnitBeingBuilt(int unitId) {
	SetCheatsEnabled(true);
	const bool isBeingBuilt = aiCallback->UnitBeingBuilt(unitId);
	SetCheatsEnabled(false);
	return isBeingBuilt;
}
bool CAIAICheats::IsUnitNeutral(int unitId) {
	SetCheatsEnabled(true);
	const bool neutral = aiCallback->IsUnitNeutral(unitId);
	SetCheatsEnabled(false);
	return neutral;
}
bool CAIAICheats::GetUnitResourceInfo(int unitId,
		UnitResourceInfo* resourceInfo) {
	SetCheatsEnabled(true);
	const bool fetchOk = aiCallback->GetUnitResourceInfo(unitId, resourceInfo);
	SetCheatsEnabled(false);
	return fetchOk;
}
const CCommandQueue* CAIAICheats::GetCurrentUnitCommands(int unitId) {
	SetCheatsEnabled(true);
	const CCommandQueue* cc = aiCallback->GetCurrentUnitCommands(unitId);
	SetCheatsEnabled(false);
	return cc;
}

int CAIAICheats::GetBuildingFacing(int unitId) {
	SetCheatsEnabled(true);
	const int facing = aiCallback->GetBuildingFacing(unitId);
	SetCheatsEnabled(false);
	return facing;
}
bool CAIAICheats::IsUnitCloaked(int unitId) {
	SetCheatsEnabled(true);
	const bool cloaked = aiCallback->IsUnitCloaked(unitId);
	SetCheatsEnabled(false);
	return cloaked;
}
bool CAIAICheats::IsUnitParalyzed(int unitId) {
	SetCheatsEnabled(true);
	const bool paralyzed = aiCallback->IsUnitParalyzed(unitId);
	SetCheatsEnabled(false);
	return paralyzed;
}


bool CAIAICheats::GetProperty(int id, int property, void* dst) {
//	SetCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetProperty(id, property, dst);
//	SetCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}
bool CAIAICheats::GetValue(int id, void* dst) {
//	SetCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetValue(id, dst);
//	SetCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}

int CAIAICheats::HandleCommand(int commandId, void* data) {
	SetCheatsEnabled(true);
	const int ret = aiCallback->HandleCommand(commandId, data);
	SetCheatsEnabled(false);
	return ret;
}
