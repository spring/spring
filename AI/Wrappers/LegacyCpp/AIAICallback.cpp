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


#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/AISCommands.h"

#include "creg/creg_cond.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#ifdef USING_CREG
creg::Class* CCommandQueue::GetClass() { return NULL; }
#endif
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
#include <string.h>
#include <cassert>


static inline void fillWithNULL(void** arr, int size) {
	for (int i=0; i < size; ++i) {
		arr[i] = NULL;
	}
}
static inline void fillWithMinusOne(int* arr, int size) {
	for (int i=0; i < size; ++i) {
		arr[i] = -1;
	}
}

static int resIndMetal = -1;
static int resIndEnergy = -1;
static inline int getResourceId_Metal(const SSkirmishAICallback* sAICallback, int teamId) {

	if (resIndMetal == -1) {
		resIndMetal = sAICallback->Clb_0MULTI1FETCH3ResourceByName0Resource(teamId, "Metal");
	}

	return resIndMetal;
}
static inline int getResourceId_Energy(const SSkirmishAICallback* sAICallback, int teamId) {

	if (resIndEnergy == -1) {
		resIndEnergy = sAICallback->Clb_0MULTI1FETCH3ResourceByName0Resource(teamId, "Energy");
	}

	return resIndEnergy;
}


CAIAICallback::CAIAICallback()
	: IAICallback(), teamId(-1), sAICallback(NULL) {
	init();
}

CAIAICallback::CAIAICallback(int teamId, const SSkirmishAICallback* sAICallback)
	: IAICallback(), teamId(teamId), sAICallback(sAICallback) {
	init();
}


void CAIAICallback::init() {
	// FIXME: group ID's have no runtime bound
	const int maxGroups = MAX_UNITS;

	// FIXME: these are far too generous, but we
	// don't have easy access to the right values
	// on the AI side (so better waste memory than
	// risk SEGVs)
	const int numUnitDefs = MAX_UNITS; // unitDefHandler->numUnitDefs;
	const int numFeatDefs = MAX_UNITS; // featureHandler->numFeatureDefs;
	const int numWeapDefs = MAX_UNITS; // weaponDefHandler->numWeaponDefs;

	weaponDefs      = new WeaponDef*[numWeapDefs];
	weaponDefFrames = new int[numWeapDefs];

	fillWithNULL((void**) weaponDefs, numWeapDefs);
	fillWithMinusOne(weaponDefFrames, numWeapDefs);


	unitDefs      = new UnitDef*[numUnitDefs];
	unitDefFrames = new int[numUnitDefs];

	fillWithNULL((void**) unitDefs, numUnitDefs);
	fillWithMinusOne(unitDefFrames, numUnitDefs);


	groupPossibleCommands    = new std::vector<CommandDescription>*[maxGroups];
	unitPossibleCommands     = new std::vector<CommandDescription>*[MAX_UNITS];
	unitCurrentCommandQueues = new CCommandQueue*[MAX_UNITS];

	fillWithNULL((void**) groupPossibleCommands,    maxGroups);
	fillWithNULL((void**) unitPossibleCommands,     MAX_UNITS);
	fillWithNULL((void**) unitCurrentCommandQueues, MAX_UNITS);


	featureDefs      = new FeatureDef*[numFeatDefs];
	featureDefFrames = new int[numFeatDefs];

	fillWithNULL((void**) featureDefs, numFeatDefs);
	fillWithMinusOne(featureDefFrames, numFeatDefs);
}


bool CAIAICallback::PosInCamera(float3 pos, float radius) {
	return sAICallback->Clb_Map_isPosInCamera(teamId, pos.toSAIFloat3(), radius);
}

int CAIAICallback::GetCurrentFrame() {
	return sAICallback->Clb_Game_getCurrentFrame(teamId);
}

int CAIAICallback::GetMyTeam() {
	return sAICallback->Clb_Game_getMyTeam(teamId);
}

int CAIAICallback::GetMyAllyTeam() {
	return sAICallback->Clb_Game_getMyAllyTeam(teamId);
}

int CAIAICallback::GetPlayerTeam(int player) {
	return sAICallback->Clb_Game_getPlayerTeam(teamId, player);
}

const char* CAIAICallback::GetTeamSide(int team) {
	return sAICallback->Clb_Game_getTeamSide(teamId, team);
}

int CAIAICallback::GetUnitGroup(int unitId) {
	return sAICallback->Clb_Unit_getGroup(teamId, unitId);
}

