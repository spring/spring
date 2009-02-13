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

#include "ExternalAI/Interface/SAICallback.h"
#include "AIAICallback.h"
#include "ExternalAI/Interface/AISCommands.h"

static int resIndMetal = -1;
static int resIndEnergy = -1;
static inline int getResourceIndex_Metal(SAICallback* sAICallback, int teamId) {

	if (resIndMetal == -1) {
		resIndMetal = sAICallback->Clb_0MULTI1FETCH3ResourceByName0Resource(
				teamId, "Metal");
	}

	return resIndMetal;
}
static inline int getResourceIndex_Energy(SAICallback* sAICallback, int teamId)
{
	if (resIndEnergy == -1) {
		resIndEnergy = sAICallback->Clb_0MULTI1FETCH3ResourceByName0Resource(
				teamId, "Energy");
	}

	return resIndEnergy;
}

CAIAICheats::CAIAICheats()
	: IAICheats(), teamId(-1), sAICallback(NULL), aiCallback(NULL) {}

CAIAICheats::CAIAICheats(int teamId, SAICallback* sAICallback,
		CAIAICallback* aiCallback)
		: IAICheats(), teamId(teamId), sAICallback(sAICallback),
		aiCallback(aiCallback) {}

void CAIAICheats::setCheatsEnabled(bool enabled) {
	sAICallback->Clb_Cheats_setEnabled(teamId, enabled);
}


void CAIAICheats::SetMyHandicap(float handicap) {
	setCheatsEnabled(true);
	SSetMyHandicapCheatCommand cmd = {handicap};
	sAICallback->Clb_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_SET_MY_HANDICAP, &cmd);
	setCheatsEnabled(false);
}
void CAIAICheats::GiveMeMetal(float amount) {

	static int m = getResourceIndex_Metal(sAICallback, teamId);
	setCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {m, amount};
	sAICallback->Clb_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	setCheatsEnabled(false);
}
void CAIAICheats::GiveMeEnergy(float amount) {

	static int e = getResourceIndex_Energy(sAICallback, teamId);
	setCheatsEnabled(true);
	SGiveMeResourceCheatCommand cmd = {e, amount};
	sAICallback->Clb_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1,
			COMMAND_CHEATS_GIVE_ME_RESOURCE, &cmd);
	setCheatsEnabled(false);
}

int CAIAICheats::CreateUnit(const char* unitDefName, float3 pos) {
	setCheatsEnabled(true);
	int unitDefId = sAICallback->Clb_0MULTI1FETCH3UnitDefByName0UnitDef(
			teamId, unitDefName);
	SGiveMeNewUnitCheatCommand cmd = {unitDefId, pos.toSAIFloat3()};
	int unitId = sAICallback->Clb_handleCommand(teamId, COMMAND_TO_ID_ENGINE,
			-1, COMMAND_CHEATS_GIVE_ME_NEW_UNIT, &cmd);
	setCheatsEnabled(false);
	return unitId;
}

const UnitDef* CAIAICheats::GetUnitDef(int unitId) {
	setCheatsEnabled(true);
	const UnitDef* unitDef = aiCallback->GetUnitDef(unitId);
	setCheatsEnabled(false);
	return unitDef;
}
float3 CAIAICheats::GetUnitPos(int unitId) {
	setCheatsEnabled(true);
	float3 pos = aiCallback->GetUnitPos(unitId);
	setCheatsEnabled(false);
	return pos;
}
int CAIAICheats::GetEnemyUnits(int* unitIds, int unitIds_max) {
	setCheatsEnabled(true);
	int numUnits = aiCallback->GetEnemyUnits(unitIds, unitIds_max);
	setCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	setCheatsEnabled(true);
	int numUnits = aiCallback->GetEnemyUnits(unitIds, pos, radius, unitIds_max);
	setCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds, int unitIds_max) {
	setCheatsEnabled(true);
	int numUnits = aiCallback->GetNeutralUnits(unitIds, unitIds_max);
	setCheatsEnabled(false);
	return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius,
		int unitIds_max) {
	setCheatsEnabled(true);
	int numUnits = aiCallback->GetNeutralUnits(unitIds, pos, radius,
			unitIds_max);
	setCheatsEnabled(false);
	return numUnits;
}

