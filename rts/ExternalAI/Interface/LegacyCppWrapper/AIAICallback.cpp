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

#include "AIAICallback.h"

#include "ExternalAI/Interface/AISCommands.h"

#include "creg/creg_cond.h"
#ifdef USING_CREG
creg::Class* CCommandQueue::GetClass() { return NULL; }
#endif
#include "Sim/Units/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"
UnitDef::~UnitDef() {
	delete movedata;
}
CIcon::CIcon() {}
CIcon::~CIcon() {}
UnitDef::UnitDefWeapon::UnitDefWeapon() {}
#include "Sim/Features/FeatureDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"
WeaponDef::~WeaponDef() {}

#include <string>


CAIAICallback::CAIAICallback()
	: IAICallback(), teamId(-1), sAICallback(NULL)/*, aiCallback(NULL)*/ {
	init();
}

CAIAICallback::CAIAICallback(int teamId, SAICallback* sAICallback/*, IAICallback* aiCallback*/)
	: IAICallback(), teamId(teamId), sAICallback(sAICallback)/*, aiCallback(aiCallback)*/ {
	init();
}


void fillWithNULL(void** arr, int size) {
	for (int i=0; i < size; ++i) {
		arr[i] = NULL;
	}
}
void fillWithMinusOne(int* arr, int size) {
	for (int i=0; i < size; ++i) {
		arr[i] = -1;
	}
}

void CAIAICallback::init() {
	
	// init caches
	int maxCacheSize = 512;
	int maxUnits = 10000;
	int maxGroups = 100;
	
	weaponDefs = new WeaponDef*[maxCacheSize]; fillWithNULL((void**)weaponDefs, maxCacheSize);
	weaponDefFrames = new int[maxCacheSize]; fillWithMinusOne(weaponDefFrames, maxCacheSize);
	
	unitDefs = new UnitDef*[maxCacheSize]; fillWithNULL((void**)unitDefs, maxCacheSize);
	unitDefFrames = new int[maxCacheSize]; fillWithMinusOne(unitDefFrames, maxCacheSize);
	groupPossibleCommands = new std::vector<CommandDescription>*[maxGroups]; fillWithNULL((void**)groupPossibleCommands, maxGroups);
	unitPossibleCommands = new std::vector<CommandDescription>*[maxUnits]; fillWithNULL((void**)unitPossibleCommands, maxUnits);
	unitCurrentCommandQueues = new CCommandQueue*[maxUnits]; fillWithNULL((void**)unitCurrentCommandQueues, maxUnits);
	
	featureDefs = new FeatureDef*[maxCacheSize]; fillWithNULL((void**)featureDefs, maxCacheSize);
	featureDefFrames = new int[maxCacheSize]; fillWithMinusOne(featureDefFrames, maxCacheSize);
}


//bool CAIAICallback::PosInCamera(float3 pos, float radius) {
//	return aiCallback->PosInCamera(pos, radius);
//}
//
//int CAIAICallback::GetCurrentFrame() {
//	return aiCallback->GetCurrentFrame();
//}
//
//int CAIAICallback::GetMyTeam() {
//	return aiCallback->GetMyTeam();
//}
//
//int CAIAICallback::GetMyAllyTeam() {
//	return aiCallback->GetMyAllyTeam();
//}
//
//int CAIAICallback::GetPlayerTeam(int player) {
//	return aiCallback->GetPlayerTeam(player);
//}
//
//const char* CAIAICallback::GetTeamSide(int team) {
//	return aiCallback->GetTeamSide(team);
//}
//
//int CAIAICallback::GetUnitGroup(int unitId) {
//	return aiCallback->GetUnitGroup(unitId);
//}
//
//int CAIAICallback::GetUnitAiHint(int unitId) {
//	return aiCallback->GetUnitAiHint(unitId);
//}
//
//int CAIAICallback::GetUnitTeam(int unitId) {
//	return aiCallback->GetUnitTeam(unitId);
//}
//
//int CAIAICallback::GetUnitAllyTeam(int unitId) {
//	return aiCallback->GetUnitAllyTeam(unitId);
//}
//
//float CAIAICallback::GetUnitHealth(int unitId) {
//	return aiCallback->GetUnitHealth(unitId);
//}
//
//float CAIAICallback::GetUnitMaxHealth(int unitId) {
//	return aiCallback->GetUnitMaxHealth(unitId);
//}
//
//float CAIAICallback::GetUnitSpeed(int unitId) {
//	return aiCallback->GetUnitSpeed(unitId);
//}
//
//float CAIAICallback::GetUnitPower(int unitId) {
//	return aiCallback->GetUnitPower(unitId);
//}
//
//float CAIAICallback::GetUnitExperience(int unitId) {
//	return aiCallback->GetUnitExperience(unitId);
//}
//
//float CAIAICallback::GetUnitMaxRange(int unitId) {
//	return aiCallback->GetUnitMaxRange(unitId);
//}
//
//bool CAIAICallback::IsUnitActivated(int unitId) {
//	return aiCallback->IsUnitActivated(unitId);
//}
//
//bool CAIAICallback::UnitBeingBuilt(int unitId) {
//	return aiCallback->UnitBeingBuilt(unitId);
//}
//
//float3 CAIAICallback::GetUnitPos(int unitId) {
//	return aiCallback->GetUnitPos(unitId);
//}
//
//int CAIAICallback::GetBuildingFacing(int unitId) {
//	return aiCallback->GetBuildingFacing(unitId);
//}
//
//bool CAIAICallback::IsUnitCloaked(int unitId) {
//	return aiCallback->IsUnitCloaked(unitId);
//}
//
//bool CAIAICallback::IsUnitParalyzed(int unitId) {
//	return aiCallback->IsUnitParalyzed(unitId);
//}
//
//bool CAIAICallback::IsUnitNeutral(int unitId) {
//	return aiCallback->IsUnitNeutral(unitId);
//}
//
//bool CAIAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {
//	return aiCallback->GetUnitResourceInfo(unitId, resourceInfo);
//}
//
//float3 CAIAICallback::GetNextWaypoint(int pathid) {
//	return aiCallback->GetNextWaypoint(pathid);
//}
//
//float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType) {
//	return aiCallback->GetPathLength(start, end, pathType);
//}
//
//int CAIAICallback::GetEnemyUnits(int* unitIds) {
//	return aiCallback->GetEnemyUnits(unitIds);
//}
//
//int CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius) {
//	return aiCallback->GetEnemyUnits(unitIds, pos, radius);
//}
//
//int CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds) {
//	return aiCallback->GetEnemyUnitsInRadarAndLos(unitIds);
//}
//
//int CAIAICallback::GetFriendlyUnits(int* unitIds) {
//	return aiCallback->GetFriendlyUnits(unitIds);
//}
//
//int CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius) {
//	return aiCallback->GetFriendlyUnits(unitIds, pos, radius);
//}
//
//int CAIAICallback::GetNeutralUnits(int* unitIds) {
//	return aiCallback->GetNeutralUnits(unitIds);
//}
//
//int CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius) {
//	return aiCallback->GetNeutralUnits(unitIds, pos, radius);
//}
//
//int CAIAICallback::GetMapWidth() {
//	return aiCallback->GetMapWidth();
//}
//
//int CAIAICallback::GetMapHeight() {
//	return aiCallback->GetMapHeight();
//}
//
//const float* CAIAICallback::GetHeightMap() {
//	return aiCallback->GetHeightMap();
//}
//
//float CAIAICallback::GetMinHeight() {
//	return aiCallback->GetMinHeight();
//}
//
//float CAIAICallback::GetMaxHeight() {
//	return aiCallback->GetMaxHeight();
//}
//
//const float* CAIAICallback::GetSlopeMap() {
//	return aiCallback->GetSlopeMap();
//}
//
//const unsigned short* CAIAICallback::GetLosMap() {
//	return aiCallback->GetLosMap();
//}
//
//const unsigned short* CAIAICallback::GetRadarMap() {
//	return aiCallback->GetRadarMap();
//}
//
//const unsigned short* CAIAICallback::GetJammerMap() {
//	return aiCallback->GetJammerMap();
//}
//
//const unsigned char* CAIAICallback::GetMetalMap() {
//	return aiCallback->GetMetalMap();
//}
//
//const char* CAIAICallback::GetMapName() {
//	return aiCallback->GetMapName();
//}
//
//const char* CAIAICallback::GetModName() {
//	return aiCallback->GetModName();
//}
//
//float CAIAICallback::GetElevation(float x, float z) {
//	return aiCallback->GetElevation(x, z);
//}
//
//float CAIAICallback::GetMaxMetal() {
//	return aiCallback->GetMaxMetal();
//}
//
//float CAIAICallback::GetExtractorRadius() {
//	return aiCallback->GetExtractorRadius();
//}
//
//float CAIAICallback::GetMinWind() {
//	return aiCallback->GetMinWind();
//}
//
//float CAIAICallback::GetMaxWind() {
//	return aiCallback->GetMaxWind();
//}
//
//float CAIAICallback::GetTidalStrength() {
//	return aiCallback->GetTidalStrength();
//}
//
//float CAIAICallback::GetGravity() {
//	return aiCallback->GetGravity();
//}
//
//bool CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {
//	return aiCallback->CanBuildAt(unitDef, pos, facing);
//}
//
//float3 CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing) {
//	return aiCallback->ClosestBuildSite(unitDef, pos, searchRadius, minDist, facing);
//}
//
//bool CAIAICallback::GetProperty(int id, int property, void* dst) {
//	return aiCallback->GetProperty(id, property, dst);
//}
//
//bool CAIAICallback::GetValue(int id, void* dst) {
//	return aiCallback->GetValue(id, dst);
//}
//
//int CAIAICallback::GetFileSize(const char* name) {
//	return aiCallback->GetFileSize(name);
//}
//
//int CAIAICallback::GetSelectedUnits(int* unitIds) {
//	return aiCallback->GetSelectedUnits(unitIds);
//}
//
//float3 CAIAICallback::GetMousePos() {
//	return aiCallback->GetMousePos();
//}
//
//float CAIAICallback::GetMetal() {
//	return aiCallback->GetMetal();
//}
//
//float CAIAICallback::GetMetalIncome() {
//	return aiCallback->GetMetalIncome();
//}
//
//float CAIAICallback::GetMetalUsage() {
//	return aiCallback->GetMetalUsage();
//}
//
//float CAIAICallback::GetMetalStorage() {
//	return aiCallback->GetMetalStorage();
//}
//
//float CAIAICallback::GetEnergy() {
//	return aiCallback->GetEnergy();
//}
//
//float CAIAICallback::GetEnergyIncome() {
//	return aiCallback->GetEnergyIncome();
//}
//
//float CAIAICallback::GetEnergyUsage() {
//	return aiCallback->GetEnergyUsage();
//}
//
//float CAIAICallback::GetEnergyStorage() {
//	return aiCallback->GetEnergyStorage();
//}
//
//int CAIAICallback::GetFeatures(int *features, int max) {
//	return aiCallback->GetFeatures(features, max);
//}
//
//int CAIAICallback::GetFeatures(int *features, int max, const float3& pos, float radius) {
//	return aiCallback->GetFeatures(features, max, pos, radius);
//}
//
//const FeatureDef* CAIAICallback::GetFeatureDef(int featureId) {
//	return aiCallback->GetFeatureDef(featureId);
//}
//
//float CAIAICallback::GetFeatureHealth(int featureId) {
//	return aiCallback->GetFeatureHealth(featureId);
//}
//
//float CAIAICallback::GetFeatureReclaimLeft(int featureId) {
//	return aiCallback->GetFeatureReclaimLeft(featureId);
//}
//
//float3 CAIAICallback::GetFeaturePos(int featureId) {
//	return aiCallback->GetFeaturePos(featureId);
//}
//
//int CAIAICallback::GetNumUnitDefs() {
//	return aiCallback->GetNumUnitDefs();
//}
//
//void CAIAICallback::GetUnitDefList(const UnitDef** list) {
//	aiCallback->GetUnitDefList(list);
//}