const std::vector<CommandDescription>* CAIAICallback::GetGroupCommands(int groupId) {

	int numCmds = sAICallback->Clb_Group_0MULTI1SIZE0SupportedCommand(teamId, groupId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Clb_Group_SupportedCommand_getId(teamId, groupId, c);
		commandDescription.name = sAICallback->Clb_Group_SupportedCommand_getName(teamId, groupId, c);
		commandDescription.tooltip = sAICallback->Clb_Group_SupportedCommand_getToolTip(teamId, groupId, c);
		commandDescription.showUnique = sAICallback->Clb_Group_SupportedCommand_isShowUnique(teamId, groupId, c);
		commandDescription.disabled = sAICallback->Clb_Group_SupportedCommand_isDisabled(teamId, groupId, c);

		int numParams = sAICallback->Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams(teamId, groupId, c);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Clb_Group_SupportedCommand_0ARRAY1VALS0getParams(teamId, groupId, c, params, numParams);
		for (int p=0; p < numParams; p++) {
			commandDescription.params.push_back(params[p]);
		}
		free(params);
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

	int numCmds = sAICallback->Clb_Unit_0MULTI1SIZE0SupportedCommand(teamId, unitId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Clb_Unit_SupportedCommand_getId(teamId, unitId, c);
		commandDescription.name = sAICallback->Clb_Unit_SupportedCommand_getName(teamId, unitId, c);
		commandDescription.tooltip = sAICallback->Clb_Unit_SupportedCommand_getToolTip(teamId, unitId, c);
		commandDescription.showUnique = sAICallback->Clb_Unit_SupportedCommand_isShowUnique(teamId, unitId, c);
		commandDescription.disabled = sAICallback->Clb_Unit_SupportedCommand_isDisabled(teamId, unitId, c);

		int numParams = sAICallback->Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams(teamId, unitId, c);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams(teamId, unitId, c, params, numParams);
		for (int p=0; p < numParams; p++) {
			commandDescription.params.push_back(params[p]);
		}
		free(params);
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

	int numCmds = sAICallback->Clb_Unit_0MULTI1SIZE1Command0CurrentCommand(teamId, unitId);
	int type = sAICallback->Clb_Unit_CurrentCommand_0STATIC0getType(teamId, unitId);

	CCommandQueue* cc = new CCommandQueue();
	cc->queueType = (CCommandQueue::QueueType) type;
	for (int c=0; c < numCmds; c++) {
		Command command;
		command.id = sAICallback->Clb_Unit_CurrentCommand_getId(teamId, unitId, c);
		command.options = sAICallback->Clb_Unit_CurrentCommand_getOptions(teamId, unitId, c);
		command.tag = sAICallback->Clb_Unit_CurrentCommand_getTag(teamId, unitId, c);
		command.timeOut = sAICallback->Clb_Unit_CurrentCommand_getTimeOut(teamId, unitId, c);

		int numParams = sAICallback->Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams(teamId, unitId, c);
		float* params = (float*) calloc(numParams, sizeof(float));
		numParams = sAICallback->Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams(teamId, unitId, c, params, numParams);
		for (int p=0; p < numParams; p++) {
			command.params.push_back(params[p]);
		}
		free(params);

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
	return sAICallback->Clb_Unit_getAiHint(teamId, unitId);
}

int CAIAICallback::GetUnitTeam(int unitId) {
	return sAICallback->Clb_Unit_getTeam(teamId, unitId);
}

int CAIAICallback::GetUnitAllyTeam(int unitId) {
	return sAICallback->Clb_Unit_getAllyTeam(teamId, unitId);
}

float CAIAICallback::GetUnitHealth(int unitId) {
	return sAICallback->Clb_Unit_getHealth(teamId, unitId);
}

float CAIAICallback::GetUnitMaxHealth(int unitId) {
	return sAICallback->Clb_Unit_getMaxHealth(teamId, unitId);
}

float CAIAICallback::GetUnitSpeed(int unitId) {
	return sAICallback->Clb_Unit_getSpeed(teamId, unitId);
}

float CAIAICallback::GetUnitPower(int unitId) {
	return sAICallback->Clb_Unit_getPower(teamId, unitId);
}

float CAIAICallback::GetUnitExperience(int unitId) {
	return sAICallback->Clb_Unit_getExperience(teamId, unitId);
}

float CAIAICallback::GetUnitMaxRange(int unitId) {
	return sAICallback->Clb_Unit_getMaxRange(teamId, unitId);
}

bool CAIAICallback::IsUnitActivated(int unitId) {
	return sAICallback->Clb_Unit_isActivated(teamId, unitId);
}

bool CAIAICallback::UnitBeingBuilt(int unitId) {
	return sAICallback->Clb_Unit_isBeingBuilt(teamId, unitId);
}

const UnitDef* CAIAICallback::GetUnitDef(int unitId) {
	int unitDefId = sAICallback->Clb_Unit_0SINGLE1FETCH2UnitDef0getDef(teamId, unitId);
	return this->GetUnitDefById(unitDefId);
}

float3 CAIAICallback::GetUnitPos(int unitId) {
	return float3(sAICallback->Clb_Unit_getPos(teamId, unitId));
}

int CAIAICallback::GetBuildingFacing(int unitId) {
	return sAICallback->Clb_Unit_getBuildingFacing(teamId, unitId);
}

bool CAIAICallback::IsUnitCloaked(int unitId) {
	return sAICallback->Clb_Unit_isCloaked(teamId, unitId);
}

bool CAIAICallback::IsUnitParalyzed(int unitId) {
	return sAICallback->Clb_Unit_isParalyzed(teamId, unitId);
}

bool CAIAICallback::IsUnitNeutral(int unitId) {
	return sAICallback->Clb_Unit_isNeutral(teamId, unitId);
}

bool CAIAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {

	static int m = getResourceId_Metal(sAICallback, teamId);
	static int e = getResourceId_Energy(sAICallback, teamId);
	resourceInfo->energyMake = sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceMake(teamId, unitId, e);
	if (resourceInfo->energyMake < 0) { return false; }
	resourceInfo->energyUse = sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceUse(teamId, unitId, e);
	resourceInfo->metalMake = sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceMake(teamId, unitId, m);
	resourceInfo->metalUse = sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceUse(teamId, unitId, m);
	return true;
}

const UnitDef* CAIAICallback::GetUnitDef(const char* unitName) {
	int unitDefId = sAICallback->Clb_0MULTI1FETCH3UnitDefByName0UnitDef(teamId, unitName);
	return this->GetUnitDefById(unitDefId);
}


const UnitDef* CAIAICallback::GetUnitDefById(int unitDefId) {
	//logT("entering: GetUnitDefById sAICallback");
	static int m = getResourceId_Metal(sAICallback, teamId);
	static int e = getResourceId_Energy(sAICallback, teamId);

	if (unitDefId < 0) {
		return NULL;
	}

	bool doRecreate = unitDefFrames[unitDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
	UnitDef* unitDef = new UnitDef();
unitDef->valid = sAICallback->Clb_UnitDef_isValid(teamId, unitDefId);
unitDef->name = sAICallback->Clb_UnitDef_getName(teamId, unitDefId);
unitDef->humanName = sAICallback->Clb_UnitDef_getHumanName(teamId, unitDefId);
unitDef->filename = sAICallback->Clb_UnitDef_getFileName(teamId, unitDefId);
//unitDef->id = sAICallback->Clb_UnitDef_getId(teamId, unitDefId);
unitDef->id = unitDefId;
unitDef->aihint = sAICallback->Clb_UnitDef_getAiHint(teamId, unitDefId);
unitDef->cobID = sAICallback->Clb_UnitDef_getCobId(teamId, unitDefId);
unitDef->techLevel = sAICallback->Clb_UnitDef_getTechLevel(teamId, unitDefId);
unitDef->gaia = sAICallback->Clb_UnitDef_getGaia(teamId, unitDefId);
unitDef->metalUpkeep = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getUpkeep(teamId, unitDefId, m);
unitDef->energyUpkeep = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getUpkeep(teamId, unitDefId, e);
unitDef->metalMake = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getResourceMake(teamId, unitDefId, m);
unitDef->makesMetal = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getMakesResource(teamId, unitDefId, m);
unitDef->energyMake = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getResourceMake(teamId, unitDefId, e);
unitDef->metalCost = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getCost(teamId, unitDefId, m);
unitDef->energyCost = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getCost(teamId, unitDefId, e);
unitDef->buildTime = sAICallback->Clb_UnitDef_getBuildTime(teamId, unitDefId);
unitDef->extractsMetal = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource(teamId, unitDefId, m);
unitDef->extractRange = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange(teamId, unitDefId, m);
unitDef->windGenerator = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator(teamId, unitDefId, e);
unitDef->tidalGenerator = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator(teamId, unitDefId, e);
unitDef->metalStorage = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getStorage(teamId, unitDefId, m);
unitDef->energyStorage = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getStorage(teamId, unitDefId, e);
unitDef->autoHeal = sAICallback->Clb_UnitDef_getAutoHeal(teamId, unitDefId);
unitDef->idleAutoHeal = sAICallback->Clb_UnitDef_getIdleAutoHeal(teamId, unitDefId);
unitDef->idleTime = sAICallback->Clb_UnitDef_getIdleTime(teamId, unitDefId);
unitDef->power = sAICallback->Clb_UnitDef_getPower(teamId, unitDefId);
unitDef->health = sAICallback->Clb_UnitDef_getHealth(teamId, unitDefId);
unitDef->category = sAICallback->Clb_UnitDef_getCategory(teamId, unitDefId);
unitDef->speed = sAICallback->Clb_UnitDef_getSpeed(teamId, unitDefId);
unitDef->turnRate = sAICallback->Clb_UnitDef_getTurnRate(teamId, unitDefId);
unitDef->turnInPlace = sAICallback->Clb_UnitDef_isTurnInPlace(teamId, unitDefId);
unitDef->upright = sAICallback->Clb_UnitDef_isUpright(teamId, unitDefId);
unitDef->collide = sAICallback->Clb_UnitDef_isCollide(teamId, unitDefId);
unitDef->controlRadius = sAICallback->Clb_UnitDef_getControlRadius(teamId, unitDefId);
unitDef->losRadius = sAICallback->Clb_UnitDef_getLosRadius(teamId, unitDefId);
unitDef->airLosRadius = sAICallback->Clb_UnitDef_getAirLosRadius(teamId, unitDefId);
unitDef->losHeight = sAICallback->Clb_UnitDef_getLosHeight(teamId, unitDefId);
unitDef->radarRadius = sAICallback->Clb_UnitDef_getRadarRadius(teamId, unitDefId);
unitDef->sonarRadius = sAICallback->Clb_UnitDef_getSonarRadius(teamId, unitDefId);
unitDef->jammerRadius = sAICallback->Clb_UnitDef_getJammerRadius(teamId, unitDefId);
unitDef->sonarJamRadius = sAICallback->Clb_UnitDef_getSonarJamRadius(teamId, unitDefId);
unitDef->seismicRadius = sAICallback->Clb_UnitDef_getSeismicRadius(teamId, unitDefId);
unitDef->seismicSignature = sAICallback->Clb_UnitDef_getSeismicSignature(teamId, unitDefId);
unitDef->stealth = sAICallback->Clb_UnitDef_isStealth(teamId, unitDefId);
unitDef->sonarStealth = sAICallback->Clb_UnitDef_isSonarStealth(teamId, unitDefId);
unitDef->buildRange3D = sAICallback->Clb_UnitDef_isBuildRange3D(teamId, unitDefId);
unitDef->buildDistance = sAICallback->Clb_UnitDef_getBuildDistance(teamId, unitDefId);
unitDef->buildSpeed = sAICallback->Clb_UnitDef_getBuildSpeed(teamId, unitDefId);
unitDef->reclaimSpeed = sAICallback->Clb_UnitDef_getReclaimSpeed(teamId, unitDefId);
unitDef->repairSpeed = sAICallback->Clb_UnitDef_getRepairSpeed(teamId, unitDefId);
unitDef->maxRepairSpeed = sAICallback->Clb_UnitDef_getMaxRepairSpeed(teamId, unitDefId);
unitDef->resurrectSpeed = sAICallback->Clb_UnitDef_getResurrectSpeed(teamId, unitDefId);
unitDef->captureSpeed = sAICallback->Clb_UnitDef_getCaptureSpeed(teamId, unitDefId);
unitDef->terraformSpeed = sAICallback->Clb_UnitDef_getTerraformSpeed(teamId, unitDefId);
unitDef->mass = sAICallback->Clb_UnitDef_getMass(teamId, unitDefId);
unitDef->pushResistant = sAICallback->Clb_UnitDef_isPushResistant(teamId, unitDefId);
unitDef->strafeToAttack = sAICallback->Clb_UnitDef_isStrafeToAttack(teamId, unitDefId);
unitDef->minCollisionSpeed = sAICallback->Clb_UnitDef_getMinCollisionSpeed(teamId, unitDefId);
unitDef->slideTolerance = sAICallback->Clb_UnitDef_getSlideTolerance(teamId, unitDefId);
unitDef->maxSlope = sAICallback->Clb_UnitDef_getMaxSlope(teamId, unitDefId);
unitDef->maxHeightDif = sAICallback->Clb_UnitDef_getMaxHeightDif(teamId, unitDefId);
unitDef->minWaterDepth = sAICallback->Clb_UnitDef_getMinWaterDepth(teamId, unitDefId);
unitDef->waterline = sAICallback->Clb_UnitDef_getWaterline(teamId, unitDefId);
unitDef->maxWaterDepth = sAICallback->Clb_UnitDef_getMaxWaterDepth(teamId, unitDefId);
unitDef->armoredMultiple = sAICallback->Clb_UnitDef_getArmoredMultiple(teamId, unitDefId);
unitDef->armorType = sAICallback->Clb_UnitDef_getArmorType(teamId, unitDefId);
unitDef->flankingBonusMode = sAICallback->Clb_UnitDef_FlankingBonus_getMode(teamId, unitDefId);
unitDef->flankingBonusDir = float3(sAICallback->Clb_UnitDef_FlankingBonus_getDir(teamId, unitDefId));
unitDef->flankingBonusMax = sAICallback->Clb_UnitDef_FlankingBonus_getMax(teamId, unitDefId);
unitDef->flankingBonusMin = sAICallback->Clb_UnitDef_FlankingBonus_getMin(teamId, unitDefId);
unitDef->flankingBonusMobilityAdd = sAICallback->Clb_UnitDef_FlankingBonus_getMobilityAdd(teamId, unitDefId);
unitDef->collisionVolumeTypeStr = sAICallback->Clb_UnitDef_CollisionVolume_getType(teamId, unitDefId);
unitDef->collisionVolumeScales = float3(sAICallback->Clb_UnitDef_CollisionVolume_getScales(teamId, unitDefId));
unitDef->collisionVolumeOffsets = float3(sAICallback->Clb_UnitDef_CollisionVolume_getOffsets(teamId, unitDefId));
unitDef->collisionVolumeTest = sAICallback->Clb_UnitDef_CollisionVolume_getTest(teamId, unitDefId);
unitDef->maxWeaponRange = sAICallback->Clb_UnitDef_getMaxWeaponRange(teamId, unitDefId);
unitDef->type = sAICallback->Clb_UnitDef_getType(teamId, unitDefId);
unitDef->tooltip = sAICallback->Clb_UnitDef_getTooltip(teamId, unitDefId);
unitDef->wreckName = sAICallback->Clb_UnitDef_getWreckName(teamId, unitDefId);
unitDef->deathExplosion = sAICallback->Clb_UnitDef_getDeathExplosion(teamId, unitDefId);
unitDef->selfDExplosion = sAICallback->Clb_UnitDef_getSelfDExplosion(teamId, unitDefId);
unitDef->TEDClassString = sAICallback->Clb_UnitDef_getTedClassString(teamId, unitDefId);
unitDef->categoryString = sAICallback->Clb_UnitDef_getCategoryString(teamId, unitDefId);
unitDef->canSelfD = sAICallback->Clb_UnitDef_isAbleToSelfD(teamId, unitDefId);
unitDef->selfDCountdown = sAICallback->Clb_UnitDef_getSelfDCountdown(teamId, unitDefId);
unitDef->canSubmerge = sAICallback->Clb_UnitDef_isAbleToSubmerge(teamId, unitDefId);
unitDef->canfly = sAICallback->Clb_UnitDef_isAbleToFly(teamId, unitDefId);
unitDef->canmove = sAICallback->Clb_UnitDef_isAbleToMove(teamId, unitDefId);
unitDef->canhover = sAICallback->Clb_UnitDef_isAbleToHover(teamId, unitDefId);
unitDef->floater = sAICallback->Clb_UnitDef_isFloater(teamId, unitDefId);
unitDef->builder = sAICallback->Clb_UnitDef_isBuilder(teamId, unitDefId);
unitDef->activateWhenBuilt = sAICallback->Clb_UnitDef_isActivateWhenBuilt(teamId, unitDefId);
unitDef->onoffable = sAICallback->Clb_UnitDef_isOnOffable(teamId, unitDefId);
unitDef->fullHealthFactory = sAICallback->Clb_UnitDef_isFullHealthFactory(teamId, unitDefId);
unitDef->factoryHeadingTakeoff = sAICallback->Clb_UnitDef_isFactoryHeadingTakeoff(teamId, unitDefId);
unitDef->reclaimable = sAICallback->Clb_UnitDef_isReclaimable(teamId, unitDefId);
unitDef->capturable = sAICallback->Clb_UnitDef_isCapturable(teamId, unitDefId);
unitDef->canRestore = sAICallback->Clb_UnitDef_isAbleToRestore(teamId, unitDefId);
unitDef->canRepair = sAICallback->Clb_UnitDef_isAbleToRepair(teamId, unitDefId);
unitDef->canSelfRepair = sAICallback->Clb_UnitDef_isAbleToSelfRepair(teamId, unitDefId);
unitDef->canReclaim = sAICallback->Clb_UnitDef_isAbleToReclaim(teamId, unitDefId);
unitDef->canAttack = sAICallback->Clb_UnitDef_isAbleToAttack(teamId, unitDefId);
unitDef->canPatrol = sAICallback->Clb_UnitDef_isAbleToPatrol(teamId, unitDefId);
unitDef->canFight = sAICallback->Clb_UnitDef_isAbleToFight(teamId, unitDefId);
unitDef->canGuard = sAICallback->Clb_UnitDef_isAbleToGuard(teamId, unitDefId);
unitDef->canBuild = sAICallback->Clb_UnitDef_isAbleToBuild(teamId, unitDefId);
unitDef->canAssist = sAICallback->Clb_UnitDef_isAbleToAssist(teamId, unitDefId);
unitDef->canBeAssisted = sAICallback->Clb_UnitDef_isAssistable(teamId, unitDefId);
unitDef->canRepeat = sAICallback->Clb_UnitDef_isAbleToRepeat(teamId, unitDefId);
unitDef->canFireControl = sAICallback->Clb_UnitDef_isAbleToFireControl(teamId, unitDefId);
unitDef->fireState = sAICallback->Clb_UnitDef_getFireState(teamId, unitDefId);
unitDef->moveState = sAICallback->Clb_UnitDef_getMoveState(teamId, unitDefId);
unitDef->wingDrag = sAICallback->Clb_UnitDef_getWingDrag(teamId, unitDefId);
unitDef->wingAngle = sAICallback->Clb_UnitDef_getWingAngle(teamId, unitDefId);
unitDef->drag = sAICallback->Clb_UnitDef_getDrag(teamId, unitDefId);
unitDef->frontToSpeed = sAICallback->Clb_UnitDef_getFrontToSpeed(teamId, unitDefId);
unitDef->speedToFront = sAICallback->Clb_UnitDef_getSpeedToFront(teamId, unitDefId);
unitDef->myGravity = sAICallback->Clb_UnitDef_getMyGravity(teamId, unitDefId);
unitDef->maxBank = sAICallback->Clb_UnitDef_getMaxBank(teamId, unitDefId);
unitDef->maxPitch = sAICallback->Clb_UnitDef_getMaxPitch(teamId, unitDefId);
unitDef->turnRadius = sAICallback->Clb_UnitDef_getTurnRadius(teamId, unitDefId);
unitDef->wantedHeight = sAICallback->Clb_UnitDef_getWantedHeight(teamId, unitDefId);
unitDef->verticalSpeed = sAICallback->Clb_UnitDef_getVerticalSpeed(teamId, unitDefId);
unitDef->canCrash = sAICallback->Clb_UnitDef_isAbleToCrash(teamId, unitDefId);
unitDef->hoverAttack = sAICallback->Clb_UnitDef_isHoverAttack(teamId, unitDefId);
unitDef->airStrafe = sAICallback->Clb_UnitDef_isAirStrafe(teamId, unitDefId);
unitDef->dlHoverFactor = sAICallback->Clb_UnitDef_getDlHoverFactor(teamId, unitDefId);
unitDef->maxAcc = sAICallback->Clb_UnitDef_getMaxAcceleration(teamId, unitDefId);
unitDef->maxDec = sAICallback->Clb_UnitDef_getMaxDeceleration(teamId, unitDefId);
unitDef->maxAileron = sAICallback->Clb_UnitDef_getMaxAileron(teamId, unitDefId);
unitDef->maxElevator = sAICallback->Clb_UnitDef_getMaxElevator(teamId, unitDefId);
unitDef->maxRudder = sAICallback->Clb_UnitDef_getMaxRudder(teamId, unitDefId);
//unitDef->yardmaps = sAICallback->Clb_UnitDef_getYardMaps(teamId, unitDefId);
unitDef->xsize = sAICallback->Clb_UnitDef_getXSize(teamId, unitDefId);
unitDef->zsize = sAICallback->Clb_UnitDef_getZSize(teamId, unitDefId);
unitDef->buildangle = sAICallback->Clb_UnitDef_getBuildAngle(teamId, unitDefId);
unitDef->loadingRadius = sAICallback->Clb_UnitDef_getLoadingRadius(teamId, unitDefId);
unitDef->unloadSpread = sAICallback->Clb_UnitDef_getUnloadSpread(teamId, unitDefId);
unitDef->transportCapacity = sAICallback->Clb_UnitDef_getTransportCapacity(teamId, unitDefId);
unitDef->transportSize = sAICallback->Clb_UnitDef_getTransportSize(teamId, unitDefId);
unitDef->minTransportSize = sAICallback->Clb_UnitDef_getMinTransportSize(teamId, unitDefId);
unitDef->isAirBase = sAICallback->Clb_UnitDef_isAirBase(teamId, unitDefId);
unitDef->transportMass = sAICallback->Clb_UnitDef_getTransportMass(teamId, unitDefId);
unitDef->minTransportMass = sAICallback->Clb_UnitDef_getMinTransportMass(teamId, unitDefId);
unitDef->holdSteady = sAICallback->Clb_UnitDef_isHoldSteady(teamId, unitDefId);
unitDef->releaseHeld = sAICallback->Clb_UnitDef_isReleaseHeld(teamId, unitDefId);
unitDef->cantBeTransported = sAICallback->Clb_UnitDef_isNotTransportable(teamId, unitDefId);
unitDef->transportByEnemy = sAICallback->Clb_UnitDef_isTransportByEnemy(teamId, unitDefId);
unitDef->transportUnloadMethod = sAICallback->Clb_UnitDef_getTransportUnloadMethod(teamId, unitDefId);
unitDef->fallSpeed = sAICallback->Clb_UnitDef_getFallSpeed(teamId, unitDefId);
unitDef->unitFallSpeed = sAICallback->Clb_UnitDef_getUnitFallSpeed(teamId, unitDefId);
unitDef->canCloak = sAICallback->Clb_UnitDef_isAbleToCloak(teamId, unitDefId);
unitDef->startCloaked = sAICallback->Clb_UnitDef_isStartCloaked(teamId, unitDefId);
unitDef->cloakCost = sAICallback->Clb_UnitDef_getCloakCost(teamId, unitDefId);
unitDef->cloakCostMoving = sAICallback->Clb_UnitDef_getCloakCostMoving(teamId, unitDefId);
unitDef->decloakDistance = sAICallback->Clb_UnitDef_getDecloakDistance(teamId, unitDefId);
unitDef->decloakSpherical = sAICallback->Clb_UnitDef_isDecloakSpherical(teamId, unitDefId);
unitDef->decloakOnFire = sAICallback->Clb_UnitDef_isDecloakOnFire(teamId, unitDefId);
unitDef->canKamikaze = sAICallback->Clb_UnitDef_isAbleToKamikaze(teamId, unitDefId);
unitDef->kamikazeDist = sAICallback->Clb_UnitDef_getKamikazeDist(teamId, unitDefId);
unitDef->targfac = sAICallback->Clb_UnitDef_isTargetingFacility(teamId, unitDefId);
unitDef->canDGun = sAICallback->Clb_UnitDef_isAbleToDGun(teamId, unitDefId);
unitDef->needGeo = sAICallback->Clb_UnitDef_isNeedGeo(teamId, unitDefId);
unitDef->isFeature = sAICallback->Clb_UnitDef_isFeature(teamId, unitDefId);
unitDef->hideDamage = sAICallback->Clb_UnitDef_isHideDamage(teamId, unitDefId);
unitDef->isCommander = sAICallback->Clb_UnitDef_isCommander(teamId, unitDefId);
unitDef->showPlayerName = sAICallback->Clb_UnitDef_isShowPlayerName(teamId, unitDefId);
unitDef->canResurrect = sAICallback->Clb_UnitDef_isAbleToResurrect(teamId, unitDefId);
unitDef->canCapture = sAICallback->Clb_UnitDef_isAbleToCapture(teamId, unitDefId);
unitDef->highTrajectoryType = sAICallback->Clb_UnitDef_getHighTrajectoryType(teamId, unitDefId);
unitDef->noChaseCategory = sAICallback->Clb_UnitDef_getNoChaseCategory(teamId, unitDefId);
unitDef->leaveTracks = sAICallback->Clb_UnitDef_isLeaveTracks(teamId, unitDefId);
unitDef->trackWidth = sAICallback->Clb_UnitDef_getTrackWidth(teamId, unitDefId);
unitDef->trackOffset = sAICallback->Clb_UnitDef_getTrackOffset(teamId, unitDefId);
unitDef->trackStrength = sAICallback->Clb_UnitDef_getTrackStrength(teamId, unitDefId);
unitDef->trackStretch = sAICallback->Clb_UnitDef_getTrackStretch(teamId, unitDefId);
unitDef->trackType = sAICallback->Clb_UnitDef_getTrackType(teamId, unitDefId);
unitDef->canDropFlare = sAICallback->Clb_UnitDef_isAbleToDropFlare(teamId, unitDefId);
unitDef->flareReloadTime = sAICallback->Clb_UnitDef_getFlareReloadTime(teamId, unitDefId);
unitDef->flareEfficiency = sAICallback->Clb_UnitDef_getFlareEfficiency(teamId, unitDefId);
unitDef->flareDelay = sAICallback->Clb_UnitDef_getFlareDelay(teamId, unitDefId);
unitDef->flareDropVector = float3(sAICallback->Clb_UnitDef_getFlareDropVector(teamId, unitDefId));
unitDef->flareTime = sAICallback->Clb_UnitDef_getFlareTime(teamId, unitDefId);
unitDef->flareSalvoSize = sAICallback->Clb_UnitDef_getFlareSalvoSize(teamId, unitDefId);
unitDef->flareSalvoDelay = sAICallback->Clb_UnitDef_getFlareSalvoDelay(teamId, unitDefId);
//unitDef->smoothAnim = sAICallback->Clb_UnitDef_isSmoothAnim(teamId, unitDefId);
unitDef->smoothAnim = false;
unitDef->isMetalMaker = sAICallback->Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker(teamId, unitDefId, m);
unitDef->canLoopbackAttack = sAICallback->Clb_UnitDef_isAbleToLoopbackAttack(teamId, unitDefId);
unitDef->levelGround = sAICallback->Clb_UnitDef_isLevelGround(teamId, unitDefId);
unitDef->useBuildingGroundDecal = sAICallback->Clb_UnitDef_isUseBuildingGroundDecal(teamId, unitDefId);
unitDef->buildingDecalType = sAICallback->Clb_UnitDef_getBuildingDecalType(teamId, unitDefId);
unitDef->buildingDecalSizeX = sAICallback->Clb_UnitDef_getBuildingDecalSizeX(teamId, unitDefId);
unitDef->buildingDecalSizeY = sAICallback->Clb_UnitDef_getBuildingDecalSizeY(teamId, unitDefId);
unitDef->buildingDecalDecaySpeed = sAICallback->Clb_UnitDef_getBuildingDecalDecaySpeed(teamId, unitDefId);
unitDef->isFirePlatform = sAICallback->Clb_UnitDef_isFirePlatform(teamId, unitDefId);
unitDef->maxFuel = sAICallback->Clb_UnitDef_getMaxFuel(teamId, unitDefId);
unitDef->refuelTime = sAICallback->Clb_UnitDef_getRefuelTime(teamId, unitDefId);
unitDef->minAirBasePower = sAICallback->Clb_UnitDef_getMinAirBasePower(teamId, unitDefId);
unitDef->maxThisUnit = sAICallback->Clb_UnitDef_getMaxThisUnit(teamId, unitDefId);
//unitDef->decoyDef = sAICallback->Clb_UnitDef_getDecoyDefId(teamId, unitDefId);
unitDef->shieldWeaponDef = this->GetWeaponDefById(sAICallback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef(teamId, unitDefId));
unitDef->stockpileWeaponDef = this->GetWeaponDefById(sAICallback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef(teamId, unitDefId));
{
	int numBo = sAICallback->Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions(teamId, unitDefId);
	int* bo = new int[numBo];
	numBo = sAICallback->Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions(teamId, unitDefId, bo, numBo);
	for (int b=0; b < numBo; b++) {
		unitDef->buildOptions[b] = sAICallback->Clb_UnitDef_getName(teamId, bo[b]);
	}
	delete [] bo;
}
{
	int size = sAICallback->Clb_UnitDef_0MAP1SIZE0getCustomParams(teamId, unitDefId);
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->Clb_UnitDef_0MAP1KEYS0getCustomParams(teamId, unitDefId, cKeys);
	sAICallback->Clb_UnitDef_0MAP1VALS0getCustomParams(teamId, unitDefId, cValues);
	int i;
	for (i=0; i < size; ++i) {
		unitDef->customParams[cKeys[i]] = cValues[i];
	}
	free(cKeys);
	free(cValues);
}
if (sAICallback->Clb_UnitDef_0AVAILABLE0MoveData(teamId, unitDefId)) {
	unitDef->movedata = new MoveData(NULL, -1);
		unitDef->movedata->moveType = (enum MoveData::MoveType)sAICallback->Clb_UnitDef_MoveData_getMoveType(teamId, unitDefId);
		unitDef->movedata->moveFamily = (enum MoveData::MoveFamily) sAICallback->Clb_UnitDef_MoveData_getMoveFamily(teamId, unitDefId);
		unitDef->movedata->size = sAICallback->Clb_UnitDef_MoveData_getSize(teamId, unitDefId);
		unitDef->movedata->depth = sAICallback->Clb_UnitDef_MoveData_getDepth(teamId, unitDefId);
		unitDef->movedata->maxSlope = sAICallback->Clb_UnitDef_MoveData_getMaxSlope(teamId, unitDefId);
		unitDef->movedata->slopeMod = sAICallback->Clb_UnitDef_MoveData_getSlopeMod(teamId, unitDefId);
		unitDef->movedata->depthMod = sAICallback->Clb_UnitDef_MoveData_getDepthMod(teamId, unitDefId);
		unitDef->movedata->pathType = sAICallback->Clb_UnitDef_MoveData_getPathType(teamId, unitDefId);
		unitDef->movedata->crushStrength = sAICallback->Clb_UnitDef_MoveData_getCrushStrength(teamId, unitDefId);
		unitDef->movedata->maxSpeed = sAICallback->Clb_UnitDef_MoveData_getMaxSpeed(teamId, unitDefId);
		unitDef->movedata->maxTurnRate = sAICallback->Clb_UnitDef_MoveData_getMaxTurnRate(teamId, unitDefId);
		unitDef->movedata->maxAcceleration = sAICallback->Clb_UnitDef_MoveData_getMaxAcceleration(teamId, unitDefId);
		unitDef->movedata->maxBreaking = sAICallback->Clb_UnitDef_MoveData_getMaxBreaking(teamId, unitDefId);
		unitDef->movedata->subMarine = sAICallback->Clb_UnitDef_MoveData_isSubMarine(teamId, unitDefId);
	} else {
		unitDef->movedata = NULL;
	}
int numWeapons = sAICallback->Clb_UnitDef_0MULTI1SIZE0WeaponMount(teamId, unitDefId);
for (int w=0; w < numWeapons; ++w) {
	unitDef->weapons.push_back(UnitDef::UnitDefWeapon());
	unitDef->weapons[w].name = sAICallback->Clb_UnitDef_WeaponMount_getName(teamId, unitDefId, w);
	int weaponDefId = sAICallback->Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef(teamId, unitDefId, w);
	unitDef->weapons[w].def = this->GetWeaponDefById(weaponDefId);
	unitDef->weapons[w].slavedTo = sAICallback->Clb_UnitDef_WeaponMount_getSlavedTo(teamId, unitDefId, w);
	unitDef->weapons[w].mainDir = float3(sAICallback->Clb_UnitDef_WeaponMount_getMainDir(teamId, unitDefId, w));
	unitDef->weapons[w].maxAngleDif = sAICallback->Clb_UnitDef_WeaponMount_getMaxAngleDif(teamId, unitDefId, w);
	unitDef->weapons[w].fuelUsage = sAICallback->Clb_UnitDef_WeaponMount_getFuelUsage(teamId, unitDefId, w);
	unitDef->weapons[w].badTargetCat = sAICallback->Clb_UnitDef_WeaponMount_getBadTargetCategory(teamId, unitDefId, w);
	unitDef->weapons[w].onlyTargetCat = sAICallback->Clb_UnitDef_WeaponMount_getOnlyTargetCategory(teamId, unitDefId, w);
}
	if (unitDefs[unitDefId] != NULL) {
		delete unitDefs[unitDefId];
	}
		unitDefs[unitDefId] = unitDef;
		unitDefFrames[unitDefId] = currentFrame;
	}

	return unitDefs[unitDefId];
}

int CAIAICallback::GetEnemyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3EnemyUnits0Unit(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3EnemyUnitsIn0Unit(teamId, pos.toSAIFloat3(), radius, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3FriendlyUnits0Unit(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3FriendlyUnitsIn0Unit(teamId, pos.toSAIFloat3(), radius, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3NeutralUnits0Unit(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3NeutralUnitsIn0Unit(teamId, pos.toSAIFloat3(), radius, unitIds, unitIds_max);
}

int CAIAICallback::GetMapWidth() {
	return sAICallback->Clb_Map_getWidth(teamId);
}

int CAIAICallback::GetMapHeight() {
	return sAICallback->Clb_Map_getHeight(teamId);
}

const float* CAIAICallback::GetHeightMap() {

	static float* heightMap = NULL;

	if (heightMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0getHeightMap(teamId);
		heightMap = new float[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0getHeightMap(teamId, heightMap, size);
	}

	return heightMap;
}

float CAIAICallback::GetMinHeight() {
	return sAICallback->Clb_Map_getMinHeight(teamId);
}

float CAIAICallback::GetMaxHeight() {
	return sAICallback->Clb_Map_getMaxHeight(teamId);
}

const float* CAIAICallback::GetSlopeMap() {

	static float* slopeMap = NULL;

	if (slopeMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0getSlopeMap(teamId);
		slopeMap = new float[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0getSlopeMap(teamId, slopeMap, size);
	}

	return slopeMap;
}

const unsigned short* CAIAICallback::GetLosMap() {

	static unsigned short* losMap = NULL;

	if (losMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0getLosMap(teamId);
		losMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0getLosMap(teamId, losMap, size);
	}

	return losMap;
}

int CAIAICallback::GetLosMapResolution() {

	int fullSize = GetMapWidth() * GetMapHeight();
	int losSize = sAICallback->Clb_Map_0ARRAY1SIZE0getLosMap(teamId);

	return fullSize / losSize;
}

const unsigned short* CAIAICallback::GetRadarMap() {

	static unsigned short* radarMap = NULL;

	if (radarMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0getRadarMap(teamId);
		radarMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0getRadarMap(teamId, radarMap, size);
	}

	return radarMap;
}

const unsigned short* CAIAICallback::GetJammerMap() {

	static unsigned short* jammerMap = NULL;

	if (jammerMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0getJammerMap(teamId);
		jammerMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0getJammerMap(teamId, jammerMap, size);
	}

	return jammerMap;
}

const unsigned char* CAIAICallback::GetMetalMap() {

	static unsigned char* metalMap = NULL;
	static int m = getResourceId_Metal(sAICallback, teamId);

	if (metalMap == NULL) {
		int size = sAICallback->Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw(teamId, m);
		metalMap = new unsigned char[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw(teamId, m, metalMap, size);
	}

	return metalMap;
}

const char* CAIAICallback::GetMapName() {
	return sAICallback->Clb_Map_getName(teamId);
}

const char* CAIAICallback::GetModName() {
	return sAICallback->Clb_Mod_getFileName(teamId);
}

float CAIAICallback::GetElevation(float x, float z) {
	return sAICallback->Clb_Map_getElevationAt(teamId, x, z);
}

float CAIAICallback::GetMaxMetal() {
	static int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Map_0REF1Resource2resourceId0getMaxResource(teamId, m);
}

float CAIAICallback::GetExtractorRadius() {
	static int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Map_0REF1Resource2resourceId0getExtractorRadius(teamId, m);
}

float CAIAICallback::GetMinWind() {
	return sAICallback->Clb_Map_getMinWind(teamId);
}

float CAIAICallback::GetMaxWind() {
	return sAICallback->Clb_Map_getMaxWind(teamId);
}

float CAIAICallback::GetTidalStrength() {
	return sAICallback->Clb_Map_getTidalStrength(teamId);
}

float CAIAICallback::GetGravity() {
	return sAICallback->Clb_Map_getGravity(teamId);
}

bool CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {
	return sAICallback->Clb_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt(teamId, unitDef->id, pos.toSAIFloat3(), facing);
}

float3 CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing) {
	return float3(sAICallback->Clb_Map_0REF1UnitDef2unitDefId0findClosestBuildSite(teamId, unitDef->id, pos.toSAIFloat3(), searchRadius, minDist, facing));
}

/*
bool CAIAICallback::GetProperty(int id, int property, void* dst) {
//	return sAICallback->Clb_getProperty(teamId, id, property, dst);
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
			(*(float*)data) = sAICallback->Clb_Unit_getCurrentFuel(teamId, unitId);
			return (*(float*)data) != -1.0f;
		}
		case AIVAL_STOCKPILED: {
			(*(int*)data) = sAICallback->Clb_Unit_getStockpile(teamId, unitId);
			return (*(int*)data) != -1;
		}
		case AIVAL_STOCKPILE_QUED: {
			(*(int*)data) = sAICallback->Clb_Unit_getStockpileQueued(teamId, unitId);
			return (*(int*)data) != -1;
		}
		case AIVAL_UNIT_MAXSPEED: {
			(*(float*) data) = sAICallback->Clb_Unit_getMaxSpeed(teamId, unitId);
			return (*(float*)data) != -1.0f;
		}
		default:
			return false;
	}
	return false;
}

/*
bool CAIAICallback::GetValue(int valueId, void* dst) {
//	return sAICallback->Clb_getValue(teamId, valueId, dst);
	return false;
}
*/
bool CAIAICallback::GetValue(int valueId, void *data)
{
	switch (valueId) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = sAICallback->Clb_WeaponDef_0STATIC0getNumDamageTypes(teamId);
			return true;
		}case AIVAL_EXCEPTION_HANDLING:{
			*(bool*)data = sAICallback->Clb_Game_isExceptionHandlingEnabled(teamId);
			return true;
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = sAICallback->Clb_Map_getChecksum(teamId);
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = sAICallback->Clb_Game_isDebugModeEnabled(teamId);
			return true;
		}case AIVAL_GAME_MODE:{
			*(int*)data = sAICallback->Clb_Game_getMode(teamId);
			return true;
		}case AIVAL_GAME_PAUSED:{
			*(bool*)data = sAICallback->Clb_Game_isPaused(teamId);
			return true;
		}case AIVAL_GAME_SPEED_FACTOR:{
			*(float*)data = sAICallback->Clb_Game_getSpeedFactor(teamId);
			return true;
		}case AIVAL_GUI_VIEW_RANGE:{
			*(float*)data = sAICallback->Clb_Gui_getViewRange(teamId);
			return true;
		}case AIVAL_GUI_SCREENX:{
			*(float*)data = sAICallback->Clb_Gui_getScreenX(teamId);
			return true;
		}case AIVAL_GUI_SCREENY:{
			*(float*)data = sAICallback->Clb_Gui_getScreenY(teamId);
			return true;
		}case AIVAL_GUI_CAMERA_DIR:{
			*(float3*)data = sAICallback->Clb_Gui_Camera_getDirection(teamId);
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			*(float3*)data = sAICallback->Clb_Gui_Camera_getPosition(teamId);
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			//sAICallback->Clb_File_locateForReading(teamId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->Clb_DataDirs_locatePath(teamId, absPath, absPath_sizeMax, (const char*) data, false, false, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}case AIVAL_LOCATE_FILE_W:{
			//sAICallback->Clb_File_locateForWriting(teamId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->Clb_DataDirs_locatePath(teamId, absPath, absPath_sizeMax, (const char*) data, true, true, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = sAICallback->Clb_Unit_0STATIC0getLimit(teamId);
			return true;
		}
		case AIVAL_SCRIPT: {
			*(const char**) data = sAICallback->Clb_Game_getSetupScript(teamId);
			return true;
		}
		default:
			return false;
	}
}

int CAIAICallback::GetFileSize(const char* name) {
	return sAICallback->Clb_File_getSize(teamId, name);
}

int CAIAICallback::GetSelectedUnits(int* unitIds, int unitIds_max) {
	return sAICallback->Clb_0MULTI1VALS3SelectedUnits0Unit(teamId, unitIds, unitIds_max);
}

float3 CAIAICallback::GetMousePos() {
	return float3(sAICallback->Clb_Map_getMousePos(teamId));
}

int CAIAICallback::GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies) {

	int numPoints = sAICallback->Clb_Map_0MULTI1SIZE0Point(teamId, includeAllies);
	for (int p=0; p < numPoints; ++p) {
		pm[p].pos = float3(sAICallback->Clb_Map_Point_getPosition(teamId, p));
		SAIFloat3 tmpColor = sAICallback->Clb_Map_Point_getColor(teamId, p);
		pm[p].color = (unsigned char*) calloc(3, sizeof(unsigned char));
		pm[p].color[0] = (unsigned char) tmpColor.x;
		pm[p].color[1] = (unsigned char) tmpColor.y;
		pm[p].color[2] = (unsigned char) tmpColor.z;
		pm[p].label = sAICallback->Clb_Map_Point_getLabel(teamId, p);
	}

	return numPoints;
}

int CAIAICallback::GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies) {

	int numLines = sAICallback->Clb_Map_0MULTI1SIZE0Line(teamId, includeAllies);
	for (int l=0; l < numLines; ++l) {
		lm[l].pos = float3(sAICallback->Clb_Map_Line_getFirstPosition(teamId, l));
		lm[l].pos2 = float3(sAICallback->Clb_Map_Line_getSecondPosition(teamId, l));
		SAIFloat3 tmpColor = sAICallback->Clb_Map_Line_getColor(teamId, l);
		lm[l].color = (unsigned char*) calloc(3, sizeof(unsigned char));
		lm[l].color[0] = (unsigned char) tmpColor.x;
		lm[l].color[1] = (unsigned char) tmpColor.y;
		lm[l].color[2] = (unsigned char) tmpColor.z;
	}

	return numLines;
}

float CAIAICallback::GetMetal() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getCurrent(teamId, m);
}

float CAIAICallback::GetMetalIncome() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getIncome(teamId, m);
}

float CAIAICallback::GetMetalUsage() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getUsage(teamId, m);
}

float CAIAICallback::GetMetalStorage() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getStorage(teamId, m);
}

float CAIAICallback::GetEnergy() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getCurrent(teamId, e);
}