int CAIAICheats::GetUnitTeam(int unitId) {
	setCheatsEnabled(true);
	int t = aiCallback->GetUnitTeam(unitId);
	setCheatsEnabled(false);
	return t;
}
int CAIAICheats::GetUnitAllyTeam(int unitId) {
	setCheatsEnabled(true);
	int t = aiCallback->GetUnitAllyTeam(unitId);
	setCheatsEnabled(false);
	return t;
}
float CAIAICheats::GetUnitHealth(int unitId) {
	setCheatsEnabled(true);
	float health = aiCallback->GetUnitHealth(unitId);
	setCheatsEnabled(false);
	return health;
}
float CAIAICheats::GetUnitMaxHealth(int unitId) {
	setCheatsEnabled(true);
	float health = aiCallback->GetUnitMaxHealth(unitId);
	setCheatsEnabled(false);
	return health;
}
float CAIAICheats::GetUnitPower(int unitId) {
	setCheatsEnabled(true);
	float power = aiCallback->GetUnitPower(unitId);
	setCheatsEnabled(false);
	return power;
}
float CAIAICheats::GetUnitExperience(int unitId) {
	setCheatsEnabled(true);
	float experience = aiCallback->GetUnitExperience(unitId);
	setCheatsEnabled(false);
	return experience;
}
bool CAIAICheats::IsUnitActivated(int unitId) {
	setCheatsEnabled(true);
	bool activated = aiCallback->IsUnitActivated(unitId);
	setCheatsEnabled(false);
	return activated;
}
bool CAIAICheats::UnitBeingBuilt(int unitId) {
	setCheatsEnabled(true);
	bool isBeingBuilt = aiCallback->UnitBeingBuilt(unitId);
	setCheatsEnabled(false);
	return isBeingBuilt;
}
bool CAIAICheats::IsUnitNeutral(int unitId) {
	setCheatsEnabled(true);
	bool neutral = aiCallback->IsUnitNeutral(unitId);
	setCheatsEnabled(false);
	return neutral;
}
bool CAIAICheats::GetUnitResourceInfo(int unitId,
		UnitResourceInfo* resourceInfo) {
	setCheatsEnabled(true);
	bool fetchOk = aiCallback->GetUnitResourceInfo(unitId, resourceInfo);
	setCheatsEnabled(false);
	return fetchOk;
}
const CCommandQueue* CAIAICheats::GetCurrentUnitCommands(int unitId) {
	setCheatsEnabled(true);
	const CCommandQueue* cc = aiCallback->GetCurrentUnitCommands(unitId);
	setCheatsEnabled(false);
	return cc;
}

int CAIAICheats::GetBuildingFacing(int unitId) {
	setCheatsEnabled(true);
	int facing = aiCallback->GetBuildingFacing(unitId);
	setCheatsEnabled(false);
	return facing;
}
bool CAIAICheats::IsUnitCloaked(int unitId) {
	setCheatsEnabled(true);
	bool cloaked = aiCallback->IsUnitCloaked(unitId);
	setCheatsEnabled(false);
	return cloaked;
}
bool CAIAICheats::IsUnitParalyzed(int unitId) {
	setCheatsEnabled(true);
	bool paralyzed = aiCallback->IsUnitParalyzed(unitId);
	setCheatsEnabled(false);
	return paralyzed;
}

bool CAIAICheats::OnlyPassiveCheats() {
	return sAICallback->Clb_Cheats_isOnlyPassive(teamId);
}
void CAIAICheats::EnableCheatEvents(bool enable) {
	sAICallback->Clb_Cheats_setEventsEnabled(teamId, enable);
}

bool CAIAICheats::GetProperty(int id, int property, void* dst) {
//	setCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetProperty(id, property, dst);
//	setCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}
bool CAIAICheats::GetValue(int id, void* dst) {
//	setCheatsEnabled(true);
//	bool fetchOk = aiCallback->GetValue(id, dst);
//	setCheatsEnabled(false);
//	return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}

int CAIAICheats::HandleCommand(int commandId, void* data) {
	setCheatsEnabled(true);
	int ret = aiCallback->HandleCommand(commandId, data);
	setCheatsEnabled(false);
	return ret;
}