//float CAIAICallback::GetUnitDefHeight(int def) {
//	return aiCallback->GetUnitDefHeight(def);
//}
//
//float CAIAICallback::GetUnitDefRadius(int def) {
//	return aiCallback->GetUnitDefRadius(def);
//}
//
//const float3* CAIAICallback::GetStartPos() {
//	return aiCallback->GetStartPos();
//}
//
//
//
//const WeaponDef* CAIAICallback::GetWeapon(const char* weaponname) {
//	logT("CAIAICallback::GetWeapon() aiCallback");
//	return aiCallback->GetWeapon(weaponname);
//}
//
//const WeaponDef* CAIAICallback::GetWeaponDefById(int weaponDefId) {
//	logT("CAIAICallback::GetWeaponDefById() return NULL");
//	return NULL;
//}
//
//const FeatureDef* CAIAICallback::GetFeatureDefById(int featureDefId) {
//	logT("CAIAICallback::GetFeatureDefById() aiCallback");
//	return aiCallback->GetFeatureDefById(featureDefId);
//}
//
//int CAIAICallback::GetMapPoints(PointMarker* pm, int maxPoints) {
//	return aiCallback->GetMapPoints(pm, maxPoints);
//}
//
//int CAIAICallback::GetMapLines(LineMarker* lm, int maxLines) {
//	return aiCallback->GetMapLines(lm, maxLines);
//}
//
//const UnitDef* CAIAICallback::GetUnitDef(const char* unitName) {
//	return aiCallback->GetUnitDef(unitName);
//}

//const UnitDef* CAIAICallback::GetUnitDefById(int unitDefId) {
//	return aiCallback->GetUnitDefById(unitDefId);
//}

//const std::vector<CommandDescription>* CAIAICallback::GetGroupCommands(int unitId) {
//	return aiCallback->GetGroupCommands(unitId);
//}
//
//const std::vector<CommandDescription>* CAIAICallback::GetUnitCommands(int unitId) {
//	return aiCallback->GetUnitCommands(unitId);
//}
//
//const CCommandQueue* CAIAICallback::GetCurrentUnitCommands(int unitId) {
//	return aiCallback->GetCurrentUnitCommands(unitId);
//}
//
//const UnitDef* CAIAICallback::GetUnitDef(int unitId) {
//	return aiCallback->GetUnitDef(unitId);
//}







//void CAIAICallback::SendTextMsg(const char* text, int zone) {
//	aiCallback->SendTextMsg(text, zone);
//}
//
//void CAIAICallback::SetLastMsgPos(float3 pos) {
//	aiCallback->SetLastMsgPos(pos);
//}
//
//void CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {
//	aiCallback->AddNotification(pos, color, alpha);
//}
//
//bool CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {
//	return aiCallback->SendResources(mAmount, eAmount, receivingTeam);
//}
//
//int CAIAICallback::SendUnits(const std::vector<int>& unitIDs, int receivingTeam) {
//	return aiCallback->SendUnits(unitIDs, receivingTeam);
//}
//
//void* CAIAICallback::CreateSharedMemArea(char* name, int size) {
//	return aiCallback->CreateSharedMemArea(name, size);
//}
//
//void CAIAICallback::ReleasedSharedMemArea(char* name) {
//	aiCallback->ReleasedSharedMemArea(name);
//}
//
//int CAIAICallback::CreateGroup(char* dll, unsigned aiNumber) {
//	return aiCallback->CreateGroup(dll, aiNumber);
//}
//
//void CAIAICallback::EraseGroup(int groupid) {
//	aiCallback->EraseGroup(groupid);
//}
//
//bool CAIAICallback::AddUnitToGroup(int unitId, int groupid) {
//	return aiCallback->AddUnitToGroup(unitId, groupid);
//}
//
//bool CAIAICallback::RemoveUnitFromGroup(int unitId) {
//	return aiCallback->RemoveUnitFromGroup(unitId);
//}
//
//int CAIAICallback::GiveGroupOrder(int unitId, Command* c) {
//	return aiCallback->GiveGroupOrder(unitId, c);
//}
//
//int CAIAICallback::GiveOrder(int unitId, Command* c) {
//	return aiCallback->GiveOrder(unitId, c);
//}
//
//int CAIAICallback::InitPath(float3 start, float3 end, int pathType) {
//	return aiCallback->InitPath(start, end, pathType);
//}
//
//float3 CAIAICallback::GetNextWaypoint(int pathId) {
//	return aiCallback->GetNextWaypoint(pathId);
//}
//
//float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType) {
//	return aiCallback->GetPathLength(start, end, pathType);
//}
//
//void CAIAICallback::FreePath(int pathid) {
//	aiCallback->FreePath(pathid);
//}
//
//void CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {
//	aiCallback->LineDrawerStartPath(pos, color);
//}
//
//void CAIAICallback::LineDrawerFinishPath() {
//	aiCallback->LineDrawerFinishPath();
//}
//
//void CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {
//	aiCallback->LineDrawerDrawLine(endPos, color);
//}
//
//void CAIAICallback::LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos, const float* color) {
//	aiCallback->LineDrawerDrawLineAndIcon(cmdID, endPos, color);
//}
//
//void CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdID) {
//	aiCallback->LineDrawerDrawIconAtLastPos(cmdID);
//}
//
//void CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {
//	aiCallback->LineDrawerBreak(endPos, color);
//}
//
//void CAIAICallback::LineDrawerRestart() {
//	aiCallback->LineDrawerRestart();
//}
//
//void CAIAICallback::LineDrawerRestartSameColor() {
//	aiCallback->LineDrawerRestartSameColor();
//}
//
//int CAIAICallback::CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifetime, int group) {
//	return aiCallback->CreateSplineFigure(pos1, pos2, pos3, pos4, width, arrow, lifetime, group);
//}
//
//int CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifetime, int group) {
//	return aiCallback->CreateLineFigure(pos1, pos2, width, arrow, lifetime, group);
//}
//
//void CAIAICallback::SetFigureColor(int group, float red, float green, float blue, float alpha) {
//	aiCallback->SetFigureColor(group, red, green, blue, alpha);
//}
//
//void CAIAICallback::DeleteFigureGroup(int group) {
//	aiCallback->DeleteFigureGroup(group);
//}
//
//void CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifetime, int team, bool transparent, bool drawBorder, int facing) {
//	aiCallback->DrawUnit(name, pos, rotation, lifetime, team, transparent, drawBorder, facing);
//}
//
//int CAIAICallback::HandleCommand(int commandId, void* data) {
//	return aiCallback->HandleCommand(commandId, data);
//}
//
//bool CAIAICallback::ReadFile(const char* name, void* buffer, int bufferLen) {
//	return aiCallback->ReadFile(name, buffer, bufferLen);
//}
//
//const char* CAIAICallback::CallLuaRules(const char* data, int inSize, int* outSize) {
//	return aiCallback->CallLuaRules(data, inSize, outSize);
//}
// ################################ <- OLD impl ################################




// ################################ NEW impl -> ################################
bool CAIAICallback::PosInCamera(float3 pos, float radius) {
	return sAICallback->Map_isPosInCamera(teamId, pos.toSAIFloat3(), radius);
}

int CAIAICallback::GetCurrentFrame() {
	return sAICallback->Game_getCurrentFrame(teamId);
}

int CAIAICallback::GetMyTeam() {
	return sAICallback->Game_getMyTeam(teamId);
}

int CAIAICallback::GetMyAllyTeam() {
	return sAICallback->Game_getMyAllyTeam(teamId);
}

int CAIAICallback::GetPlayerTeam(int player) {
	return sAICallback->Game_getPlayerTeam(teamId, player);
}

const char* CAIAICallback::GetTeamSide(int team) {
	return sAICallback->Game_getTeamSide(teamId, team);
}

int CAIAICallback::GetUnitGroup(int unitId) {
	return sAICallback->Unit_getGroup(teamId, unitId);
}


const std::vector<CommandDescription>* CAIAICallback::GetGroupCommands(int groupId) {
	int numCmds = sAICallback->Group_getNumSupportedCommands(teamId, groupId);
int ids[numCmds];
const char* names[numCmds];
const char* toolTips[numCmds];
bool showUniques[numCmds];
bool disableds[numCmds];
int numParams[numCmds];
const char** params[numCmds];
sAICallback->Group_SupportedCommands_getId(teamId, groupId, ids);
sAICallback->Group_SupportedCommands_getName(teamId, groupId, names);
sAICallback->Group_SupportedCommands_getToolTip(teamId, groupId, toolTips);
sAICallback->Group_SupportedCommands_isShowUnique(teamId, groupId, showUniques);
sAICallback->Group_SupportedCommands_isDisabled(teamId, groupId, disableds);
sAICallback->Group_SupportedCommands_getNumParams(teamId, groupId, numParams);
for (int c=0; c < numCmds; c++) {
	params[c] = new const char*[numParams[c]];
}
sAICallback->Group_SupportedCommands_getParams(teamId, groupId, params);
	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = ids[c];
		commandDescription.name = names[c];
		commandDescription.tooltip = toolTips[c];
		commandDescription.showUnique = showUniques[c];
		commandDescription.disabled = disableds[c];
		for (int p=0; p < numParams[c]; p++) {
			commandDescription.params.push_back(params[c][p]);
		}
		cmdDescVec->push_back(commandDescription);
	}
	
	// to prevent memory wholes
	if (groupPossibleCommands[groupId] != NULL) {
		delete groupPossibleCommands[groupId];
	}
	groupPossibleCommands[groupId] = cmdDescVec;
	
	return cmdDescVec;
}


const std::vector<CommandDescription>* CAIAICallback::GetUnitCommands(int unitId) {
	int numCmds = sAICallback->Unit_getNumSupportedCommands(teamId, unitId);
int* ids = new int[numCmds];
const char* names[numCmds];
const char* toolTips[numCmds];
bool showUniques[numCmds];
bool disableds[numCmds];
int numParams[numCmds];
const char** params[numCmds];
sAICallback->Unit_SupportedCommands_getId(teamId, unitId, ids);
sAICallback->Unit_SupportedCommands_getName(teamId, unitId, names);
sAICallback->Unit_SupportedCommands_getToolTip(teamId, unitId, toolTips);
sAICallback->Unit_SupportedCommands_isShowUnique(teamId, unitId, showUniques);
sAICallback->Unit_SupportedCommands_isDisabled(teamId, unitId, disableds);
sAICallback->Unit_SupportedCommands_getNumParams(teamId, unitId, numParams);
for (int c=0; c < numCmds; c++) {
	params[c] = new const char*[numParams[c]];
}
sAICallback->Unit_SupportedCommands_getParams(teamId, unitId, params);
	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = ids[c];
		commandDescription.name = names[c];
		commandDescription.tooltip = toolTips[c];
		commandDescription.showUnique = showUniques[c];
		commandDescription.disabled = disableds[c];
		for (int p=0; p < numParams[c]; p++) {
			commandDescription.params.push_back(params[c][p]);
		}
		cmdDescVec->push_back(commandDescription);
	}
	
	// to prevent memory wholes
	if (unitPossibleCommands[unitId] != NULL) {
		delete unitPossibleCommands[unitId];
	}
	unitPossibleCommands[unitId] = cmdDescVec;
	
	return cmdDescVec;
}

const CCommandQueue* CAIAICallback::GetCurrentUnitCommands(int unitId) {
	int numCmds = sAICallback->Unit_getNumCurrentCommands(teamId, unitId);
int ids[numCmds];
unsigned char options[numCmds];
unsigned int tags[numCmds];
int timeOuts[numCmds];
int numParams[numCmds];
float* params[numCmds];
sAICallback->Unit_getNumCurrentCommands(teamId, unitId);
sAICallback->Unit_CurrentCommands_getIds(teamId, unitId, ids);
sAICallback->Unit_CurrentCommands_getOptions(teamId, unitId, options);
sAICallback->Unit_CurrentCommands_getTag(teamId, unitId, tags);
sAICallback->Unit_CurrentCommands_getTimeOut(teamId, unitId, timeOuts);
sAICallback->Unit_CurrentCommands_getNumParams(teamId, unitId, numParams);
for (int c=0; c < numCmds; c++) {
	params[c] = new float[numParams[c]];
}	
sAICallback->Unit_CurrentCommands_getParams(teamId, unitId, params);
	CCommandQueue* cc = new CCommandQueue();
	for (int c=0; c < numCmds; c++) {
		Command command;
		command.id = ids[c];
		command.options = options[c];
		command.tag = tags[c];
		command.timeOut = timeOuts[c];
		for (int p=0; p < numParams[c]; p++) {
			command.params.push_back(params[c][p]);
		}
		cc->push_back(command);
	}
	
	// to prevent memory wholes
	if (unitCurrentCommandQueues[unitId] != NULL) {
		delete unitCurrentCommandQueues[unitId];
	}
	unitCurrentCommandQueues[unitId] = cc;
	
	return cc;
}

int CAIAICallback::GetUnitAiHint(int unitId) {
	return sAICallback->Unit_getAiHint(teamId, unitId);
}

int CAIAICallback::GetUnitTeam(int unitId) {
	return sAICallback->Unit_getTeam(teamId, unitId);
}

int CAIAICallback::GetUnitAllyTeam(int unitId) {
	return sAICallback->Unit_getAllyTeam(teamId, unitId);
}

float CAIAICallback::GetUnitHealth(int unitId) {
	return sAICallback->Unit_getHealth(teamId, unitId);
}