float CAIAICallback::GetEnergyIncome() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getIncome(teamId, e);
}

float CAIAICallback::GetEnergyUsage() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getUsage(teamId, e);
}

float CAIAICallback::GetEnergyStorage() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Clb_Economy_0REF1Resource2resourceId0getStorage(teamId, e);
}

int CAIAICallback::GetFeatures(int* featureIds, int featureIds_max) {
	return sAICallback->Clb_0MULTI1VALS0Feature(teamId, featureIds, featureIds_max);
}

int CAIAICallback::GetFeatures(int *featureIds, int featureIds_max, const float3& pos, float radius) {
	return sAICallback->Clb_0MULTI1VALS3FeaturesIn0Feature(teamId, pos.toSAIFloat3(), radius, featureIds, featureIds_max);
}

const FeatureDef* CAIAICallback::GetFeatureDef(int featureId) {
	int featureDefId = sAICallback->Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef(teamId, featureId);
	return this->GetFeatureDefById(featureDefId);
}

const FeatureDef* CAIAICallback::GetFeatureDefById(int featureDefId) {

	static int m = getResourceId_Metal(sAICallback, teamId);
	static int e = getResourceId_Energy(sAICallback, teamId);

	if (featureDefId < 0) {
		return NULL;
	}

	bool doRecreate = featureDefFrames[featureDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
	FeatureDef* featureDef = new FeatureDef();
featureDef->myName = sAICallback->Clb_FeatureDef_getName(teamId, featureDefId);
featureDef->description = sAICallback->Clb_FeatureDef_getDescription(teamId, featureDefId);
featureDef->filename = sAICallback->Clb_FeatureDef_getFileName(teamId, featureDefId);
//featureDef->id = sAICallback->Clb_FeatureDef_getId(teamId, featureDefId);
featureDef->id = featureDefId;
featureDef->metal = sAICallback->Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource(teamId, featureDefId, m);
featureDef->energy = sAICallback->Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource(teamId, featureDefId, e);
featureDef->maxHealth = sAICallback->Clb_FeatureDef_getMaxHealth(teamId, featureDefId);
featureDef->reclaimTime = sAICallback->Clb_FeatureDef_getReclaimTime(teamId, featureDefId);
featureDef->mass = sAICallback->Clb_FeatureDef_getMass(teamId, featureDefId);
featureDef->collisionVolumeTypeStr = sAICallback->Clb_FeatureDef_CollisionVolume_getType(teamId, featureDefId);
featureDef->collisionVolumeScales = float3(sAICallback->Clb_FeatureDef_CollisionVolume_getScales(teamId, featureDefId));
featureDef->collisionVolumeOffsets = float3(sAICallback->Clb_FeatureDef_CollisionVolume_getOffsets(teamId, featureDefId));
featureDef->collisionVolumeTest = sAICallback->Clb_FeatureDef_CollisionVolume_getTest(teamId, featureDefId);
featureDef->upright = sAICallback->Clb_FeatureDef_isUpright(teamId, featureDefId);
featureDef->drawType = sAICallback->Clb_FeatureDef_getDrawType(teamId, featureDefId);
featureDef->modelname = sAICallback->Clb_FeatureDef_getModelName(teamId, featureDefId);
featureDef->resurrectable = sAICallback->Clb_FeatureDef_getResurrectable(teamId, featureDefId);
featureDef->smokeTime = sAICallback->Clb_FeatureDef_getSmokeTime(teamId, featureDefId);
featureDef->destructable = sAICallback->Clb_FeatureDef_isDestructable(teamId, featureDefId);
featureDef->reclaimable = sAICallback->Clb_FeatureDef_isReclaimable(teamId, featureDefId);
featureDef->blocking = sAICallback->Clb_FeatureDef_isBlocking(teamId, featureDefId);
featureDef->burnable = sAICallback->Clb_FeatureDef_isBurnable(teamId, featureDefId);
featureDef->floating = sAICallback->Clb_FeatureDef_isFloating(teamId, featureDefId);
featureDef->noSelect = sAICallback->Clb_FeatureDef_isNoSelect(teamId, featureDefId);
featureDef->geoThermal = sAICallback->Clb_FeatureDef_isGeoThermal(teamId, featureDefId);
featureDef->deathFeature = sAICallback->Clb_FeatureDef_getDeathFeature(teamId, featureDefId);
featureDef->xsize = sAICallback->Clb_FeatureDef_getXSize(teamId, featureDefId);
featureDef->zsize = sAICallback->Clb_FeatureDef_getZSize(teamId, featureDefId);
{
	int size = sAICallback->Clb_FeatureDef_0MAP1SIZE0getCustomParams(teamId, featureDefId);
	featureDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->Clb_FeatureDef_0MAP1KEYS0getCustomParams(teamId, featureDefId, cKeys);
	sAICallback->Clb_FeatureDef_0MAP1VALS0getCustomParams(teamId, featureDefId, cValues);
	int i;
	for (i=0; i < size; ++i) {
		featureDef->customParams[cKeys[i]] = cValues[i];
	}
	free(cKeys);
	free(cValues);
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
	return sAICallback->Clb_Feature_getHealth(teamId, featureId);
}

float CAIAICallback::GetFeatureReclaimLeft(int featureId) {
	return sAICallback->Clb_Feature_getReclaimLeft(teamId, featureId);
}

float3 CAIAICallback::GetFeaturePos(int featureId) {
	return float3(sAICallback->Clb_Feature_getPosition(teamId, featureId));
}

int CAIAICallback::GetNumUnitDefs() {
	return sAICallback->Clb_0MULTI1SIZE0UnitDef(teamId);
}

void CAIAICallback::GetUnitDefList(const UnitDef** list) {

	int size = sAICallback->Clb_0MULTI1SIZE0UnitDef(teamId);
	int* unitDefIds = new int[size];
	size = sAICallback->Clb_0MULTI1VALS0UnitDef(teamId, unitDefIds, size);
	for (int i=0; i < size; ++i) {
		list[i] = this->GetUnitDefById(unitDefIds[i]);
	}
}

float CAIAICallback::GetUnitDefHeight(int def) {
	return sAICallback->Clb_UnitDef_getHeight(teamId, def);
}

float CAIAICallback::GetUnitDefRadius(int def) {
	return sAICallback->Clb_UnitDef_getRadius(teamId, def);
}

const WeaponDef* CAIAICallback::GetWeapon(const char* weaponName) {
	int weaponDefId = sAICallback->Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef(teamId, weaponName);
	return this->GetWeaponDefById(weaponDefId);
}

const WeaponDef* CAIAICallback::GetWeaponDefById(int weaponDefId) {

	static int m = getResourceId_Metal(sAICallback, teamId);
	static int e = getResourceId_Energy(sAICallback, teamId);

//	logT("entering: GetWeaponDefById sAICallback");
	if (weaponDefId < 0) {
		return NULL;
	}

	bool doRecreate = weaponDefFrames[weaponDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
//weaponDef->damages = sAICallback->Clb_WeaponDef_getDamages(teamId, weaponDefId);
//{
int numTypes = sAICallback->Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes(teamId, weaponDefId);
//	logT("GetWeaponDefById 1");
//float* typeDamages = new float[numTypes];
float* typeDamages = (float*) calloc(numTypes, sizeof(float));
numTypes = sAICallback->Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes(teamId, weaponDefId, typeDamages, numTypes);
//	logT("GetWeaponDefById 2");
//for(int i=0; i < numTypes; ++i) {
//	typeDamages[i] = sAICallback->Clb_WeaponDef_Damages_getType(teamId, weaponDefId, i);
//}
DamageArray da(numTypes, typeDamages);
// DamageArray is copying the array internaly, so it does no harm freeing it here
free(typeDamages);
//	logT("GetWeaponDefById 3");
//AIDamageArray tmpDa(numTypes, typeDamages);
//AIDamageArray tmpDa;
//weaponDef->damages = *(reinterpret_cast<DamageArray*>(&tmpDa));
//tmpDa.numTypes = numTypes;
//tmpDa.damages = typeDamages;
//delete tmpDa;
//da.SetTypes(numTypes, typeDamages);
//delete [] typeDamages;
da.paralyzeDamageTime = sAICallback->Clb_WeaponDef_Damage_getParalyzeDamageTime(teamId, weaponDefId);
da.impulseFactor = sAICallback->Clb_WeaponDef_Damage_getImpulseFactor(teamId, weaponDefId);
da.impulseBoost = sAICallback->Clb_WeaponDef_Damage_getImpulseBoost(teamId, weaponDefId);
da.craterMult = sAICallback->Clb_WeaponDef_Damage_getCraterMult(teamId, weaponDefId);
da.craterBoost = sAICallback->Clb_WeaponDef_Damage_getCraterBoost(teamId, weaponDefId);
//	logT("GetWeaponDefById 4");
//}

	WeaponDef* weaponDef = new WeaponDef(da);
//	WeaponDef* weaponDef = new WeaponDef();
//	logT("GetWeaponDefById 5");
//	logI("GetWeaponDefById 5 defId: %d", weaponDefId);
weaponDef->name = sAICallback->Clb_WeaponDef_getName(teamId, weaponDefId);
weaponDef->type = sAICallback->Clb_WeaponDef_getType(teamId, weaponDefId);
weaponDef->description = sAICallback->Clb_WeaponDef_getDescription(teamId, weaponDefId);
weaponDef->filename = sAICallback->Clb_WeaponDef_getFileName(teamId, weaponDefId);
weaponDef->cegTag = sAICallback->Clb_WeaponDef_getCegTag(teamId, weaponDefId);
weaponDef->range = sAICallback->Clb_WeaponDef_getRange(teamId, weaponDefId);
weaponDef->heightmod = sAICallback->Clb_WeaponDef_getHeightMod(teamId, weaponDefId);
weaponDef->accuracy = sAICallback->Clb_WeaponDef_getAccuracy(teamId, weaponDefId);
weaponDef->sprayAngle = sAICallback->Clb_WeaponDef_getSprayAngle(teamId, weaponDefId);
weaponDef->movingAccuracy = sAICallback->Clb_WeaponDef_getMovingAccuracy(teamId, weaponDefId);
weaponDef->targetMoveError = sAICallback->Clb_WeaponDef_getTargetMoveError(teamId, weaponDefId);
weaponDef->leadLimit = sAICallback->Clb_WeaponDef_getLeadLimit(teamId, weaponDefId);
weaponDef->leadBonus = sAICallback->Clb_WeaponDef_getLeadBonus(teamId, weaponDefId);
weaponDef->predictBoost = sAICallback->Clb_WeaponDef_getPredictBoost(teamId, weaponDefId);
weaponDef->areaOfEffect = sAICallback->Clb_WeaponDef_getAreaOfEffect(teamId, weaponDefId);
weaponDef->noSelfDamage = sAICallback->Clb_WeaponDef_isNoSelfDamage(teamId, weaponDefId);
weaponDef->fireStarter = sAICallback->Clb_WeaponDef_getFireStarter(teamId, weaponDefId);
weaponDef->edgeEffectiveness = sAICallback->Clb_WeaponDef_getEdgeEffectiveness(teamId, weaponDefId);
weaponDef->size = sAICallback->Clb_WeaponDef_getSize(teamId, weaponDefId);
weaponDef->sizeGrowth = sAICallback->Clb_WeaponDef_getSizeGrowth(teamId, weaponDefId);
weaponDef->collisionSize = sAICallback->Clb_WeaponDef_getCollisionSize(teamId, weaponDefId);
weaponDef->salvosize = sAICallback->Clb_WeaponDef_getSalvoSize(teamId, weaponDefId);
weaponDef->salvodelay = sAICallback->Clb_WeaponDef_getSalvoDelay(teamId, weaponDefId);
weaponDef->reload = sAICallback->Clb_WeaponDef_getReload(teamId, weaponDefId);
weaponDef->beamtime = sAICallback->Clb_WeaponDef_getBeamTime(teamId, weaponDefId);
weaponDef->beamburst = sAICallback->Clb_WeaponDef_isBeamBurst(teamId, weaponDefId);
weaponDef->waterBounce = sAICallback->Clb_WeaponDef_isWaterBounce(teamId, weaponDefId);
weaponDef->groundBounce = sAICallback->Clb_WeaponDef_isGroundBounce(teamId, weaponDefId);
weaponDef->bounceRebound = sAICallback->Clb_WeaponDef_getBounceRebound(teamId, weaponDefId);
weaponDef->bounceSlip = sAICallback->Clb_WeaponDef_getBounceSlip(teamId, weaponDefId);
weaponDef->numBounce = sAICallback->Clb_WeaponDef_getNumBounce(teamId, weaponDefId);
weaponDef->maxAngle = sAICallback->Clb_WeaponDef_getMaxAngle(teamId, weaponDefId);
weaponDef->restTime = sAICallback->Clb_WeaponDef_getRestTime(teamId, weaponDefId);
weaponDef->uptime = sAICallback->Clb_WeaponDef_getUpTime(teamId, weaponDefId);
weaponDef->flighttime = sAICallback->Clb_WeaponDef_getFlightTime(teamId, weaponDefId);
weaponDef->metalcost = sAICallback->Clb_WeaponDef_0REF1Resource2resourceId0getCost(teamId, weaponDefId, m);
weaponDef->energycost = sAICallback->Clb_WeaponDef_0REF1Resource2resourceId0getCost(teamId, weaponDefId, e);
weaponDef->supplycost = sAICallback->Clb_WeaponDef_getSupplyCost(teamId, weaponDefId);
weaponDef->projectilespershot = sAICallback->Clb_WeaponDef_getProjectilesPerShot(teamId, weaponDefId);
//weaponDef->id = sAICallback->Clb_WeaponDef_getId(teamId, weaponDefId);
weaponDef->id = weaponDefId;
//weaponDef->tdfId = sAICallback->Clb_WeaponDef_getTdfId(teamId, weaponDefId);
weaponDef->tdfId = -1;
weaponDef->turret = sAICallback->Clb_WeaponDef_isTurret(teamId, weaponDefId);
weaponDef->onlyForward = sAICallback->Clb_WeaponDef_isOnlyForward(teamId, weaponDefId);
weaponDef->fixedLauncher = sAICallback->Clb_WeaponDef_isFixedLauncher(teamId, weaponDefId);
weaponDef->waterweapon = sAICallback->Clb_WeaponDef_isWaterWeapon(teamId, weaponDefId);
weaponDef->fireSubmersed = sAICallback->Clb_WeaponDef_isFireSubmersed(teamId, weaponDefId);
weaponDef->submissile = sAICallback->Clb_WeaponDef_isSubMissile(teamId, weaponDefId);
weaponDef->tracks = sAICallback->Clb_WeaponDef_isTracks(teamId, weaponDefId);
weaponDef->dropped = sAICallback->Clb_WeaponDef_isDropped(teamId, weaponDefId);
weaponDef->paralyzer = sAICallback->Clb_WeaponDef_isParalyzer(teamId, weaponDefId);
weaponDef->impactOnly = sAICallback->Clb_WeaponDef_isImpactOnly(teamId, weaponDefId);
weaponDef->noAutoTarget = sAICallback->Clb_WeaponDef_isNoAutoTarget(teamId, weaponDefId);
weaponDef->manualfire = sAICallback->Clb_WeaponDef_isManualFire(teamId, weaponDefId);
weaponDef->interceptor = sAICallback->Clb_WeaponDef_getInterceptor(teamId, weaponDefId);
weaponDef->targetable = sAICallback->Clb_WeaponDef_getTargetable(teamId, weaponDefId);
weaponDef->stockpile = sAICallback->Clb_WeaponDef_isStockpileable(teamId, weaponDefId);
weaponDef->coverageRange = sAICallback->Clb_WeaponDef_getCoverageRange(teamId, weaponDefId);
weaponDef->stockpileTime = sAICallback->Clb_WeaponDef_getStockpileTime(teamId, weaponDefId);
weaponDef->intensity = sAICallback->Clb_WeaponDef_getIntensity(teamId, weaponDefId);
weaponDef->thickness = sAICallback->Clb_WeaponDef_getThickness(teamId, weaponDefId);
weaponDef->laserflaresize = sAICallback->Clb_WeaponDef_getLaserFlareSize(teamId, weaponDefId);
weaponDef->corethickness = sAICallback->Clb_WeaponDef_getCoreThickness(teamId, weaponDefId);
weaponDef->duration = sAICallback->Clb_WeaponDef_getDuration(teamId, weaponDefId);
weaponDef->lodDistance = sAICallback->Clb_WeaponDef_getLodDistance(teamId, weaponDefId);
weaponDef->falloffRate = sAICallback->Clb_WeaponDef_getFalloffRate(teamId, weaponDefId);
weaponDef->graphicsType = sAICallback->Clb_WeaponDef_getGraphicsType(teamId, weaponDefId);
weaponDef->soundTrigger = sAICallback->Clb_WeaponDef_isSoundTrigger(teamId, weaponDefId);
weaponDef->selfExplode = sAICallback->Clb_WeaponDef_isSelfExplode(teamId, weaponDefId);
weaponDef->gravityAffected = sAICallback->Clb_WeaponDef_isGravityAffected(teamId, weaponDefId);
weaponDef->highTrajectory = sAICallback->Clb_WeaponDef_getHighTrajectory(teamId, weaponDefId);
weaponDef->myGravity = sAICallback->Clb_WeaponDef_getMyGravity(teamId, weaponDefId);
weaponDef->noExplode = sAICallback->Clb_WeaponDef_isNoExplode(teamId, weaponDefId);
weaponDef->startvelocity = sAICallback->Clb_WeaponDef_getStartVelocity(teamId, weaponDefId);
weaponDef->weaponacceleration = sAICallback->Clb_WeaponDef_getWeaponAcceleration(teamId, weaponDefId);
weaponDef->turnrate = sAICallback->Clb_WeaponDef_getTurnRate(teamId, weaponDefId);
weaponDef->maxvelocity = sAICallback->Clb_WeaponDef_getMaxVelocity(teamId, weaponDefId);
weaponDef->projectilespeed = sAICallback->Clb_WeaponDef_getProjectileSpeed(teamId, weaponDefId);
weaponDef->explosionSpeed = sAICallback->Clb_WeaponDef_getExplosionSpeed(teamId, weaponDefId);
weaponDef->onlyTargetCategory = sAICallback->Clb_WeaponDef_getOnlyTargetCategory(teamId, weaponDefId);
weaponDef->wobble = sAICallback->Clb_WeaponDef_getWobble(teamId, weaponDefId);
weaponDef->dance = sAICallback->Clb_WeaponDef_getDance(teamId, weaponDefId);
weaponDef->trajectoryHeight = sAICallback->Clb_WeaponDef_getTrajectoryHeight(teamId, weaponDefId);
weaponDef->largeBeamLaser = sAICallback->Clb_WeaponDef_isLargeBeamLaser(teamId, weaponDefId);
weaponDef->isShield = sAICallback->Clb_WeaponDef_isShield(teamId, weaponDefId);
weaponDef->shieldRepulser = sAICallback->Clb_WeaponDef_isShieldRepulser(teamId, weaponDefId);
weaponDef->smartShield = sAICallback->Clb_WeaponDef_isSmartShield(teamId, weaponDefId);
weaponDef->exteriorShield = sAICallback->Clb_WeaponDef_isExteriorShield(teamId, weaponDefId);
weaponDef->visibleShield = sAICallback->Clb_WeaponDef_isVisibleShield(teamId, weaponDefId);
weaponDef->visibleShieldRepulse = sAICallback->Clb_WeaponDef_isVisibleShieldRepulse(teamId, weaponDefId);
weaponDef->visibleShieldHitFrames = sAICallback->Clb_WeaponDef_getVisibleShieldHitFrames(teamId, weaponDefId);
weaponDef->shieldEnergyUse = sAICallback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse(teamId, weaponDefId, e);
weaponDef->shieldRadius = sAICallback->Clb_WeaponDef_Shield_getRadius(teamId, weaponDefId);
weaponDef->shieldForce = sAICallback->Clb_WeaponDef_Shield_getForce(teamId, weaponDefId);
weaponDef->shieldMaxSpeed = sAICallback->Clb_WeaponDef_Shield_getMaxSpeed(teamId, weaponDefId);
weaponDef->shieldPower = sAICallback->Clb_WeaponDef_Shield_getPower(teamId, weaponDefId);
weaponDef->shieldPowerRegen = sAICallback->Clb_WeaponDef_Shield_getPowerRegen(teamId, weaponDefId);
weaponDef->shieldPowerRegenEnergy = sAICallback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource(teamId, weaponDefId, e);
weaponDef->shieldStartingPower = sAICallback->Clb_WeaponDef_Shield_getStartingPower(teamId, weaponDefId);
weaponDef->shieldRechargeDelay = sAICallback->Clb_WeaponDef_Shield_getRechargeDelay(teamId, weaponDefId);
weaponDef->shieldGoodColor = float3(sAICallback->Clb_WeaponDef_Shield_getGoodColor(teamId, weaponDefId));
weaponDef->shieldBadColor = float3(sAICallback->Clb_WeaponDef_Shield_getBadColor(teamId, weaponDefId));
weaponDef->shieldAlpha = sAICallback->Clb_WeaponDef_Shield_getAlpha(teamId, weaponDefId);
weaponDef->shieldInterceptType = sAICallback->Clb_WeaponDef_Shield_getInterceptType(teamId, weaponDefId);
weaponDef->interceptedByShieldType = sAICallback->Clb_WeaponDef_getInterceptedByShieldType(teamId, weaponDefId);
weaponDef->avoidFriendly = sAICallback->Clb_WeaponDef_isAvoidFriendly(teamId, weaponDefId);
weaponDef->avoidFeature = sAICallback->Clb_WeaponDef_isAvoidFeature(teamId, weaponDefId);
weaponDef->avoidNeutral = sAICallback->Clb_WeaponDef_isAvoidNeutral(teamId, weaponDefId);
weaponDef->targetBorder = sAICallback->Clb_WeaponDef_getTargetBorder(teamId, weaponDefId);
weaponDef->cylinderTargetting = sAICallback->Clb_WeaponDef_getCylinderTargetting(teamId, weaponDefId);
weaponDef->minIntensity = sAICallback->Clb_WeaponDef_getMinIntensity(teamId, weaponDefId);
weaponDef->heightBoostFactor = sAICallback->Clb_WeaponDef_getHeightBoostFactor(teamId, weaponDefId);
weaponDef->proximityPriority = sAICallback->Clb_WeaponDef_getProximityPriority(teamId, weaponDefId);
weaponDef->collisionFlags = sAICallback->Clb_WeaponDef_getCollisionFlags(teamId, weaponDefId);
weaponDef->sweepFire = sAICallback->Clb_WeaponDef_isSweepFire(teamId, weaponDefId);
weaponDef->canAttackGround = sAICallback->Clb_WeaponDef_isAbleToAttackGround(teamId, weaponDefId);
weaponDef->cameraShake = sAICallback->Clb_WeaponDef_getCameraShake(teamId, weaponDefId);
weaponDef->dynDamageExp = sAICallback->Clb_WeaponDef_getDynDamageExp(teamId, weaponDefId);
weaponDef->dynDamageMin = sAICallback->Clb_WeaponDef_getDynDamageMin(teamId, weaponDefId);
weaponDef->dynDamageRange = sAICallback->Clb_WeaponDef_getDynDamageRange(teamId, weaponDefId);
weaponDef->dynDamageInverted = sAICallback->Clb_WeaponDef_isDynDamageInverted(teamId, weaponDefId);
{
	int size = sAICallback->Clb_WeaponDef_0MAP1SIZE0getCustomParams(teamId, weaponDefId);
	weaponDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->Clb_WeaponDef_0MAP1KEYS0getCustomParams(teamId, weaponDefId, cKeys);
	sAICallback->Clb_WeaponDef_0MAP1VALS0getCustomParams(teamId, weaponDefId, cValues);
	int i;
	for (i=0; i < size; ++i) {
		weaponDef->customParams[cKeys[i]] = cValues[i];
	}
	free(cKeys);
	free(cValues);
}
	if (weaponDefs[weaponDefId] != NULL) {
		delete weaponDefs[weaponDefId];
	}
		weaponDefs[weaponDefId] = weaponDef;
		weaponDefFrames[weaponDefId] = currentFrame;
	}

	return weaponDefs[weaponDefId];
}

const float3* CAIAICallback::GetStartPos() {
	return new float3(sAICallback->Clb_Map_getStartPos(teamId));
}







void CAIAICallback::SendTextMsg(const char* text, int zone) {
	SSendTextMessageCommand cmd = {text, zone};
	sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &cmd);
}

void CAIAICallback::SetLastMsgPos(float3 pos) {
	SSetLastPosMessageCommand cmd = {pos.toSAIFloat3()}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &cmd);
}

void CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {
	SAddNotificationDrawerCommand cmd = {pos.toSAIFloat3(), color.toSAIFloat3(), alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &cmd);
}

bool CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {

	SSendResourcesCommand cmd = {mAmount, eAmount, receivingTeam};
	sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &cmd);
	return cmd.ret_isExecuted;
}

int CAIAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeam) {

	int* arr_unitIds = (int*) calloc(unitIds.size(), sizeof(int));
	for (size_t i=0; i < unitIds.size(); ++i) {
		arr_unitIds[i] = unitIds[i];
	}
	SSendUnitsCommand cmd = {arr_unitIds, unitIds.size(), receivingTeam};
	sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &cmd);
	free(arr_unitIds);
	return cmd.ret_sentUnits;
}

void* CAIAICallback::CreateSharedMemArea(char* name, int size) {

	//SCreateSharedMemAreaCommand cmd = {name, size};
	//sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_CREATE, &cmd);
	//return cmd.ret_sharedMemArea;
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
	return NULL;
}

void CAIAICallback::ReleasedSharedMemArea(char* name) {

	//SReleaseSharedMemAreaCommand cmd = {name};
	//sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_RELEASE, &cmd);
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
}

int CAIAICallback::CreateGroup() {

	SCreateGroupCommand cmd = {};
	sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &cmd);
	return cmd.ret_groupId;
}

