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

#include "ExternalAI/Interface/AISCommands.h"


CAIAICheats::CAIAICheats()
    : IAICheats(), teamId(-1), sAICallback(NULL)/*, aiCheatCallback(NULL)*/, aiCallback(NULL) {
    
}

CAIAICheats::CAIAICheats(int teamId, SAICallback* sAICallback/*, IAICheats* aiCheatCallback*/, CAIAICallback* aiCallback)
    : IAICheats(), teamId(teamId), sAICallback(sAICallback)/*, aiCheatCallback(aiCheatCallback)*/, aiCallback(aiCallback) {
    
}

void CAIAICheats::setCheatsEnabled(bool enabled) {
    sAICallback->Cheats_setEnabled(teamId, enabled);
}


void CAIAICheats::SetMyHandicap(float handicap) {
    setCheatsEnabled(true);
    SSetMyHandicapCheatCommand cmd = {handicap};
    sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_SET_MY_HANDICAP, &cmd);
    setCheatsEnabled(false);
}
void CAIAICheats::GiveMeMetal(float amount) {
    setCheatsEnabled(true);
    SGiveMeMetalCheatCommand cmd = {amount};
    sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_GIVE_ME_METAL, &cmd);
    setCheatsEnabled(false);
}
void CAIAICheats::GiveMeEnergy(float amount) {
    setCheatsEnabled(true);
    SGiveMeEnergyCheatCommand cmd = {amount};
    sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_GIVE_ME_ENERGY, &cmd);
    setCheatsEnabled(false);
}

int CAIAICheats::CreateUnit(const char* unitDefName, float3 pos) {
    setCheatsEnabled(true);
    int unitDefId = sAICallback->UnitDef_STATIC_getIdByName(teamId, unitDefName);
    SGiveMeNewUnitCheatCommand cmd = {unitDefId, pos.toSAIFloat3()};
    int unitId = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CHEATS_GIVE_ME_NEW_UNIT, &cmd);
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
int CAIAICheats::GetEnemyUnits(int* unitIds) {
    setCheatsEnabled(true);
    int numUnits = aiCallback->GetEnemyUnits(unitIds);
    setCheatsEnabled(false);
    return numUnits;
}
int CAIAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius) {
    setCheatsEnabled(true);
    int numUnits = aiCallback->GetEnemyUnits(unitIds, pos, radius);
    setCheatsEnabled(false);
    return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds) {
    setCheatsEnabled(true);
    int numUnits = aiCallback->GetNeutralUnits(unitIds);
    setCheatsEnabled(false);
    return numUnits;
}
int CAIAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius) {
    setCheatsEnabled(true);
    int numUnits = aiCallback->GetNeutralUnits(unitIds, pos, radius);
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
bool CAIAICheats::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {
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
    return sAICallback->Cheats_isOnlyPassive(teamId);
}
void CAIAICheats::EnableCheatEvents(bool enable) {
    sAICallback->Cheats_setEventsEnabled(teamId, enable);
}

bool CAIAICheats::GetProperty(int id, int property, void* dst) {
//    setCheatsEnabled(true);
//    bool fetchOk = aiCallback->GetProperty(id, property, dst);
//    setCheatsEnabled(false);
//    return fetchOk;
	// this returns always false, cause these values are now available through
	// individual callback functions -> this method is deprecated
	return false;
}
bool CAIAICheats::GetValue(int id, void* dst) {
//    setCheatsEnabled(true);
//    bool fetchOk = aiCallback->GetValue(id, dst);
//    setCheatsEnabled(false);
//    return fetchOk;
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