float CAIAICallback::GetUnitMaxHealth(int unitId) {
	return sAICallback->Unit_getMaxHealth(teamId, unitId);
}

float CAIAICallback::GetUnitSpeed(int unitId) {
	return sAICallback->Unit_getSpeed(teamId, unitId);
}

float CAIAICallback::GetUnitPower(int unitId) {
	return sAICallback->Unit_getPower(teamId, unitId);
}

float CAIAICallback::GetUnitExperience(int unitId) {
	return sAICallback->Unit_getExperience(teamId, unitId);
}

float CAIAICallback::GetUnitMaxRange(int unitId) {
	return sAICallback->Unit_getMaxRange(teamId, unitId);
}

bool CAIAICallback::IsUnitActivated(int unitId) {
	return sAICallback->Unit_isActivated(teamId, unitId);
}

bool CAIAICallback::UnitBeingBuilt(int unitId) {
	return sAICallback->Unit_isBeingBuilt(teamId, unitId);
}

const UnitDef* CAIAICallback::GetUnitDef(int unitId) {
	int unitDefId = sAICallback->Unit_getDefId(teamId, unitId);
	return this->GetUnitDefById(unitDefId);
}

float3 CAIAICallback::GetUnitPos(int unitId) {
	return float3(sAICallback->Unit_getPos(teamId, unitId));
}

int CAIAICallback::GetBuildingFacing(int unitId) {
	return sAICallback->Unit_getBuildingFacing(teamId, unitId);
}

bool CAIAICallback::IsUnitCloaked(int unitId) {
	return sAICallback->Unit_isCloaked(teamId, unitId);
}

bool CAIAICallback::IsUnitParalyzed(int unitId) {
	return sAICallback->Unit_isParalyzed(teamId, unitId);
}

bool CAIAICallback::IsUnitNeutral(int unitId) {
	return sAICallback->Unit_isNeutral(teamId, unitId);
}

bool CAIAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {
	resourceInfo->energyMake = sAICallback->Unit_ResourceInfo_Energy_getMake(teamId, unitId);
	if (resourceInfo->energyMake < 0) return false;
	resourceInfo->energyUse = sAICallback->Unit_ResourceInfo_Energy_getUse(teamId, unitId);
	resourceInfo->metalMake = sAICallback->Unit_ResourceInfo_Metal_getMake(teamId, unitId);
	resourceInfo->metalUse = sAICallback->Unit_ResourceInfo_Metal_getUse(teamId, unitId);
	return true;
}

const UnitDef* CAIAICallback::GetUnitDef(const char* unitName) {
	int unitDefId = sAICallback->UnitDef_STATIC_getIdByName(teamId, unitName);
	return this->GetUnitDefById(unitDefId);
}