void CAIAICallback::EraseGroup(int groupId) {
	SEraseGroupCommand cmd = {groupId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &cmd);
}

bool CAIAICallback::AddUnitToGroup(int unitId, int groupId) {
		SAddUnitToGroupCommand cmd = {unitId, groupId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ADD_UNIT, &cmd); return cmd.ret_isExecuted;
}

bool CAIAICallback::RemoveUnitFromGroup(int unitId) {
		SRemoveUnitFromGroupCommand cmd = {unitId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_REMOVE_UNIT, &cmd); return cmd.ret_isExecuted;
}

int CAIAICallback::GiveGroupOrder(int groupId, Command* c) {
	return this->Internal_GiveOrder(-1, groupId, c);
}

int CAIAICallback::GiveOrder(int unitId, Command* c) {
	return this->Internal_GiveOrder(unitId, -1, c);
}

int CAIAICallback::Internal_GiveOrder(int unitId, int groupId, Command* c) {

	int sCommandId;
	void* sCommandData = mallocSUnitCommand(unitId, groupId, c, &sCommandId);

	int ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, sCommandId, sCommandData);

	freeSUnitCommand(sCommandData, sCommandId);

	return ret;
}

int CAIAICallback::InitPath(float3 start, float3 end, int pathType) {
		SInitPathCommand cmd = {start.toSAIFloat3(), end.toSAIFloat3(), pathType}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &cmd); return cmd.ret_pathId;
}