const UnitDef* CAIAICallback::GetUnitDefById(int unitDefId) {
	//logT("entering: GetUnitDefById sAICallback");
	
	if (unitDefId < 0) {
		return NULL;
	}
	
	bool doRecreate = unitDefFrames[unitDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
	UnitDef* unitDef = new UnitDef();
unitDef->valid = sAICallback->UnitDef_isValid(teamId, unitDefId);
unitDef->name = sAICallback->UnitDef_getName(teamId, unitDefId);
unitDef->humanName = sAICallback->UnitDef_getHumanName(teamId, unitDefId);
unitDef->filename = sAICallback->UnitDef_getFilename(teamId, unitDefId);
unitDef->id = sAICallback->UnitDef_getId(teamId, unitDefId);
unitDef->aihint = sAICallback->UnitDef_getAiHint(teamId, unitDefId);
unitDef->cobID = sAICallback->UnitDef_getCobID(teamId, unitDefId);
unitDef->techLevel = sAICallback->UnitDef_getTechLevel(teamId, unitDefId);
unitDef->gaia = sAICallback->UnitDef_getGaia(teamId, unitDefId);
unitDef->metalUpkeep = sAICallback->UnitDef_getMetalUpkeep(teamId, unitDefId);
unitDef->energyUpkeep = sAICallback->UnitDef_getEnergyUpkeep(teamId, unitDefId);
unitDef->metalMake = sAICallback->UnitDef_getMetalMake(teamId, unitDefId);
unitDef->makesMetal = sAICallback->UnitDef_getMakesMetal(teamId, unitDefId);
unitDef->energyMake = sAICallback->UnitDef_getEnergyMake(teamId, unitDefId);
unitDef->metalCost = sAICallback->UnitDef_getMetalCost(teamId, unitDefId);
unitDef->energyCost = sAICallback->UnitDef_getEnergyCost(teamId, unitDefId);
unitDef->buildTime = sAICallback->UnitDef_getBuildTime(teamId, unitDefId);
unitDef->extractsMetal = sAICallback->UnitDef_getExtractsMetal(teamId, unitDefId);
unitDef->extractRange = sAICallback->UnitDef_getExtractRange(teamId, unitDefId);
unitDef->windGenerator = sAICallback->UnitDef_getWindGenerator(teamId, unitDefId);
unitDef->tidalGenerator = sAICallback->UnitDef_getTidalGenerator(teamId, unitDefId);
unitDef->metalStorage = sAICallback->UnitDef_getMetalStorage(teamId, unitDefId);
unitDef->energyStorage = sAICallback->UnitDef_getEnergyStorage(teamId, unitDefId);
unitDef->autoHeal = sAICallback->UnitDef_getAutoHeal(teamId, unitDefId);
unitDef->idleAutoHeal = sAICallback->UnitDef_getIdleAutoHeal(teamId, unitDefId);
unitDef->idleTime = sAICallback->UnitDef_getIdleTime(teamId, unitDefId);
unitDef->power = sAICallback->UnitDef_getPower(teamId, unitDefId);
unitDef->health = sAICallback->UnitDef_getHealth(teamId, unitDefId);
unitDef->category = sAICallback->UnitDef_getCategory(teamId, unitDefId);
unitDef->speed = sAICallback->UnitDef_getSpeed(teamId, unitDefId);
unitDef->turnRate = sAICallback->UnitDef_getTurnRate(teamId, unitDefId);
unitDef->turnInPlace = sAICallback->UnitDef_isTurnInPlace(teamId, unitDefId);
unitDef->moveType = sAICallback->UnitDef_getMoveType(teamId, unitDefId);
unitDef->upright = sAICallback->UnitDef_isUpright(teamId, unitDefId);
unitDef->collide = sAICallback->UnitDef_isCollide(teamId, unitDefId);
unitDef->controlRadius = sAICallback->UnitDef_getControlRadius(teamId, unitDefId);
unitDef->losRadius = sAICallback->UnitDef_getLosRadius(teamId, unitDefId);
unitDef->airLosRadius = sAICallback->UnitDef_getAirLosRadius(teamId, unitDefId);
unitDef->losHeight = sAICallback->UnitDef_getLosHeight(teamId, unitDefId);
unitDef->radarRadius = sAICallback->UnitDef_getRadarRadius(teamId, unitDefId);
unitDef->sonarRadius = sAICallback->UnitDef_getSonarRadius(teamId, unitDefId);
unitDef->jammerRadius = sAICallback->UnitDef_getJammerRadius(teamId, unitDefId);
unitDef->sonarJamRadius = sAICallback->UnitDef_getSonarJamRadius(teamId, unitDefId);
unitDef->seismicRadius = sAICallback->UnitDef_getSeismicRadius(teamId, unitDefId);
unitDef->seismicSignature = sAICallback->UnitDef_getSeismicSignature(teamId, unitDefId);
unitDef->stealth = sAICallback->UnitDef_isStealth(teamId, unitDefId);
unitDef->sonarStealth = sAICallback->UnitDef_isSonarStealth(teamId, unitDefId);
unitDef->buildRange3D = sAICallback->UnitDef_isBuildRange3D(teamId, unitDefId);
unitDef->buildDistance = sAICallback->UnitDef_getBuildDistance(teamId, unitDefId);
unitDef->buildSpeed = sAICallback->UnitDef_getBuildSpeed(teamId, unitDefId);
unitDef->reclaimSpeed = sAICallback->UnitDef_getReclaimSpeed(teamId, unitDefId);
unitDef->repairSpeed = sAICallback->UnitDef_getRepairSpeed(teamId, unitDefId);
unitDef->maxRepairSpeed = sAICallback->UnitDef_getMaxRepairSpeed(teamId, unitDefId);
unitDef->resurrectSpeed = sAICallback->UnitDef_getResurrectSpeed(teamId, unitDefId);
unitDef->captureSpeed = sAICallback->UnitDef_getCaptureSpeed(teamId, unitDefId);
unitDef->terraformSpeed = sAICallback->UnitDef_getTerraformSpeed(teamId, unitDefId);
unitDef->mass = sAICallback->UnitDef_getMass(teamId, unitDefId);
unitDef->pushResistant = sAICallback->UnitDef_isPushResistant(teamId, unitDefId);
unitDef->strafeToAttack = sAICallback->UnitDef_isStrafeToAttack(teamId, unitDefId);
unitDef->minCollisionSpeed = sAICallback->UnitDef_getMinCollisionSpeed(teamId, unitDefId);
unitDef->slideTolerance = sAICallback->UnitDef_getSlideTolerance(teamId, unitDefId);
unitDef->maxSlope = sAICallback->UnitDef_getMaxSlope(teamId, unitDefId);
unitDef->maxHeightDif = sAICallback->UnitDef_getMaxHeightDif(teamId, unitDefId);
unitDef->minWaterDepth = sAICallback->UnitDef_getMinWaterDepth(teamId, unitDefId);
unitDef->waterline = sAICallback->UnitDef_getWaterline(teamId, unitDefId);
unitDef->maxWaterDepth = sAICallback->UnitDef_getMaxWaterDepth(teamId, unitDefId);
unitDef->armoredMultiple = sAICallback->UnitDef_getArmoredMultiple(teamId, unitDefId);
unitDef->armorType = sAICallback->UnitDef_getArmorType(teamId, unitDefId);
unitDef->flankingBonusMode = sAICallback->UnitDef_getFlankingBonusMode(teamId, unitDefId);
unitDef->flankingBonusDir = float3(sAICallback->UnitDef_getFlankingBonusDir(teamId, unitDefId));
unitDef->flankingBonusMax = sAICallback->UnitDef_getFlankingBonusMax(teamId, unitDefId);
unitDef->flankingBonusMin = sAICallback->UnitDef_getFlankingBonusMin(teamId, unitDefId);
unitDef->flankingBonusMobilityAdd = sAICallback->UnitDef_getFlankingBonusMobilityAdd(teamId, unitDefId);
unitDef->collisionVolumeType = sAICallback->UnitDef_getCollisionVolumeType(teamId, unitDefId);
unitDef->collisionVolumeScales = float3(sAICallback->UnitDef_getCollisionVolumeScales(teamId, unitDefId));
unitDef->collisionVolumeOffsets = float3(sAICallback->UnitDef_getCollisionVolumeOffsets(teamId, unitDefId));
unitDef->collisionVolumeTest = sAICallback->UnitDef_getCollisionVolumeTest(teamId, unitDefId);
unitDef->maxWeaponRange = sAICallback->UnitDef_getMaxWeaponRange(teamId, unitDefId);
unitDef->type = sAICallback->UnitDef_getType(teamId, unitDefId);
unitDef->tooltip = sAICallback->UnitDef_getTooltip(teamId, unitDefId);
unitDef->wreckName = sAICallback->UnitDef_getWreckName(teamId, unitDefId);
unitDef->deathExplosion = sAICallback->UnitDef_getDeathExplosion(teamId, unitDefId);
unitDef->selfDExplosion = sAICallback->UnitDef_getSelfDExplosion(teamId, unitDefId);
unitDef->TEDClassString = sAICallback->UnitDef_getTedClassString(teamId, unitDefId);
unitDef->categoryString = sAICallback->UnitDef_getCategoryString(teamId, unitDefId);
unitDef->canSelfD = sAICallback->UnitDef_isCanSelfD(teamId, unitDefId);
unitDef->selfDCountdown = sAICallback->UnitDef_getSelfDCountdown(teamId, unitDefId);
unitDef->canSubmerge = sAICallback->UnitDef_isCanSubmerge(teamId, unitDefId);
unitDef->canfly = sAICallback->UnitDef_isCanFly(teamId, unitDefId);
unitDef->canmove = sAICallback->UnitDef_isCanMove(teamId, unitDefId);
unitDef->canhover = sAICallback->UnitDef_isCanHover(teamId, unitDefId);
unitDef->floater = sAICallback->UnitDef_isFloater(teamId, unitDefId);
unitDef->builder = sAICallback->UnitDef_isBuilder(teamId, unitDefId);
unitDef->activateWhenBuilt = sAICallback->UnitDef_isActivateWhenBuilt(teamId, unitDefId);
unitDef->onoffable = sAICallback->UnitDef_isOnOffable(teamId, unitDefId);
unitDef->fullHealthFactory = sAICallback->UnitDef_isFullHealthFactory(teamId, unitDefId);
unitDef->factoryHeadingTakeoff = sAICallback->UnitDef_isFactoryHeadingTakeoff(teamId, unitDefId);
unitDef->reclaimable = sAICallback->UnitDef_isReclaimable(teamId, unitDefId);
unitDef->capturable = sAICallback->UnitDef_isCapturable(teamId, unitDefId);
unitDef->canRestore = sAICallback->UnitDef_isCanRestore(teamId, unitDefId);
unitDef->canRepair = sAICallback->UnitDef_isCanRepair(teamId, unitDefId);
unitDef->canSelfRepair = sAICallback->UnitDef_isCanSelfRepair(teamId, unitDefId);
unitDef->canReclaim = sAICallback->UnitDef_isCanReclaim(teamId, unitDefId);
unitDef->canAttack = sAICallback->UnitDef_isCanAttack(teamId, unitDefId);
unitDef->canPatrol = sAICallback->UnitDef_isCanPatrol(teamId, unitDefId);
unitDef->canFight = sAICallback->UnitDef_isCanFight(teamId, unitDefId);
unitDef->canGuard = sAICallback->UnitDef_isCanGuard(teamId, unitDefId);
unitDef->canBuild = sAICallback->UnitDef_isCanBuild(teamId, unitDefId);
unitDef->canAssist = sAICallback->UnitDef_isCanAssist(teamId, unitDefId);
unitDef->canBeAssisted = sAICallback->UnitDef_isCanBeAssisted(teamId, unitDefId);
unitDef->canRepeat = sAICallback->UnitDef_isCanRepeat(teamId, unitDefId);
unitDef->canFireControl = sAICallback->UnitDef_isCanFireControl(teamId, unitDefId);
unitDef->fireState = sAICallback->UnitDef_getFireState(teamId, unitDefId);
unitDef->moveState = sAICallback->UnitDef_getMoveState(teamId, unitDefId);
unitDef->wingDrag = sAICallback->UnitDef_getWingDrag(teamId, unitDefId);
unitDef->wingAngle = sAICallback->UnitDef_getWingAngle(teamId, unitDefId);
unitDef->drag = sAICallback->UnitDef_getDrag(teamId, unitDefId);
unitDef->frontToSpeed = sAICallback->UnitDef_getFrontToSpeed(teamId, unitDefId);
unitDef->speedToFront = sAICallback->UnitDef_getSpeedToFront(teamId, unitDefId);
unitDef->myGravity = sAICallback->UnitDef_getMyGravity(teamId, unitDefId);
unitDef->maxBank = sAICallback->UnitDef_getMaxBank(teamId, unitDefId);
unitDef->maxPitch = sAICallback->UnitDef_getMaxPitch(teamId, unitDefId);
unitDef->turnRadius = sAICallback->UnitDef_getTurnRadius(teamId, unitDefId);
unitDef->wantedHeight = sAICallback->UnitDef_getWantedHeight(teamId, unitDefId);
unitDef->verticalSpeed = sAICallback->UnitDef_getVerticalSpeed(teamId, unitDefId);
unitDef->canCrash = sAICallback->UnitDef_isCanCrash(teamId, unitDefId);
unitDef->hoverAttack = sAICallback->UnitDef_isHoverAttack(teamId, unitDefId);
unitDef->airStrafe = sAICallback->UnitDef_isAirStrafe(teamId, unitDefId);
unitDef->dlHoverFactor = sAICallback->UnitDef_getDlHoverFactor(teamId, unitDefId);
unitDef->maxAcc = sAICallback->UnitDef_getMaxAcceleration(teamId, unitDefId);
unitDef->maxDec = sAICallback->UnitDef_getMaxDeceleration(teamId, unitDefId);
unitDef->maxAileron = sAICallback->UnitDef_getMaxAileron(teamId, unitDefId);
unitDef->maxElevator = sAICallback->UnitDef_getMaxElevator(teamId, unitDefId);
unitDef->maxRudder = sAICallback->UnitDef_getMaxRudder(teamId, unitDefId);
//unitDef->yardmaps = sAICallback->UnitDef_getYardMaps(teamId, unitDefId);
unitDef->xsize = sAICallback->UnitDef_getXSize(teamId, unitDefId);
unitDef->ysize = sAICallback->UnitDef_getYSize(teamId, unitDefId);
unitDef->buildangle = sAICallback->UnitDef_getBuildAngle(teamId, unitDefId);
unitDef->loadingRadius = sAICallback->UnitDef_getLoadingRadius(teamId, unitDefId);
unitDef->unloadSpread = sAICallback->UnitDef_getUnloadSpread(teamId, unitDefId);
unitDef->transportCapacity = sAICallback->UnitDef_getTransportCapacity(teamId, unitDefId);
unitDef->transportSize = sAICallback->UnitDef_getTransportSize(teamId, unitDefId);
unitDef->minTransportSize = sAICallback->UnitDef_getMinTransportSize(teamId, unitDefId);
unitDef->isAirBase = sAICallback->UnitDef_isAirBase(teamId, unitDefId);
unitDef->transportMass = sAICallback->UnitDef_getTransportMass(teamId, unitDefId);
unitDef->minTransportMass = sAICallback->UnitDef_getMinTransportMass(teamId, unitDefId);
unitDef->holdSteady = sAICallback->UnitDef_isHoldSteady(teamId, unitDefId);
unitDef->releaseHeld = sAICallback->UnitDef_isReleaseHeld(teamId, unitDefId);
unitDef->cantBeTransported = sAICallback->UnitDef_isCantBeTransported(teamId, unitDefId);
unitDef->transportByEnemy = sAICallback->UnitDef_isTransportByEnemy(teamId, unitDefId);
unitDef->transportUnloadMethod = sAICallback->UnitDef_getTransportUnloadMethod(teamId, unitDefId);
unitDef->fallSpeed = sAICallback->UnitDef_getFallSpeed(teamId, unitDefId);
unitDef->unitFallSpeed = sAICallback->UnitDef_getUnitFallSpeed(teamId, unitDefId);
unitDef->canCloak = sAICallback->UnitDef_isCanCloak(teamId, unitDefId);
unitDef->startCloaked = sAICallback->UnitDef_isStartCloaked(teamId, unitDefId);
unitDef->cloakCost = sAICallback->UnitDef_getCloakCost(teamId, unitDefId);
unitDef->cloakCostMoving = sAICallback->UnitDef_getCloakCostMoving(teamId, unitDefId);
unitDef->decloakDistance = sAICallback->UnitDef_getDecloakDistance(teamId, unitDefId);
unitDef->decloakSpherical = sAICallback->UnitDef_isDecloakSpherical(teamId, unitDefId);
unitDef->decloakOnFire = sAICallback->UnitDef_isDecloakOnFire(teamId, unitDefId);
unitDef->canKamikaze = sAICallback->UnitDef_isCanKamikaze(teamId, unitDefId);
unitDef->kamikazeDist = sAICallback->UnitDef_getKamikazeDist(teamId, unitDefId);
unitDef->targfac = sAICallback->UnitDef_isTargetingFacility(teamId, unitDefId);
unitDef->canDGun = sAICallback->UnitDef_isCanDGun(teamId, unitDefId);
unitDef->needGeo = sAICallback->UnitDef_isNeedGeo(teamId, unitDefId);
unitDef->isFeature = sAICallback->UnitDef_isFeature(teamId, unitDefId);
unitDef->hideDamage = sAICallback->UnitDef_isHideDamage(teamId, unitDefId);
unitDef->isCommander = sAICallback->UnitDef_isCommander(teamId, unitDefId);
unitDef->showPlayerName = sAICallback->UnitDef_isShowPlayerName(teamId, unitDefId);
unitDef->canResurrect = sAICallback->UnitDef_isCanResurrect(teamId, unitDefId);
unitDef->canCapture = sAICallback->UnitDef_isCanCapture(teamId, unitDefId);
unitDef->highTrajectoryType = sAICallback->UnitDef_getHighTrajectoryType(teamId, unitDefId);
unitDef->noChaseCategory = sAICallback->UnitDef_getNoChaseCategory(teamId, unitDefId);
unitDef->leaveTracks = sAICallback->UnitDef_isLeaveTracks(teamId, unitDefId);
unitDef->trackWidth = sAICallback->UnitDef_getTrackWidth(teamId, unitDefId);
unitDef->trackOffset = sAICallback->UnitDef_getTrackOffset(teamId, unitDefId);
unitDef->trackStrength = sAICallback->UnitDef_getTrackStrength(teamId, unitDefId);
unitDef->trackStretch = sAICallback->UnitDef_getTrackStretch(teamId, unitDefId);
unitDef->trackType = sAICallback->UnitDef_getTrackType(teamId, unitDefId);
unitDef->canDropFlare = sAICallback->UnitDef_isCanDropFlare(teamId, unitDefId);
unitDef->flareReloadTime = sAICallback->UnitDef_getFlareReloadTime(teamId, unitDefId);
unitDef->flareEfficiency = sAICallback->UnitDef_getFlareEfficiency(teamId, unitDefId);
unitDef->flareDelay = sAICallback->UnitDef_getFlareDelay(teamId, unitDefId);
unitDef->flareDropVector = float3(sAICallback->UnitDef_getFlareDropVector(teamId, unitDefId));
unitDef->flareTime = sAICallback->UnitDef_getFlareTime(teamId, unitDefId);
unitDef->flareSalvoSize = sAICallback->UnitDef_getFlareSalvoSize(teamId, unitDefId);
unitDef->flareSalvoDelay = sAICallback->UnitDef_getFlareSalvoDelay(teamId, unitDefId);
unitDef->smoothAnim = sAICallback->UnitDef_isSmoothAnim(teamId, unitDefId);
unitDef->isMetalMaker = sAICallback->UnitDef_isMetalMaker(teamId, unitDefId);
unitDef->canLoopbackAttack = sAICallback->UnitDef_isCanLoopbackAttack(teamId, unitDefId);
unitDef->levelGround = sAICallback->UnitDef_isLevelGround(teamId, unitDefId);
unitDef->useBuildingGroundDecal = sAICallback->UnitDef_isUseBuildingGroundDecal(teamId, unitDefId);
unitDef->buildingDecalType = sAICallback->UnitDef_getBuildingDecalType(teamId, unitDefId);
unitDef->buildingDecalSizeX = sAICallback->UnitDef_getBuildingDecalSizeX(teamId, unitDefId);
unitDef->buildingDecalSizeY = sAICallback->UnitDef_getBuildingDecalSizeY(teamId, unitDefId);
unitDef->buildingDecalDecaySpeed = sAICallback->UnitDef_getBuildingDecalDecaySpeed(teamId, unitDefId);
unitDef->isFirePlatform = sAICallback->UnitDef_isFirePlatform(teamId, unitDefId);
unitDef->maxFuel = sAICallback->UnitDef_getMaxFuel(teamId, unitDefId);
unitDef->refuelTime = sAICallback->UnitDef_getRefuelTime(teamId, unitDefId);
unitDef->minAirBasePower = sAICallback->UnitDef_getMinAirBasePower(teamId, unitDefId);
unitDef->maxThisUnit = sAICallback->UnitDef_getMaxThisUnit(teamId, unitDefId);
//unitDef->decoyDef = sAICallback->UnitDef_getDecoyDefId(teamId, unitDefId);
unitDef->shieldWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getShieldWeaponDefId(teamId, unitDefId));
unitDef->stockpileWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getStockpileWeaponDefId(teamId, unitDefId));
{
	int numBo = sAICallback->UnitDef_getNumBuildOptions(teamId, unitDefId);
	int* bo = new int[numBo];
	sAICallback->UnitDef_getBuildOptions(teamId, unitDefId, bo);
	for (int b=0; b < numBo; b++) {
		unitDef->buildOptions[b] = sAICallback->UnitDef_getName(teamId, bo[b]);
	}
	delete [] bo;
}
{
	int size = sAICallback->UnitDef_getNumCustomParams(teamId, unitDefId);
	const char* cMap[size][2];
	sAICallback->UnitDef_getCustomParams(teamId, unitDefId, cMap);
	int i;
	for (i=0; i < size; ++i) {
		unitDef->customParams[cMap[i][0]] = cMap[i][1];
	}
}
if (sAICallback->UnitDef_hasMoveData(teamId, unitDefId)) {
	unitDef->movedata = new MoveData(NULL, -1);
		unitDef->movedata->moveType = (enum MoveData::MoveType)sAICallback->UnitDef_MoveData_getMoveType(teamId, unitDefId);
		unitDef->movedata->moveFamily = (enum MoveData::MoveFamily) sAICallback->UnitDef_MoveData_getMoveFamily(teamId, unitDefId);
        unitDef->movedata->size = sAICallback->UnitDef_MoveData_getSize(teamId, unitDefId);
		unitDef->movedata->depth = sAICallback->UnitDef_MoveData_getDepth(teamId, unitDefId);
		unitDef->movedata->maxSlope = sAICallback->UnitDef_MoveData_getMaxSlope(teamId, unitDefId);
		unitDef->movedata->slopeMod = sAICallback->UnitDef_MoveData_getSlopeMod(teamId, unitDefId);
		unitDef->movedata->depthMod = sAICallback->UnitDef_MoveData_getDepthMod(teamId, unitDefId);
		unitDef->movedata->pathType = sAICallback->UnitDef_MoveData_getPathType(teamId, unitDefId);
		unitDef->movedata->crushStrength = sAICallback->UnitDef_MoveData_getCrushStrength(teamId, unitDefId);
		unitDef->movedata->maxSpeed = sAICallback->UnitDef_MoveData_getMaxSpeed(teamId, unitDefId);
		unitDef->movedata->maxTurnRate = sAICallback->UnitDef_MoveData_getMaxTurnRate(teamId, unitDefId);
		unitDef->movedata->maxAcceleration = sAICallback->UnitDef_MoveData_getMaxAcceleration(teamId, unitDefId);
		unitDef->movedata->maxBreaking = sAICallback->UnitDef_MoveData_getMaxBreaking(teamId, unitDefId);
		unitDef->movedata->subMarine = sAICallback->UnitDef_MoveData_isSubMarine(teamId, unitDefId);
	} else {
		unitDef->movedata = NULL;
	}