float3 CAIAICallback::GetNextWaypoint(int pathId) {
		SGetNextWaypointPathCommand cmd = {pathId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &cmd); return float3(cmd.ret_nextWaypoint);
}

float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType) {
		SGetApproximateLengthPathCommand cmd = {start.toSAIFloat3(), end.toSAIFloat3(), pathType}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &cmd); return cmd.ret_approximatePathLength;
}

void CAIAICallback::FreePath(int pathId) {
	SFreePathCommand cmd = {pathId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &cmd);
}

void CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SStartPathDrawerCommand cmd = {pos.toSAIFloat3(), col3, alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &cmd);
}

void CAIAICallback::LineDrawerFinishPath() {
	SFinishPathDrawerCommand cmd = {}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &cmd);
}

void CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SDrawLinePathDrawerCommand cmd = {endPos.toSAIFloat3(), col3, alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &cmd);
}

void CAIAICallback::LineDrawerDrawLineAndIcon(int cmdId, const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SDrawLineAndIconPathDrawerCommand cmd = {cmdId, endPos.toSAIFloat3(), col3, alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &cmd);
}

void CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdId) {
	SDrawIconAtLastPosPathDrawerCommand cmd = {cmdId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &cmd);
}

void CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {
	SAIFloat3 col3 = {color[0], color[1], color[2]};
	float alpha = color[3];
	SBreakPathDrawerCommand cmd = {endPos.toSAIFloat3(), col3, alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &cmd);
}