int numWeapons = sAICallback->UnitDef_getNumUnitDefWeapons(teamId, unitDefId);
for (int w=0; w < numWeapons; ++w) {
	unitDef->weapons.push_back(UnitDef::UnitDefWeapon());
	unitDef->weapons[w].name = sAICallback->UnitDef_UnitDefWeapon_getName(teamId, unitDefId, w);
	int weaponDefId = sAICallback->UnitDef_UnitDefWeapon_getWeaponDefId(teamId, unitDefId, w);
	unitDef->weapons[w].def = this->GetWeaponDefById(weaponDefId);
	unitDef->weapons[w].slavedTo = sAICallback->UnitDef_UnitDefWeapon_getSlavedTo(teamId, unitDefId, w);
	unitDef->weapons[w].mainDir = float3(sAICallback->UnitDef_UnitDefWeapon_getMainDir(teamId, unitDefId, w));
	unitDef->weapons[w].maxAngleDif = sAICallback->UnitDef_UnitDefWeapon_getMaxAngleDif(teamId, unitDefId, w);
	unitDef->weapons[w].fuelUsage = sAICallback->UnitDef_UnitDefWeapon_getFuelUsage(teamId, unitDefId, w);
	unitDef->weapons[w].badTargetCat = sAICallback->UnitDef_UnitDefWeapon_getBadTargetCat(teamId, unitDefId, w);
	unitDef->weapons[w].onlyTargetCat = sAICallback->UnitDef_UnitDefWeapon_getOnlyTargetCat(teamId, unitDefId, w);
}
	if (unitDefs[unitDefId] != NULL) {
		delete unitDefs[unitDefId];
	}
		unitDefs[unitDefId] = unitDef;
		unitDefFrames[unitDefId] = currentFrame;
	}

	return unitDefs[unitDefId];
}

int CAIAICallback::GetEnemyUnits(int* unitIds) {
	return sAICallback->Unit_STATIC_getEnemies(teamId, unitIds);
}

int CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius) {
	return sAICallback->Unit_STATIC_getEnemiesIn(teamId, unitIds, pos.toSAIFloat3(), radius);
}

int CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds) {
	return sAICallback->Unit_STATIC_getEnemiesInRadarAndLos(teamId, unitIds);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds) {
	return sAICallback->Unit_STATIC_getFriendlies(teamId, unitIds);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius) {
	return sAICallback->Unit_STATIC_getFriendliesIn(teamId, unitIds, pos.toSAIFloat3(), radius);
}

int CAIAICallback::GetNeutralUnits(int* unitIds) {
	return sAICallback->Unit_STATIC_getNeutrals(teamId, unitIds);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius) {
	return sAICallback->Unit_STATIC_getNeutralsIn(teamId, unitIds, pos.toSAIFloat3(), radius);
}

int CAIAICallback::GetMapWidth() {
	return sAICallback->Map_getWidth(teamId);
}

int CAIAICallback::GetMapHeight() {
	return sAICallback->Map_getHeight(teamId);
}

const float* CAIAICallback::GetHeightMap() {
	return sAICallback->Map_getHeightMap(teamId);
}

float CAIAICallback::GetMinHeight() {
	return sAICallback->Map_getMinHeight(teamId);
}

float CAIAICallback::GetMaxHeight() {
	return sAICallback->Map_getMaxHeight(teamId);
}

const float* CAIAICallback::GetSlopeMap() {
	return sAICallback->Map_getSlopeMap(teamId);
}

const unsigned short* CAIAICallback::GetLosMap() {
	return sAICallback->Map_getLosMap(teamId);
}

const unsigned short* CAIAICallback::GetRadarMap() {
	return sAICallback->Map_getRadarMap(teamId);
}

const unsigned short* CAIAICallback::GetJammerMap() {
	return sAICallback->Map_getJammerMap(teamId);
}

const unsigned char* CAIAICallback::GetMetalMap() {
	return sAICallback->Map_getMetalMap(teamId);
}

const char* CAIAICallback::GetMapName() {
	return sAICallback->Map_getName(teamId);
}

const char* CAIAICallback::GetModName() {
	return sAICallback->Mod_getName(teamId);
}

float CAIAICallback::GetElevation(float x, float z) {
	return sAICallback->Map_getElevationAt(teamId, x, z);
}

float CAIAICallback::GetMaxMetal() {
	return sAICallback->Map_getMaxMetal(teamId);
}

float CAIAICallback::GetExtractorRadius() {
	return sAICallback->Map_getExtractorRadius(teamId);
}

float CAIAICallback::GetMinWind() {
	return sAICallback->Map_getMinWind(teamId);
}

float CAIAICallback::GetMaxWind() {
	return sAICallback->Map_getMaxWind(teamId);
}

float CAIAICallback::GetTidalStrength() {
	return sAICallback->Map_getTidalStrength(teamId);
}

float CAIAICallback::GetGravity() {
	return sAICallback->Map_getGravity(teamId);
}

bool CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {
	return sAICallback->Map_canBuildAt(teamId, unitDef->id, pos.toSAIFloat3(), facing);
}

float3 CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing) {
	return float3(sAICallback->Map_findClosestBuildSite(teamId, unitDef->id, pos.toSAIFloat3(), searchRadius, minDist, facing));
}

/*
bool CAIAICallback::GetProperty(int id, int property, void* dst) {
//	return sAICallback->getProperty(teamId, id, property, dst);
	return false;
}
*/
bool CAIAICallback::GetProperty(int unitId, int propertyId, void *data)
{
    switch (propertyId) {
        case AIVAL_UNITDEF: {
            return false;
        }
        case AIVAL_CURRENT_FUEL: {
            (*(float*)data) = sAICallback->Unit_getCurrentFuel(teamId, unitId);
            return (*(float*)data) != -1.0f;
        }
        case AIVAL_STOCKPILED: {
            (*(int*)data) = sAICallback->Unit_getStockpile(teamId, unitId);
            return (*(int*)data) != -1;
        }
        case AIVAL_STOCKPILE_QUED: {
            (*(int*)data) = sAICallback->Unit_getStockpileQueued(teamId, unitId);
            return (*(int*)data) != -1;
        }
        case AIVAL_UNIT_MAXSPEED: {
            (*(float*) data) = sAICallback->Unit_getMaxSpeed(teamId, unitId);
            return (*(float*)data) != -1.0f;
        }
        default:
            return false;
	}
	return false;
}

/*
bool CAIAICallback::GetValue(int valueId, void* dst) {
//	return sAICallback->getValue(teamId, valueId, dst);
	return false;
}
*/
bool CAIAICallback::GetValue(int valueId, void *data)
{
	switch (valueId) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = sAICallback->WeaponDef_STATIC_getNumDamageTypes(teamId);
			return true;
		}case AIVAL_EXCEPTION_HANDLING:{
			*(bool*)data = sAICallback->Game_isExceptionHandlingEnabled(teamId);
			return true;
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = sAICallback->Map_getChecksum(teamId);
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = sAICallback->Game_isDebugModeEnabled(teamId);
			return true;
		}case AIVAL_GAME_MODE:{
			*(int*)data = sAICallback->Game_getMode(teamId);
			return true;
		}case AIVAL_GAME_PAUSED:{
			*(bool*)data = sAICallback->Game_isPaused(teamId);
			return true;
		}case AIVAL_GAME_SPEED_FACTOR:{
			*(float*)data = sAICallback->Game_getSpeedFactor(teamId);
			return true;
		}case AIVAL_GUI_VIEW_RANGE:{
			*(float*)data = sAICallback->Gui_getViewRange(teamId);
			return true;
		}case AIVAL_GUI_SCREENX:{
			*(float*)data = sAICallback->Gui_getScreenX(teamId);
			return true;
		}case AIVAL_GUI_SCREENY:{
			*(float*)data = sAICallback->Gui_getScreenY(teamId);
			return true;
		}case AIVAL_GUI_CAMERA_DIR:{
			*(float3*)data = sAICallback->Gui_Camera_getDirection(teamId);
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			*(float3*)data = sAICallback->Gui_Camera_getPosition(teamId);
			return true;
		}case AIVAL_LOCATE_FILE_R:{
            sAICallback->File_locateForReading(teamId, (char*) data);
			return true;
		}case AIVAL_LOCATE_FILE_W:{
            sAICallback->File_locateForWriting(teamId, (char*) data);
			return true;
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = sAICallback->Unit_STATIC_getLimit(teamId);
			return true;
		}
		case AIVAL_SCRIPT: {
			*(const char**) data = sAICallback->Game_getSetupScript(teamId);
			return true;
		}
		default:
			return false;
	}
}

int CAIAICallback::GetFileSize(const char* name) {
	return sAICallback->File_getSize(teamId, name);
}

int CAIAICallback::GetSelectedUnits(int* unitIds) {
	return sAICallback->Unit_STATIC_getSelected(teamId, unitIds);
}

float3 CAIAICallback::GetMousePos() {
	return float3(sAICallback->Map_getMousePos(teamId));
}

int CAIAICallback::GetMapPoints(PointMarker* pm, int maxPoints) {
	SAIFloat3* positions = new SAIFloat3[maxPoints];
	unsigned char** colors = new unsigned char*[maxPoints];
	const char** labels = new const char*[maxPoints];
	int numPoints = sAICallback->Map_getPoints(teamId, positions, colors, labels, maxPoints);
	for (int i=0; i < numPoints; ++i) {
		pm[i].pos = float3(positions[i]);
		pm[i].color = colors[i];
		pm[i].label = labels[i];
	}
	delete [] positions;
	delete [] colors;
	delete [] labels;
	return numPoints;
}

int CAIAICallback::GetMapLines(LineMarker* lm, int maxLines) {
	SAIFloat3* firstPositions = new SAIFloat3[maxLines];
	SAIFloat3* secondPositions = new SAIFloat3[maxLines];
	unsigned char** colors = new unsigned char*[maxLines];
	int numLines = sAICallback->Map_getLines(teamId, firstPositions, secondPositions, colors, maxLines);
	for (int i=0; i < numLines; ++i) {
		lm[i].pos = float3(firstPositions[i]);
		lm[i].pos2 = float3(secondPositions[i]);
		lm[i].color = colors[i];
	}
	delete [] firstPositions;
	delete [] secondPositions;
	delete [] colors;
	return numLines;
}

float CAIAICallback::GetMetal() {
	return sAICallback->ResourceInfo_Metal_getCurrent(teamId);
}

float CAIAICallback::GetMetalIncome() {
	return sAICallback->ResourceInfo_Metal_getIncome(teamId);
}

float CAIAICallback::GetMetalUsage() {
	return sAICallback->ResourceInfo_Metal_getUsage(teamId);
}

float CAIAICallback::GetMetalStorage() {
	return sAICallback->ResourceInfo_Metal_getStorage(teamId);
}

float CAIAICallback::GetEnergy() {
	return sAICallback->ResourceInfo_Energy_getCurrent(teamId);
}

float CAIAICallback::GetEnergyIncome() {
	return sAICallback->ResourceInfo_Energy_getIncome(teamId);
}

float CAIAICallback::GetEnergyUsage() {
	return sAICallback->ResourceInfo_Energy_getUsage(teamId);
}

float CAIAICallback::GetEnergyStorage() {
	return sAICallback->ResourceInfo_Energy_getStorage(teamId);
}

int CAIAICallback::GetFeatures(int *featureIds, int max) {
	return sAICallback->Feature_STATIC_getIds(teamId, featureIds, max);
}

int CAIAICallback::GetFeatures(int *featureIds, int max, const float3& pos, float radius) {
	return sAICallback->Feature_STATIC_getIdsIn(teamId, featureIds, max, pos.toSAIFloat3(), radius);
}

const FeatureDef* CAIAICallback::GetFeatureDef(int featureId) {
	int featureDefId = sAICallback->Feature_getDefId(teamId, featureId);
	return this->GetFeatureDefById(featureDefId);
}

const FeatureDef* CAIAICallback::GetFeatureDefById(int featureDefId) {
	
	if (featureDefId < 0) {
		return NULL;
	}
	
	bool doRecreate = featureDefFrames[featureDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
	FeatureDef* featureDef = new FeatureDef();
featureDef->myName = sAICallback->FeatureDef_getName(teamId, featureDefId);
featureDef->description = sAICallback->FeatureDef_getDescription(teamId, featureDefId);
featureDef->filename = sAICallback->FeatureDef_getFilename(teamId, featureDefId);
featureDef->id = sAICallback->FeatureDef_getId(teamId, featureDefId);
featureDef->metal = sAICallback->FeatureDef_getMetal(teamId, featureDefId);
featureDef->energy = sAICallback->FeatureDef_getEnergy(teamId, featureDefId);
featureDef->maxHealth = sAICallback->FeatureDef_getMaxHealth(teamId, featureDefId);
featureDef->reclaimTime = sAICallback->FeatureDef_getReclaimTime(teamId, featureDefId);
featureDef->mass = sAICallback->FeatureDef_getMass(teamId, featureDefId);
featureDef->collisionVolumeType = sAICallback->FeatureDef_getCollisionVolumeType(teamId, featureDefId);	
featureDef->collisionVolumeScales = float3(sAICallback->FeatureDef_getCollisionVolumeScales(teamId, featureDefId));		
featureDef->collisionVolumeOffsets = float3(sAICallback->FeatureDef_getCollisionVolumeOffsets(teamId, featureDefId));		
featureDef->collisionVolumeTest = sAICallback->FeatureDef_getCollisionVolumeTest(teamId, featureDefId);			
featureDef->upright = sAICallback->FeatureDef_isUpright(teamId, featureDefId);
featureDef->drawType = sAICallback->FeatureDef_getDrawType(teamId, featureDefId);
featureDef->modelname = sAICallback->FeatureDef_getModelName(teamId, featureDefId);
featureDef->modelType = sAICallback->FeatureDef_getModelType(teamId, featureDefId);
featureDef->resurrectable = sAICallback->FeatureDef_getResurrectable(teamId, featureDefId);
featureDef->smokeTime = sAICallback->FeatureDef_getSmokeTime(teamId, featureDefId);
featureDef->destructable = sAICallback->FeatureDef_isDestructable(teamId, featureDefId);
featureDef->reclaimable = sAICallback->FeatureDef_isReclaimable(teamId, featureDefId);
featureDef->blocking = sAICallback->FeatureDef_isBlocking(teamId, featureDefId);
featureDef->burnable = sAICallback->FeatureDef_isBurnable(teamId, featureDefId);
featureDef->floating = sAICallback->FeatureDef_isFloating(teamId, featureDefId);
featureDef->noSelect = sAICallback->FeatureDef_isNoSelect(teamId, featureDefId);
featureDef->geoThermal = sAICallback->FeatureDef_isGeoThermal(teamId, featureDefId);
featureDef->deathFeature = sAICallback->FeatureDef_getDeathFeature(teamId, featureDefId);
featureDef->xsize = sAICallback->FeatureDef_getXsize(teamId, featureDefId);
featureDef->ysize = sAICallback->FeatureDef_getYsize(teamId, featureDefId);
{
	int size = sAICallback->FeatureDef_getNumCustomParams(teamId, featureDefId);
	featureDef->customParams = std::map<std::string,std::string>();
	const char* cMap[size][2];
	sAICallback->FeatureDef_getCustomParams(teamId, featureDefId, cMap);
	int i;
	for (i=0; i < size; ++i) {
		featureDef->customParams[cMap[i][0]] = cMap[i][1];
	}
}
	if (featureDefs[featureDefId] != NULL) {
		delete featureDefs[featureDefId];
	}
		featureDefs[featureDefId] = featureDef;
		featureDefFrames[featureDefId] = currentFrame;
	}

	return featureDefs[featureDefId];
}

float CAIAICallback::GetFeatureHealth(int featureId) {
	return sAICallback->Feature_getHealth(teamId, featureId);
}

float CAIAICallback::GetFeatureReclaimLeft(int featureId) {
	return sAICallback->Feature_getReclaimLeft(teamId, featureId);
}

float3 CAIAICallback::GetFeaturePos(int featureId) {
	return float3(sAICallback->Feature_getPos(teamId, featureId));
}

int CAIAICallback::GetNumUnitDefs() {
	return sAICallback->UnitDef_STATIC_getNumIds(teamId);
}

void CAIAICallback::GetUnitDefList(const UnitDef** list) {
	int numUnitDefs = sAICallback->UnitDef_STATIC_getNumIds(teamId);
	int* unitDefIds = new int[numUnitDefs];
	sAICallback->UnitDef_STATIC_getIds(teamId, unitDefIds);
	for (int i=0; i < numUnitDefs; ++i) {
		list[i] = this->GetUnitDefById(unitDefIds[i]);
	}
}

float CAIAICallback::GetUnitDefHeight(int def) {
	return sAICallback->UnitDef_getHeight(teamId, def);
}

float CAIAICallback::GetUnitDefRadius(int def) {
	return sAICallback->UnitDef_getRadius(teamId, def);
}

const WeaponDef* CAIAICallback::GetWeapon(const char* weaponName) {
	int weaponDefId = sAICallback->WeaponDef_STATIC_getIdByName(teamId, weaponName);
	return this->GetWeaponDefById(weaponDefId);
}