void CAIAICallback::LineDrawerRestart() {
	SRestartPathDrawerCommand cmd = {false}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

void CAIAICallback::LineDrawerRestartSameColor() {
	SRestartPathDrawerCommand cmd = {true}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

int CAIAICallback::CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifeTime, int figureGroupId) {
		SCreateSplineFigureDrawerCommand cmd = {pos1.toSAIFloat3(), pos2.toSAIFloat3(), pos3.toSAIFloat3(), pos4.toSAIFloat3(), width, arrow, lifeTime, figureGroupId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &cmd); return cmd.ret_newFigureGroupId;
}

int CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) {
		SCreateLineFigureDrawerCommand cmd = {pos1.toSAIFloat3(), pos2.toSAIFloat3(), width, arrow, lifeTime, figureGroupId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &cmd); return cmd.ret_newFigureGroupId;
}

void CAIAICallback::SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) {
	SAIFloat3 col3 = {red, green, blue};
	SSetColorFigureDrawerCommand cmd = {figureGroupId, col3, alpha}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &cmd);
}

void CAIAICallback::DeleteFigureGroup(int figureGroupId) {
	SDeleteFigureDrawerCommand cmd = {figureGroupId}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &cmd);
}

void CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifeTime, int unitTeamId, bool transparent, bool drawBorder, int facing) {
	SDrawUnitDrawerCommand cmd = {sAICallback->Clb_0MULTI1FETCH3UnitDefByName0UnitDef(teamId, name), pos.toSAIFloat3(), rotation, lifeTime, unitTeamId, transparent, drawBorder, facing}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &cmd);
}