const WeaponDef* CAIAICallback::GetWeaponDefById(int weaponDefId) {
	
//	logT("entering: GetWeaponDefById sAICallback");
	if (weaponDefId < 0) {
		return NULL;
	}
	
	bool doRecreate = weaponDefFrames[weaponDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
//weaponDef->damages = sAICallback->WeaponDef_getDamages(teamId, weaponDefId);
//{
int numTypes = sAICallback->WeaponDef_Damages_getNumTypes(teamId, weaponDefId);
//	logT("GetWeaponDefById 1");
//float* typeDamages = new float[numTypes];
float typeDamages[numTypes];
sAICallback->WeaponDef_Damages_getTypeDamages(teamId, weaponDefId, typeDamages);
//	logT("GetWeaponDefById 2");
//for(int i=0; i < numTypes; ++i) {
//	typeDamages[i] = sAICallback->WeaponDef_Damages_getType(teamId, weaponDefId, i);
//}
DamageArray da(numTypes, typeDamages);
//	logT("GetWeaponDefById 3");
//AIDamageArray tmpDa(numTypes, typeDamages);
//AIDamageArray tmpDa;
//weaponDef->damages = *(reinterpret_cast<DamageArray*>(&tmpDa));
//tmpDa.numTypes = numTypes;
//tmpDa.damages = typeDamages;
//delete tmpDa;
//da.SetTypes(numTypes, typeDamages);
//delete [] typeDamages;
da.paralyzeDamageTime = sAICallback->WeaponDef_Damages_getParalyzeDamageTime(teamId, weaponDefId);
da.impulseFactor = sAICallback->WeaponDef_Damages_getImpulseFactor(teamId, weaponDefId);
da.impulseBoost = sAICallback->WeaponDef_Damages_getImpulseBoost(teamId, weaponDefId);
da.craterMult = sAICallback->WeaponDef_Damages_getCraterMult(teamId, weaponDefId);
da.craterBoost = sAICallback->WeaponDef_Damages_getCraterBoost(teamId, weaponDefId);
//	logT("GetWeaponDefById 4");
//}

	WeaponDef* weaponDef = new WeaponDef(da);
//	WeaponDef* weaponDef = new WeaponDef();
//	logT("GetWeaponDefById 5");
//	logI("GetWeaponDefById 5 defId: %d", weaponDefId);
weaponDef->name = sAICallback->WeaponDef_getName(teamId, weaponDefId);
weaponDef->type = sAICallback->WeaponDef_getType(teamId, weaponDefId);
weaponDef->description = sAICallback->WeaponDef_getDescription(teamId, weaponDefId);
weaponDef->filename = sAICallback->WeaponDef_getFilename(teamId, weaponDefId);
weaponDef->cegTag = sAICallback->WeaponDef_getCegTag(teamId, weaponDefId);
weaponDef->range = sAICallback->WeaponDef_getRange(teamId, weaponDefId);
weaponDef->heightmod = sAICallback->WeaponDef_getHeightMod(teamId, weaponDefId);
weaponDef->accuracy = sAICallback->WeaponDef_getAccuracy(teamId, weaponDefId);
weaponDef->sprayAngle = sAICallback->WeaponDef_getSprayAngle(teamId, weaponDefId);
weaponDef->movingAccuracy = sAICallback->WeaponDef_getMovingAccuracy(teamId, weaponDefId);
weaponDef->targetMoveError = sAICallback->WeaponDef_getTargetMoveError(teamId, weaponDefId);
weaponDef->leadLimit = sAICallback->WeaponDef_getLeadLimit(teamId, weaponDefId);
weaponDef->leadBonus = sAICallback->WeaponDef_getLeadBonus(teamId, weaponDefId);
weaponDef->predictBoost = sAICallback->WeaponDef_getPredictBoost(teamId, weaponDefId);
weaponDef->areaOfEffect = sAICallback->WeaponDef_getAreaOfEffect(teamId, weaponDefId);
weaponDef->noSelfDamage = sAICallback->WeaponDef_isNoSelfDamage(teamId, weaponDefId);
weaponDef->fireStarter = sAICallback->WeaponDef_getFireStarter(teamId, weaponDefId);
weaponDef->edgeEffectiveness = sAICallback->WeaponDef_getEdgeEffectiveness(teamId, weaponDefId);
weaponDef->size = sAICallback->WeaponDef_getSize(teamId, weaponDefId);
weaponDef->sizeGrowth = sAICallback->WeaponDef_getSizeGrowth(teamId, weaponDefId);
weaponDef->collisionSize = sAICallback->WeaponDef_getCollisionSize(teamId, weaponDefId);
weaponDef->salvosize = sAICallback->WeaponDef_getSalvoSize(teamId, weaponDefId);
weaponDef->salvodelay = sAICallback->WeaponDef_getSalvoDelay(teamId, weaponDefId);
weaponDef->reload = sAICallback->WeaponDef_getReload(teamId, weaponDefId);
weaponDef->beamtime = sAICallback->WeaponDef_getBeamTime(teamId, weaponDefId);
weaponDef->beamburst = sAICallback->WeaponDef_isBeamBurst(teamId, weaponDefId);
weaponDef->waterBounce = sAICallback->WeaponDef_isWaterBounce(teamId, weaponDefId);
weaponDef->groundBounce = sAICallback->WeaponDef_isGroundBounce(teamId, weaponDefId);
weaponDef->bounceRebound = sAICallback->WeaponDef_getBounceRebound(teamId, weaponDefId);
weaponDef->bounceSlip = sAICallback->WeaponDef_getBounceSlip(teamId, weaponDefId);
weaponDef->numBounce = sAICallback->WeaponDef_getNumBounce(teamId, weaponDefId);
weaponDef->maxAngle = sAICallback->WeaponDef_getMaxAngle(teamId, weaponDefId);
weaponDef->restTime = sAICallback->WeaponDef_getRestTime(teamId, weaponDefId);
weaponDef->uptime = sAICallback->WeaponDef_getUpTime(teamId, weaponDefId);
weaponDef->flighttime = sAICallback->WeaponDef_getFlightTime(teamId, weaponDefId);
weaponDef->metalcost = sAICallback->WeaponDef_getMetalCost(teamId, weaponDefId);
weaponDef->energycost = sAICallback->WeaponDef_getEnergyCost(teamId, weaponDefId);
weaponDef->supplycost = sAICallback->WeaponDef_getSupplyCost(teamId, weaponDefId);
weaponDef->projectilespershot = sAICallback->WeaponDef_getProjectilesPerShot(teamId, weaponDefId);
weaponDef->id = sAICallback->WeaponDef_getId(teamId, weaponDefId);
weaponDef->tdfId = sAICallback->WeaponDef_getTdfId(teamId, weaponDefId);
weaponDef->turret = sAICallback->WeaponDef_isTurret(teamId, weaponDefId);
weaponDef->onlyForward = sAICallback->WeaponDef_isOnlyForward(teamId, weaponDefId);
weaponDef->fixedLauncher = sAICallback->WeaponDef_isFixedLauncher(teamId, weaponDefId);
weaponDef->waterweapon = sAICallback->WeaponDef_isWaterWeapon(teamId, weaponDefId);
weaponDef->fireSubmersed = sAICallback->WeaponDef_isFireSubmersed(teamId, weaponDefId);
weaponDef->submissile = sAICallback->WeaponDef_isSubMissile(teamId, weaponDefId);
weaponDef->tracks = sAICallback->WeaponDef_isTracks(teamId, weaponDefId);
weaponDef->dropped = sAICallback->WeaponDef_isDropped(teamId, weaponDefId);
weaponDef->paralyzer = sAICallback->WeaponDef_isParalyzer(teamId, weaponDefId);
weaponDef->impactOnly = sAICallback->WeaponDef_isImpactOnly(teamId, weaponDefId);
weaponDef->noAutoTarget = sAICallback->WeaponDef_isNoAutoTarget(teamId, weaponDefId);
weaponDef->manualfire = sAICallback->WeaponDef_isManualFire(teamId, weaponDefId);
weaponDef->interceptor = sAICallback->WeaponDef_getInterceptor(teamId, weaponDefId);
weaponDef->targetable = sAICallback->WeaponDef_getTargetable(teamId, weaponDefId);
weaponDef->stockpile = sAICallback->WeaponDef_isStockpileable(teamId, weaponDefId);
weaponDef->coverageRange = sAICallback->WeaponDef_getCoverageRange(teamId, weaponDefId);
weaponDef->stockpileTime = sAICallback->WeaponDef_getStockpileTime(teamId, weaponDefId);
weaponDef->intensity = sAICallback->WeaponDef_getIntensity(teamId, weaponDefId);
weaponDef->thickness = sAICallback->WeaponDef_getThickness(teamId, weaponDefId);
weaponDef->laserflaresize = sAICallback->WeaponDef_getLaserFlareSize(teamId, weaponDefId);
weaponDef->corethickness = sAICallback->WeaponDef_getCoreThickness(teamId, weaponDefId);
weaponDef->duration = sAICallback->WeaponDef_getDuration(teamId, weaponDefId);
weaponDef->lodDistance = sAICallback->WeaponDef_getLodDistance(teamId, weaponDefId);
weaponDef->falloffRate = sAICallback->WeaponDef_getFalloffRate(teamId, weaponDefId);
weaponDef->graphicsType = sAICallback->WeaponDef_getGraphicsType(teamId, weaponDefId);
weaponDef->soundTrigger = sAICallback->WeaponDef_isSoundTrigger(teamId, weaponDefId);
weaponDef->selfExplode = sAICallback->WeaponDef_isSelfExplode(teamId, weaponDefId);
weaponDef->gravityAffected = sAICallback->WeaponDef_isGravityAffected(teamId, weaponDefId);
weaponDef->highTrajectory = sAICallback->WeaponDef_getHighTrajectory(teamId, weaponDefId);
weaponDef->myGravity = sAICallback->WeaponDef_getMyGravity(teamId, weaponDefId);
weaponDef->noExplode = sAICallback->WeaponDef_isNoExplode(teamId, weaponDefId);
weaponDef->startvelocity = sAICallback->WeaponDef_getStartVelocity(teamId, weaponDefId);
weaponDef->weaponacceleration = sAICallback->WeaponDef_getWeaponAcceleration(teamId, weaponDefId);
weaponDef->turnrate = sAICallback->WeaponDef_getTurnRate(teamId, weaponDefId);
weaponDef->maxvelocity = sAICallback->WeaponDef_getMaxVelocity(teamId, weaponDefId);
weaponDef->projectilespeed = sAICallback->WeaponDef_getProjectileSpeed(teamId, weaponDefId);
weaponDef->explosionSpeed = sAICallback->WeaponDef_getExplosionSpeed(teamId, weaponDefId);
weaponDef->onlyTargetCategory = sAICallback->WeaponDef_getOnlyTargetCategory(teamId, weaponDefId);
weaponDef->wobble = sAICallback->WeaponDef_getWobble(teamId, weaponDefId);
weaponDef->dance = sAICallback->WeaponDef_getDance(teamId, weaponDefId);
weaponDef->trajectoryHeight = sAICallback->WeaponDef_getTrajectoryHeight(teamId, weaponDefId);
weaponDef->largeBeamLaser = sAICallback->WeaponDef_isLargeBeamLaser(teamId, weaponDefId);
weaponDef->isShield = sAICallback->WeaponDef_isShield(teamId, weaponDefId);
weaponDef->shieldRepulser = sAICallback->WeaponDef_isShieldRepulser(teamId, weaponDefId);
weaponDef->smartShield = sAICallback->WeaponDef_isSmartShield(teamId, weaponDefId);
weaponDef->exteriorShield = sAICallback->WeaponDef_isExteriorShield(teamId, weaponDefId);
weaponDef->visibleShield = sAICallback->WeaponDef_isVisibleShield(teamId, weaponDefId);
weaponDef->visibleShieldRepulse = sAICallback->WeaponDef_isVisibleShieldRepulse(teamId, weaponDefId);
weaponDef->visibleShieldHitFrames = sAICallback->WeaponDef_getVisibleShieldHitFrames(teamId, weaponDefId);
weaponDef->shieldEnergyUse = sAICallback->WeaponDef_getShieldEnergyUse(teamId, weaponDefId);
weaponDef->shieldRadius = sAICallback->WeaponDef_getShieldRadius(teamId, weaponDefId);
weaponDef->shieldForce = sAICallback->WeaponDef_getShieldForce(teamId, weaponDefId);
weaponDef->shieldMaxSpeed = sAICallback->WeaponDef_getShieldMaxSpeed(teamId, weaponDefId);
weaponDef->shieldPower = sAICallback->WeaponDef_getShieldPower(teamId, weaponDefId);
weaponDef->shieldPowerRegen = sAICallback->WeaponDef_getShieldPowerRegen(teamId, weaponDefId);
weaponDef->shieldPowerRegenEnergy = sAICallback->WeaponDef_getShieldPowerRegenEnergy(teamId, weaponDefId);
weaponDef->shieldStartingPower = sAICallback->WeaponDef_getShieldStartingPower(teamId, weaponDefId);
weaponDef->shieldRechargeDelay = sAICallback->WeaponDef_getShieldRechargeDelay(teamId, weaponDefId);
weaponDef->shieldGoodColor = float3(sAICallback->WeaponDef_getShieldGoodColor(teamId, weaponDefId));
weaponDef->shieldBadColor = float3(sAICallback->WeaponDef_getShieldBadColor(teamId, weaponDefId));
weaponDef->shieldAlpha = sAICallback->WeaponDef_getShieldAlpha(teamId, weaponDefId);
weaponDef->shieldInterceptType = sAICallback->WeaponDef_getShieldInterceptType(teamId, weaponDefId);
weaponDef->interceptedByShieldType = sAICallback->WeaponDef_getInterceptedByShieldType(teamId, weaponDefId);
weaponDef->avoidFriendly = sAICallback->WeaponDef_isAvoidFriendly(teamId, weaponDefId);
weaponDef->avoidFeature = sAICallback->WeaponDef_isAvoidFeature(teamId, weaponDefId);
weaponDef->avoidNeutral = sAICallback->WeaponDef_isAvoidNeutral(teamId, weaponDefId);
weaponDef->targetBorder = sAICallback->WeaponDef_getTargetBorder(teamId, weaponDefId);
weaponDef->cylinderTargetting = sAICallback->WeaponDef_getCylinderTargetting(teamId, weaponDefId);
weaponDef->minIntensity = sAICallback->WeaponDef_getMinIntensity(teamId, weaponDefId);
weaponDef->heightBoostFactor = sAICallback->WeaponDef_getHeightBoostFactor(teamId, weaponDefId);
weaponDef->proximityPriority = sAICallback->WeaponDef_getProximityPriority(teamId, weaponDefId);
weaponDef->collisionFlags = sAICallback->WeaponDef_getCollisionFlags(teamId, weaponDefId);
weaponDef->sweepFire = sAICallback->WeaponDef_isSweepFire(teamId, weaponDefId);
weaponDef->canAttackGround = sAICallback->WeaponDef_isCanAttackGround(teamId, weaponDefId);
weaponDef->cameraShake = sAICallback->WeaponDef_getCameraShake(teamId, weaponDefId);
weaponDef->dynDamageExp = sAICallback->WeaponDef_getDynDamageExp(teamId, weaponDefId);
weaponDef->dynDamageMin = sAICallback->WeaponDef_getDynDamageMin(teamId, weaponDefId);
weaponDef->dynDamageRange = sAICallback->WeaponDef_getDynDamageRange(teamId, weaponDefId);
weaponDef->dynDamageInverted = sAICallback->WeaponDef_isDynDamageInverted(teamId, weaponDefId);
//	logT("GetWeaponDefById 6");
//{
//	SProperties* sProperties = sAICallback->WeaponDef_getCustomParams(teamId, weaponDefId);
//	weaponDef->customParams = std::map<std::string,std::string>();
//	int i;
//	for (i=0; i < sProperties->size; ++i) {
//		weaponDef->customParams.insert(sProperties->map[i][0], sProperties->map[i][1]);
//	}
//	free(sProperties->map);
//}
{
	int size = sAICallback->WeaponDef_getNumCustomParams(teamId, weaponDefId);
	weaponDef->customParams = std::map<std::string,std::string>();
//	logT("GetWeaponDefById 7");
//	logI("GetWeaponDefById 7 size: %d", size);
//	int i;
//	for (i=0; i < size; ++i) {
//		const char** entry = sAICallback->WeaponDef_getCustomParam(teamId, weaponDefId, i);
////		weaponDef->customParams.insert(entry[0], entry[1]);
//		weaponDef->customParams[entry[0]] = entry[1];
////		free(entry);
//	}
//	const char*** cMap = (const char***) malloc(size*2*sizeof(char*));
	const char* cMap[size][2];
	sAICallback->WeaponDef_getCustomParams(teamId, weaponDefId, cMap);
//	logT("GetWeaponDefById 8");
//	logI("GetWeaponDefById 8 size: %d", size);
	int i;
	for (i=0; i < size; ++i) {
//	logI("GetWeaponDefById 8 i: %d", i);
		weaponDef->customParams[cMap[i][0]] = cMap[i][1];
	}
//	free(cMap);
}
//	logT("GetWeaponDefById 9");
	if (weaponDefs[weaponDefId] != NULL) {
		delete weaponDefs[weaponDefId];
	}
//	logT("GetWeaponDefById 10");
		weaponDefs[weaponDefId] = weaponDef;
//	logT("GetWeaponDefById 11");
		weaponDefFrames[weaponDefId] = currentFrame;
//	logT("GetWeaponDefById 12");
	}

//	logT("leaving: GetWeaponDefById sAICallback");
	return weaponDefs[weaponDefId];
}

const float3* CAIAICallback::GetStartPos() {
	return new float3(sAICallback->Map_getStartPos(teamId));
}







void CAIAICallback::SendTextMsg(const char* text, int zone) {
	SSendTextMessageCommand cmd = {text, zone};
	sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &cmd);
}

void CAIAICallback::SetLastMsgPos(float3 pos) {
	SSetLastPosMessageCommand cmd = {pos.toSAIFloat3()}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &cmd);
}

void CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {
	SAddNotificationDrawerCommand cmd = {pos.toSAIFloat3(), color.toSAIFloat3(), alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &cmd);
}

bool CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {
		SSendResourcesCommand cmd = {mAmount, eAmount, receivingTeam}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &cmd); return cmd.ret_isExecuted;
}

int CAIAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeam) {
	int arr_unitIds[unitIds.size()];
	for (unsigned int i=0; i < unitIds.size(); ++i) {
		arr_unitIds[i] = unitIds[i];
	}
	SSendUnitsCommand cmd = {arr_unitIds, unitIds.size(), receivingTeam}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &cmd); return cmd.ret_sentUnits;
}

void* CAIAICallback::CreateSharedMemArea(char* name, int size) {
		SCreateSharedMemAreaCommand cmd = {name, size}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_CREATE, &cmd); return cmd.ret_sharedMemArea;
}

void CAIAICallback::ReleasedSharedMemArea(char* name) {
	SReleaseSharedMemAreaCommand cmd = {name}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_RELEASE, &cmd);
}

int CAIAICallback::CreateGroup(char* libraryName, unsigned aiNumber) {
		SCreateGroupCommand cmd = {libraryName, aiNumber}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &cmd); return cmd.ret_groupId;
}

void CAIAICallback::EraseGroup(int groupId) {
	SEraseGroupCommand cmd = {groupId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &cmd);
}

bool CAIAICallback::AddUnitToGroup(int unitId, int groupId) {
		SAddUnitToGroupCommand cmd = {unitId, groupId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ADD_UNIT, &cmd); return cmd.ret_isExecuted;
}

bool CAIAICallback::RemoveUnitFromGroup(int unitId) {
		SRemoveUnitFromGroupCommand cmd = {unitId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_REMOVE_UNIT, &cmd); return cmd.ret_isExecuted;
}

int CAIAICallback::GiveGroupOrder(int groupId, Command* c) {
	return this->Internal_GiveOrder(-1, groupId, c);
}

int CAIAICallback::GiveOrder(int unitId, Command* c) {
	return this->Internal_GiveOrder(unitId, -1, c);
}

int CAIAICallback::Internal_GiveOrder(int unitId, int groupId, Command* c) {
	
/*
	int ret = -1;
	
	switch (c->id) {
        case CMD_STOP:
		{
			SStopUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_STOP, &cmd);
			break;
		}
		case CMD_WAIT:
		{
			SWaitUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT, &cmd);
			break;
		}
		case CMD_TIMEWAIT:
		{
			STimeWaitUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_TIME, &cmd);
			break;
		}
		case CMD_DEATHWAIT:
		{
			SDeathWaitUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_DEATH, &cmd);
			break;
		}
		case CMD_SQUADWAIT:
		{
			SSquadWaitUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_SQUAD, &cmd);
			break;
		}
		case CMD_GATHERWAIT:
		{
			SGatherWaitUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_WAIT_GATHER, &cmd);
			break;
		}
		case CMD_MOVE:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SMoveUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toPos};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_MOVE, &cmd);
			break;
		}
		case CMD_PATROL:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SPatrolUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toPos};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_PATROL, &cmd);
			break;
		}
		case CMD_FIGHT:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SFightUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toPos};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_FIGHT, &cmd);
			break;
		}
		case CMD_ATTACK:
		{
			if (c->params.size() < 3) {
				SAttackUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_ATTACK, &cmd);
			} else {
				SAIFloat3 toAttackPos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SAttackAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toAttackPos, radius};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_ATTACK_AREA, &cmd);
			}
			break;
		}
		case CMD_GUARD:
		{
			SGuardUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GUARD, &cmd);
			break;
		}
		case CMD_AISELECT:
		{
			SAiSelectUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_AI_SELECT, &cmd);
			break;
		}
		case CMD_GROUPADD:
		{
			SGroupAddUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_ADD, &cmd);
			break;
		}
		case CMD_GROUPCLEAR:
		{
			SGroupClearUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_CLEAR, &cmd);
			break;
		}
		case CMD_REPAIR:
		{
			SRepairUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_REPAIR, &cmd);
			break;
		}
		case CMD_FIRE_STATE:
		{
			SSetFireStateUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_FIRE_STATE, &cmd);
			break;
		}
		case CMD_MOVE_STATE:
		{
			SSetMoveStateUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_MOVE_STATE, &cmd);
			break;
		}
		case CMD_SETBASE:
		{
			SAIFloat3 basePos = {c->params[0], c->params[1], c->params[2]};
			SSetBaseUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, basePos};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_BASE, &cmd);
			break;
		}
		case CMD_SELFD:
		{
			SSelfDestroyUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SELF_DESTROY, &cmd);
			break;
		}
		case CMD_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_WANTED_MAX_SPEED, &cmd);
			break;
		}
		case CMD_LOAD_UNITS:
		{
			if (c->params.size() < 3) {
				int toLoadUnitId = (int) c->params[0];
				SLoadUnitsUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, &toLoadUnitId, 1};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_UNITS, &cmd);
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SLoadUnitsAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos, radius};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_UNITS_AREA, &cmd);
			}
			break;
		}
		case CMD_LOAD_ONTO:
		{
			SLoadOntoUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_LOAD_ONTO, &cmd);
			break;
		}
		case CMD_UNLOAD_UNIT:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SUnloadUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toPos, (int) c->params[3]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_UNLOAD_UNIT, &cmd);
			break;
		}
		case CMD_UNLOAD_UNITS:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SUnloadUnitsAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toPos, c->params[3]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_UNLOAD_UNITS_AREA, &cmd);
			break;
		}
		case CMD_ONOFF:
		{
			SSetOnOffUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (bool) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_ON_OFF, &cmd);
			break;
		}
		case CMD_RECLAIM:
		{
			if (c->params.size() < 3) {
				SReclaimUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RECLAIM, &cmd);
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SReclaimAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos, radius};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RECLAIM_AREA, &cmd);
			}
			break;
		}
		case CMD_CLOAK:
		{
			SCloakUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (bool) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CLOAK, &cmd);
			break;
		}
		case CMD_STOCKPILE:
		{
			SStockpileUnitCommand cmd = {unitId, groupId, c->options, c->timeOut};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_STOCKPILE, &cmd);
			break;
		}
		case CMD_DGUN:
		{
			if (c->params.size() < 3) {
				SDGunUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_D_GUN, &cmd);
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				SDGunPosUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_D_GUN_POS, &cmd);
			}
			break;
		}
		case CMD_RESTORE:
		{
			SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
			SRestoreAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos, c->params[3]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESTORE_AREA, &cmd);
			break;
		}
		case CMD_REPEAT:
		{
			SSetRepeatUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (bool) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_REPEAT, &cmd);
			break;
		}
		case CMD_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_TRAJECTORY, &cmd);
			break;
		}
		case CMD_RESURRECT:
		{
			if (c->params.size() < 3) {
				SResurrectUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESURRECT, &cmd);
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SResurrectAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos, radius};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_RESURRECT_AREA, &cmd);
			}
			break;
		}
		case CMD_CAPTURE:
		{
			if (c->params.size() < 3) {
				SCaptureUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CAPTURE, &cmd);
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SCaptureAreaUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, pos, radius};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CAPTURE_AREA, &cmd);
			}
			break;
		}
		case CMD_AUTOREPAIRLEVEL:
		{
			SSetAutoRepairLevelUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL, &cmd);
			break;
		}
		case CMD_IDLEMODE:
		{
			SSetIdleModeUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, (int) c->params[0]};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_SET_IDLE_MODE, &cmd);
			break;
		}
		default:
		{
			if (c->id < 0) { // CMD_BUILD
				int toBuildUnitDefId = -c->id;
				SAIFloat3 buildPos = {c->params[0], c->params[1], c->params[2]};
				int facing = UNIT_COMMAND_BUILD_NO_FACING;
				if (c->params.size() >= 4) facing = c->params[3];
				SBuildUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, toBuildUnitDefId, buildPos, facing};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_BUILD, &cmd);
			} else { // CMD_CUSTOM
				int cmdId = c->id;
				int numParams = c->params.size();
				float params[numParams];
				SCustomUnitCommand cmd = {unitId, groupId, c->options, c->timeOut, cmdId, params, numParams};
				ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_CUSTOM, &cmd);
			}
			break;
		}

	}
	
	return ret;
*/
    int sCommandId;
    void* sCommandData = mallocSUnitCommand(unitId, groupId, c, &sCommandId);
    
    int ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, sCommandId, sCommandData);
    
    freeSUnitCommand(sCommandData, sCommandId);
    
    return ret;
}

int CAIAICallback::InitPath(float3 start, float3 end, int pathType) {
		SInitPathCommand cmd = {start.toSAIFloat3(), end.toSAIFloat3(), pathType}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &cmd); return cmd.ret_pathId;
}

float3 CAIAICallback::GetNextWaypoint(int pathId) {
		SGetNextWaypointPathCommand cmd = {pathId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &cmd); return float3(cmd.ret_nextWaypoint);
}

float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType) {
		SGetApproximateLengthPathCommand cmd = {start.toSAIFloat3(), end.toSAIFloat3(), pathType}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &cmd); return cmd.ret_approximatePathLength;
}

void CAIAICallback::FreePath(int pathId) {
	SFreePathCommand cmd = {pathId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &cmd);
}

void CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SStartPathDrawerCommand cmd = {pos.toSAIFloat3(), col3, alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &cmd);
}

void CAIAICallback::LineDrawerFinishPath() {
	SFinishPathDrawerCommand cmd = {}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &cmd);
}

void CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SDrawLinePathDrawerCommand cmd = {endPos.toSAIFloat3(), col3, alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &cmd);
}

void CAIAICallback::LineDrawerDrawLineAndIcon(int cmdId, const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SDrawLineAndIconPathDrawerCommand cmd = {cmdId, endPos.toSAIFloat3(), col3, alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &cmd);
}

void CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdId) {
	SDrawIconAtLastPosPathDrawerCommand cmd = {cmdId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &cmd);
}

void CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SBreakPathDrawerCommand cmd = {endPos.toSAIFloat3(), col3, alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &cmd);
}

void CAIAICallback::LineDrawerRestart() {
	SRestartPathDrawerCommand cmd = {false}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

void CAIAICallback::LineDrawerRestartSameColor() {
	SRestartPathDrawerCommand cmd = {true}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

int CAIAICallback::CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifeTime, int figureGroupId) {
		SCreateSplineFigureDrawerCommand cmd = {pos1.toSAIFloat3(), pos2.toSAIFloat3(), pos3.toSAIFloat3(), pos4.toSAIFloat3(), width, arrow, lifeTime, figureGroupId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &cmd); return cmd.ret_newFigureGroupId;
}

int CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) {
		SCreateLineFigureDrawerCommand cmd = {pos1.toSAIFloat3(), pos2.toSAIFloat3(), width, arrow, lifeTime, figureGroupId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &cmd); return cmd.ret_newFigureGroupId;
}

void CAIAICallback::SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) {
	SAIFloat3 col3 = {red, green, blue};
	SSetColorFigureDrawerCommand cmd = {figureGroupId, col3, alpha}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &cmd);
}

void CAIAICallback::DeleteFigureGroup(int figureGroupId) {
	SDeleteFigureDrawerCommand cmd = {figureGroupId}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &cmd);
}

void CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifeTime, int unitTeamId, bool transparent, bool drawBorder, int facing) {
	SDrawUnitDrawerCommand cmd = {sAICallback->UnitDef_STATIC_getIdByName(teamId, name), pos.toSAIFloat3(), rotation, lifeTime, unitTeamId, transparent, drawBorder, facing}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &cmd);
}

int CAIAICallback::HandleCommand(int commandId, void* data) {
	
	int cmdTopicIndex = commandId;
	int ret = -99;
	
	switch (commandId) {
		case AIHCQuerySubVersionId: {
//			SQuerySubVersionCommand cmd;
//			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicIndex, &cmd);
			ret = sAICallback->Game_getAiInterfaceVersion(teamId);
			break;
		}
		case AIHCAddMapPointId: {
			AIHCAddMapPoint* myData = (AIHCAddMapPoint*) data;
			SAddPointDrawCommand cmd = {myData->pos.toSAIFloat3(), myData->label};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicIndex, &cmd);
			break;
		}
		case AIHCAddMapLineId: {
			AIHCAddMapLine* myData = (AIHCAddMapLine*) data;
			SAddLineDrawCommand cmd = {myData->posfrom.toSAIFloat3(), myData->posto.toSAIFloat3()};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicIndex, &cmd);
			break;
		}
		case AIHCRemoveMapPointId: {
			AIHCRemoveMapPoint* myData = (AIHCRemoveMapPoint*) data;
			SRemovePointDrawCommand cmd = {myData->pos.toSAIFloat3()};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicIndex, &cmd);
			break;
		}
		case AIHCSendStartPosId: {
			AIHCSendStartPos* myData = (AIHCSendStartPos*) data;
			SSendStartPosCommand cmd = {myData->ready, myData->pos.toSAIFloat3()};
			ret = sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicIndex, &cmd);
			break;
		}
	}
	
	return ret;
}

bool CAIAICallback::ReadFile(const char* filename, void* buffer, int bufferLen) {
//		SReadFileCommand cmd = {name, buffer, bufferLen}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_READ_FILE, &cmd); return cmd.ret_isExecuted;
	return sAICallback->File_getContent(teamId, filename, buffer, bufferLen);
}

const char* CAIAICallback::CallLuaRules(const char* data, int inSize, int* outSize) {
		SCallLuaRulesCommand cmd = {data, inSize, outSize}; sAICallback->handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_RULES, &cmd); return cmd.ret_outData;
}