int CAIAICallback::HandleCommand(int commandId, void* data) {
	int ret = -99;

	switch (commandId) {
		case AIHCQuerySubVersionId: {
//			SQuerySubVersionCommand cmd;
//			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicId, &cmd);
			ret = sAICallback->Clb_Game_getAiInterfaceVersion(teamId);
			break;
		}
		case AIHCAddMapPointId: {
			AIHCAddMapPoint* myData = (AIHCAddMapPoint*) data;
			SAddPointDrawCommand cmd = {myData->pos.toSAIFloat3(), myData->label};
			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_ADD, &cmd);
			break;
		}
		case AIHCAddMapLineId: {
			AIHCAddMapLine* myData = (AIHCAddMapLine*) data;
			SAddLineDrawCommand cmd = {myData->posfrom.toSAIFloat3(), myData->posto.toSAIFloat3()};
			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_LINE_ADD, &cmd);
			break;
		}
		case AIHCRemoveMapPointId: {
			AIHCRemoveMapPoint* myData = (AIHCRemoveMapPoint*) data;
			SRemovePointDrawCommand cmd = {myData->pos.toSAIFloat3()};
			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_REMOVE, &cmd);
			break;
		}
		case AIHCSendStartPosId: {
			AIHCSendStartPos* myData = (AIHCSendStartPos*) data;
			SSendStartPosCommand cmd = {myData->ready, myData->pos.toSAIFloat3()};
			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_START_POS, &cmd);
			break;
		}

		case AIHCTraceRayId: {
			AIHCTraceRay* cppCmdData = (AIHCTraceRay*) data;
			STraceRayCommand cCmdData = {
				cppCmdData->rayPos.toSAIFloat3(),
				cppCmdData->rayDir.toSAIFloat3(),
				cppCmdData->rayLen,
				cppCmdData->srcUID,
				cppCmdData->hitUID,
				cppCmdData->flags
			};

			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY, &cCmdData);

			cppCmdData->rayLen = cCmdData.rayLen;
			cppCmdData->hitUID = cCmdData.hitUID;
			break;
		}

		case AIHCPauseId: {
			AIHCPause* cppCmdData = (AIHCPause*) data;
			SPauseCommand cCmdData = {
				cppCmdData->enable,
				cppCmdData->reason
			};
			ret = sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PAUSE, &cCmdData);
			break;
		}
	}

	return ret;
}

bool CAIAICallback::ReadFile(const char* filename, void* buffer, int bufferLen) {
//		SReadFileCommand cmd = {name, buffer, bufferLen}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_READ_FILE, &cmd); return cmd.ret_isExecuted;
	return sAICallback->Clb_File_getContent(teamId, filename, buffer, bufferLen);
}

const char* CAIAICallback::CallLuaRules(const char* data, int inSize, int* outSize) {
		SCallLuaRulesCommand cmd = {data, inSize, outSize}; sAICallback->Clb_Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_RULES, &cmd); return cmd.ret_outData;
}


