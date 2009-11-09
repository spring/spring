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
		resIndMetal = sAICallback->getResourceByName(teamId, "Metal");
	}

	return resIndMetal;
}
static inline int getResourceId_Energy(const SSkirmishAICallback* sAICallback, int teamId) {

	if (resIndEnergy == -1) {
		resIndEnergy = sAICallback->getResourceByName(teamId, "Energy");
	}

	return resIndEnergy;
}

static inline void copyShortToUCharArray(const short* src, unsigned char* const dst, const size_t size) {

	int i;
	for (i = 0; i < size; ++i) {
		dst[i] = (unsigned char) src[i];
	}
}
static inline void copyIntToUShortArray(const int* src, unsigned short* const dst, const size_t size) {

	int i;
	for (i = 0; i < size; ++i) {
		dst[i] = (unsigned short) src[i];
	}
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

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->Map_isPosInCamera(teamId, pos_param, radius);
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

	int numCmds = sAICallback->Group_getSupportedCommands(teamId, groupId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Group_SupportedCommand_getId(teamId, groupId, c);
		commandDescription.name = sAICallback->Group_SupportedCommand_getName(teamId, groupId, c);
		commandDescription.tooltip = sAICallback->Group_SupportedCommand_getToolTip(teamId, groupId, c);
		commandDescription.showUnique = sAICallback->Group_SupportedCommand_isShowUnique(teamId, groupId, c);
		commandDescription.disabled = sAICallback->Group_SupportedCommand_isDisabled(teamId, groupId, c);

		int numParams = sAICallback->Group_SupportedCommand_getParams(teamId, groupId, c, NULL, 0);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Group_SupportedCommand_getParams(teamId, groupId, c, params, numParams);
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

	int numCmds = sAICallback->Unit_getSupportedCommands(teamId, unitId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Unit_SupportedCommand_getId(teamId, unitId, c);
		commandDescription.name = sAICallback->Unit_SupportedCommand_getName(teamId, unitId, c);
		commandDescription.tooltip = sAICallback->Unit_SupportedCommand_getToolTip(teamId, unitId, c);
		commandDescription.showUnique = sAICallback->Unit_SupportedCommand_isShowUnique(teamId, unitId, c);
		commandDescription.disabled = sAICallback->Unit_SupportedCommand_isDisabled(teamId, unitId, c);

		int numParams = sAICallback->Unit_SupportedCommand_getParams(teamId, unitId, c, NULL, 0);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Unit_SupportedCommand_getParams(teamId, unitId, c, params, numParams);
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

	const int numCmds = sAICallback->Unit_getCurrentCommands(teamId, unitId);
	const int type = sAICallback->Unit_CurrentCommand_getType(teamId, unitId);

	CCommandQueue* cc = new CCommandQueue();
	cc->queueType = (CCommandQueue::QueueType) type;
	for (int c=0; c < numCmds; c++) {
		Command command;
		command.id = sAICallback->Unit_CurrentCommand_getId(teamId, unitId, c);
		command.options = sAICallback->Unit_CurrentCommand_getOptions(teamId, unitId, c);
		command.tag = sAICallback->Unit_CurrentCommand_getTag(teamId, unitId, c);
		command.timeOut = sAICallback->Unit_CurrentCommand_getTimeOut(teamId, unitId, c);

		int numParams = sAICallback->Unit_CurrentCommand_getParams(teamId, unitId, c, NULL, 0);
		float* params = (float*) calloc(numParams, sizeof(float));
		numParams = sAICallback->Unit_CurrentCommand_getParams(teamId, unitId, c, params, numParams);
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
	int unitDefId = sAICallback->Unit_getDef(teamId, unitId);
	return this->GetUnitDefById(unitDefId);
}

float3 CAIAICallback::GetUnitPos(int unitId) {

	float pos_cache[3];
	sAICallback->Unit_getPos(teamId, unitId, pos_cache);
	return pos_cache;
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

	static int m = getResourceId_Metal(sAICallback, teamId);
	static int e = getResourceId_Energy(sAICallback, teamId);
	resourceInfo->energyMake = sAICallback->Unit_getResourceMake(teamId, unitId, e);
	if (resourceInfo->energyMake < 0) { return false; }
	resourceInfo->energyUse = sAICallback->Unit_getResourceUse(teamId, unitId, e);
	resourceInfo->metalMake = sAICallback->Unit_getResourceMake(teamId, unitId, m);
	resourceInfo->metalUse = sAICallback->Unit_getResourceUse(teamId, unitId, m);
	return true;
}

const UnitDef* CAIAICallback::GetUnitDef(const char* unitName) {
	int unitDefId = sAICallback->getUnitDefByName(teamId, unitName);
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

		float pos_cache[3];

		UnitDef* unitDef = new UnitDef();
		unitDef->valid = sAICallback->UnitDef_isValid(teamId, unitDefId);
		unitDef->name = sAICallback->UnitDef_getName(teamId, unitDefId);
		unitDef->humanName = sAICallback->UnitDef_getHumanName(teamId, unitDefId);
		unitDef->filename = sAICallback->UnitDef_getFileName(teamId, unitDefId);
		//unitDef->id = sAICallback->UnitDef_getId(teamId, unitDefId);
		unitDef->id = unitDefId;
		unitDef->aihint = sAICallback->UnitDef_getAiHint(teamId, unitDefId);
		unitDef->cobID = sAICallback->UnitDef_getCobId(teamId, unitDefId);
		unitDef->techLevel = sAICallback->UnitDef_getTechLevel(teamId, unitDefId);
		unitDef->gaia = sAICallback->UnitDef_getGaia(teamId, unitDefId);
		unitDef->metalUpkeep = sAICallback->UnitDef_getUpkeep(teamId, unitDefId, m);
		unitDef->energyUpkeep = sAICallback->UnitDef_getUpkeep(teamId, unitDefId, e);
		unitDef->metalMake = sAICallback->UnitDef_getResourceMake(teamId, unitDefId, m);
		unitDef->makesMetal = sAICallback->UnitDef_getMakesResource(teamId, unitDefId, m);
		unitDef->energyMake = sAICallback->UnitDef_getResourceMake(teamId, unitDefId, e);
		unitDef->metalCost = sAICallback->UnitDef_getCost(teamId, unitDefId, m);
		unitDef->energyCost = sAICallback->UnitDef_getCost(teamId, unitDefId, e);
		unitDef->buildTime = sAICallback->UnitDef_getBuildTime(teamId, unitDefId);
		unitDef->extractsMetal = sAICallback->UnitDef_getExtractsResource(teamId, unitDefId, m);
		unitDef->extractRange = sAICallback->UnitDef_getResourceExtractorRange(teamId, unitDefId, m);
		unitDef->windGenerator = sAICallback->UnitDef_getWindResourceGenerator(teamId, unitDefId, e);
		unitDef->tidalGenerator = sAICallback->UnitDef_getTidalResourceGenerator(teamId, unitDefId, e);
		unitDef->metalStorage = sAICallback->UnitDef_getStorage(teamId, unitDefId, m);
		unitDef->energyStorage = sAICallback->UnitDef_getStorage(teamId, unitDefId, e);
		unitDef->autoHeal = sAICallback->UnitDef_getAutoHeal(teamId, unitDefId);
		unitDef->idleAutoHeal = sAICallback->UnitDef_getIdleAutoHeal(teamId, unitDefId);
		unitDef->idleTime = sAICallback->UnitDef_getIdleTime(teamId, unitDefId);
		unitDef->power = sAICallback->UnitDef_getPower(teamId, unitDefId);
		unitDef->health = sAICallback->UnitDef_getHealth(teamId, unitDefId);
		unitDef->category = sAICallback->UnitDef_getCategory(teamId, unitDefId);
		unitDef->speed = sAICallback->UnitDef_getSpeed(teamId, unitDefId);
		unitDef->turnRate = sAICallback->UnitDef_getTurnRate(teamId, unitDefId);
		unitDef->turnInPlace = sAICallback->UnitDef_isTurnInPlace(teamId, unitDefId);
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
		unitDef->flankingBonusMode = sAICallback->UnitDef_FlankingBonus_getMode(teamId, unitDefId);
		sAICallback->UnitDef_FlankingBonus_getDir(teamId, unitDefId, pos_cache);
		unitDef->flankingBonusDir = pos_cache;
		unitDef->flankingBonusMax = sAICallback->UnitDef_FlankingBonus_getMax(teamId, unitDefId);
		unitDef->flankingBonusMin = sAICallback->UnitDef_FlankingBonus_getMin(teamId, unitDefId);
		unitDef->flankingBonusMobilityAdd = sAICallback->UnitDef_FlankingBonus_getMobilityAdd(teamId, unitDefId);
		unitDef->collisionVolumeTypeStr = sAICallback->UnitDef_CollisionVolume_getType(teamId, unitDefId);
		sAICallback->UnitDef_CollisionVolume_getScales(teamId, unitDefId, pos_cache);
		unitDef->collisionVolumeScales = pos_cache;
		sAICallback->UnitDef_CollisionVolume_getOffsets(teamId, unitDefId, pos_cache);
		unitDef->collisionVolumeOffsets = pos_cache;
		unitDef->collisionVolumeTest = sAICallback->UnitDef_CollisionVolume_getTest(teamId, unitDefId);
		unitDef->maxWeaponRange = sAICallback->UnitDef_getMaxWeaponRange(teamId, unitDefId);
		unitDef->type = sAICallback->UnitDef_getType(teamId, unitDefId);
		unitDef->tooltip = sAICallback->UnitDef_getTooltip(teamId, unitDefId);
		unitDef->wreckName = sAICallback->UnitDef_getWreckName(teamId, unitDefId);
		unitDef->deathExplosion = sAICallback->UnitDef_getDeathExplosion(teamId, unitDefId);
		unitDef->selfDExplosion = sAICallback->UnitDef_getSelfDExplosion(teamId, unitDefId);
		unitDef->TEDClassString = sAICallback->UnitDef_getTedClassString(teamId, unitDefId);
		unitDef->categoryString = sAICallback->UnitDef_getCategoryString(teamId, unitDefId);
		unitDef->canSelfD = sAICallback->UnitDef_isAbleToSelfD(teamId, unitDefId);
		unitDef->selfDCountdown = sAICallback->UnitDef_getSelfDCountdown(teamId, unitDefId);
		unitDef->canSubmerge = sAICallback->UnitDef_isAbleToSubmerge(teamId, unitDefId);
		unitDef->canfly = sAICallback->UnitDef_isAbleToFly(teamId, unitDefId);
		unitDef->canmove = sAICallback->UnitDef_isAbleToMove(teamId, unitDefId);
		unitDef->canhover = sAICallback->UnitDef_isAbleToHover(teamId, unitDefId);
		unitDef->floater = sAICallback->UnitDef_isFloater(teamId, unitDefId);
		unitDef->builder = sAICallback->UnitDef_isBuilder(teamId, unitDefId);
		unitDef->activateWhenBuilt = sAICallback->UnitDef_isActivateWhenBuilt(teamId, unitDefId);
		unitDef->onoffable = sAICallback->UnitDef_isOnOffable(teamId, unitDefId);
		unitDef->fullHealthFactory = sAICallback->UnitDef_isFullHealthFactory(teamId, unitDefId);
		unitDef->factoryHeadingTakeoff = sAICallback->UnitDef_isFactoryHeadingTakeoff(teamId, unitDefId);
		unitDef->reclaimable = sAICallback->UnitDef_isReclaimable(teamId, unitDefId);
		unitDef->capturable = sAICallback->UnitDef_isCapturable(teamId, unitDefId);
		unitDef->canRestore = sAICallback->UnitDef_isAbleToRestore(teamId, unitDefId);
		unitDef->canRepair = sAICallback->UnitDef_isAbleToRepair(teamId, unitDefId);
		unitDef->canSelfRepair = sAICallback->UnitDef_isAbleToSelfRepair(teamId, unitDefId);
		unitDef->canReclaim = sAICallback->UnitDef_isAbleToReclaim(teamId, unitDefId);
		unitDef->canAttack = sAICallback->UnitDef_isAbleToAttack(teamId, unitDefId);
		unitDef->canPatrol = sAICallback->UnitDef_isAbleToPatrol(teamId, unitDefId);
		unitDef->canFight = sAICallback->UnitDef_isAbleToFight(teamId, unitDefId);
		unitDef->canGuard = sAICallback->UnitDef_isAbleToGuard(teamId, unitDefId);
		unitDef->canAssist = sAICallback->UnitDef_isAbleToAssist(teamId, unitDefId);
		unitDef->canBeAssisted = sAICallback->UnitDef_isAssistable(teamId, unitDefId);
		unitDef->canRepeat = sAICallback->UnitDef_isAbleToRepeat(teamId, unitDefId);
		unitDef->canFireControl = sAICallback->UnitDef_isAbleToFireControl(teamId, unitDefId);
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
		unitDef->canCrash = sAICallback->UnitDef_isAbleToCrash(teamId, unitDefId);
		unitDef->hoverAttack = sAICallback->UnitDef_isHoverAttack(teamId, unitDefId);
		unitDef->airStrafe = sAICallback->UnitDef_isAirStrafe(teamId, unitDefId);
		unitDef->dlHoverFactor = sAICallback->UnitDef_getDlHoverFactor(teamId, unitDefId);
		unitDef->maxAcc = sAICallback->UnitDef_getMaxAcceleration(teamId, unitDefId);
		unitDef->maxDec = sAICallback->UnitDef_getMaxDeceleration(teamId, unitDefId);
		unitDef->maxAileron = sAICallback->UnitDef_getMaxAileron(teamId, unitDefId);
		unitDef->maxElevator = sAICallback->UnitDef_getMaxElevator(teamId, unitDefId);
		unitDef->maxRudder = sAICallback->UnitDef_getMaxRudder(teamId, unitDefId);
		{
			static const size_t facings = 4;
			const int yardMap_size = sAICallback->UnitDef_getYardMap(teamId, unitDefId, 0, NULL, 0);
			short tmpYardMaps[facings][yardMap_size];
			int ym;
			for (ym = 0 ; ym < facings; ++ym) {
				sAICallback->UnitDef_getYardMap(teamId, unitDefId, ym, tmpYardMaps[ym], yardMap_size);
			}

			int i;
			for (ym = 0 ; ym < facings; ++ym) {
				unitDef->yardmaps[ym] = new unsigned char[yardMap_size];
				for (i = 0; i < yardMap_size; ++i) {
					unitDef->yardmaps[ym][i] = (const char) tmpYardMaps[ym][i];
				}
			}
		}
		unitDef->xsize = sAICallback->UnitDef_getXSize(teamId, unitDefId);
		unitDef->zsize = sAICallback->UnitDef_getZSize(teamId, unitDefId);
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
		unitDef->cantBeTransported = sAICallback->UnitDef_isNotTransportable(teamId, unitDefId);
		unitDef->transportByEnemy = sAICallback->UnitDef_isTransportByEnemy(teamId, unitDefId);
		unitDef->transportUnloadMethod = sAICallback->UnitDef_getTransportUnloadMethod(teamId, unitDefId);
		unitDef->fallSpeed = sAICallback->UnitDef_getFallSpeed(teamId, unitDefId);
		unitDef->unitFallSpeed = sAICallback->UnitDef_getUnitFallSpeed(teamId, unitDefId);
		unitDef->canCloak = sAICallback->UnitDef_isAbleToCloak(teamId, unitDefId);
		unitDef->startCloaked = sAICallback->UnitDef_isStartCloaked(teamId, unitDefId);
		unitDef->cloakCost = sAICallback->UnitDef_getCloakCost(teamId, unitDefId);
		unitDef->cloakCostMoving = sAICallback->UnitDef_getCloakCostMoving(teamId, unitDefId);
		unitDef->decloakDistance = sAICallback->UnitDef_getDecloakDistance(teamId, unitDefId);
		unitDef->decloakSpherical = sAICallback->UnitDef_isDecloakSpherical(teamId, unitDefId);
		unitDef->decloakOnFire = sAICallback->UnitDef_isDecloakOnFire(teamId, unitDefId);
		unitDef->canKamikaze = sAICallback->UnitDef_isAbleToKamikaze(teamId, unitDefId);
		unitDef->kamikazeDist = sAICallback->UnitDef_getKamikazeDist(teamId, unitDefId);
		unitDef->targfac = sAICallback->UnitDef_isTargetingFacility(teamId, unitDefId);
		unitDef->canDGun = sAICallback->UnitDef_isAbleToDGun(teamId, unitDefId);
		unitDef->needGeo = sAICallback->UnitDef_isNeedGeo(teamId, unitDefId);
		unitDef->isFeature = sAICallback->UnitDef_isFeature(teamId, unitDefId);
		unitDef->hideDamage = sAICallback->UnitDef_isHideDamage(teamId, unitDefId);
		unitDef->isCommander = sAICallback->UnitDef_isCommander(teamId, unitDefId);
		unitDef->showPlayerName = sAICallback->UnitDef_isShowPlayerName(teamId, unitDefId);
		unitDef->canResurrect = sAICallback->UnitDef_isAbleToResurrect(teamId, unitDefId);
		unitDef->canCapture = sAICallback->UnitDef_isAbleToCapture(teamId, unitDefId);
		unitDef->highTrajectoryType = sAICallback->UnitDef_getHighTrajectoryType(teamId, unitDefId);
		unitDef->noChaseCategory = sAICallback->UnitDef_getNoChaseCategory(teamId, unitDefId);
		unitDef->leaveTracks = sAICallback->UnitDef_isLeaveTracks(teamId, unitDefId);
		unitDef->trackWidth = sAICallback->UnitDef_getTrackWidth(teamId, unitDefId);
		unitDef->trackOffset = sAICallback->UnitDef_getTrackOffset(teamId, unitDefId);
		unitDef->trackStrength = sAICallback->UnitDef_getTrackStrength(teamId, unitDefId);
		unitDef->trackStretch = sAICallback->UnitDef_getTrackStretch(teamId, unitDefId);
		unitDef->trackType = sAICallback->UnitDef_getTrackType(teamId, unitDefId);
		unitDef->canDropFlare = sAICallback->UnitDef_isAbleToDropFlare(teamId, unitDefId);
		unitDef->flareReloadTime = sAICallback->UnitDef_getFlareReloadTime(teamId, unitDefId);
		unitDef->flareEfficiency = sAICallback->UnitDef_getFlareEfficiency(teamId, unitDefId);
		unitDef->flareDelay = sAICallback->UnitDef_getFlareDelay(teamId, unitDefId);
		sAICallback->UnitDef_getFlareDropVector(teamId, unitDefId, pos_cache);
		unitDef->flareDropVector = pos_cache;
		unitDef->flareTime = sAICallback->UnitDef_getFlareTime(teamId, unitDefId);
		unitDef->flareSalvoSize = sAICallback->UnitDef_getFlareSalvoSize(teamId, unitDefId);
		unitDef->flareSalvoDelay = sAICallback->UnitDef_getFlareSalvoDelay(teamId, unitDefId);
		//unitDef->smoothAnim = sAICallback->UnitDef_isSmoothAnim(teamId, unitDefId);
		unitDef->smoothAnim = false;
		unitDef->isMetalMaker = sAICallback->UnitDef_isResourceMaker(teamId, unitDefId, m);
		unitDef->canLoopbackAttack = sAICallback->UnitDef_isAbleToLoopbackAttack(teamId, unitDefId);
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
		unitDef->shieldWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getShieldDef(teamId, unitDefId));
		unitDef->stockpileWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getStockpileDef(teamId, unitDefId));

		{
			int numBo = sAICallback->UnitDef_getBuildOptions(teamId, unitDefId, NULL, 0);
			int* bo = new int[numBo];
			numBo = sAICallback->UnitDef_getBuildOptions(teamId, unitDefId, bo, numBo);
			for (int b=0; b < numBo; b++) {
				unitDef->buildOptions[b] = sAICallback->UnitDef_getName(teamId, bo[b]);
			}
			delete [] bo;
		}
		{
			const int size = sAICallback->UnitDef_getCustomParams(teamId, unitDefId, NULL, NULL);
			const char** cKeys = (const char**) calloc(size, sizeof(char*));
			const char** cValues = (const char**) calloc(size, sizeof(char*));
			sAICallback->UnitDef_getCustomParams(teamId, unitDefId, cKeys, cValues);
			int i;
			for (i = 0; i < size; ++i) {
				unitDef->customParams[cKeys[i]] = cValues[i];
			}
			free(cKeys);
			free(cValues);
		}

		if (sAICallback->UnitDef_isMoveDataAvailable(teamId, unitDefId)) {
			unitDef->movedata = new MoveData(NULL);
			unitDef->movedata->maxAcceleration = sAICallback->UnitDef_MoveData_getMaxAcceleration(teamId, unitDefId);
			unitDef->movedata->maxBreaking = sAICallback->UnitDef_MoveData_getMaxBreaking(teamId, unitDefId);
			unitDef->movedata->maxSpeed = sAICallback->UnitDef_MoveData_getMaxSpeed(teamId, unitDefId);
			unitDef->movedata->maxTurnRate = sAICallback->UnitDef_MoveData_getMaxTurnRate(teamId, unitDefId);

			unitDef->movedata->size = sAICallback->UnitDef_MoveData_getSize(teamId, unitDefId);
			unitDef->movedata->depth = sAICallback->UnitDef_MoveData_getDepth(teamId, unitDefId);
			unitDef->movedata->maxSlope = sAICallback->UnitDef_MoveData_getMaxSlope(teamId, unitDefId);
			unitDef->movedata->slopeMod = sAICallback->UnitDef_MoveData_getSlopeMod(teamId, unitDefId);
			unitDef->movedata->depthMod = sAICallback->UnitDef_MoveData_getDepthMod(teamId, unitDefId);
			unitDef->movedata->pathType = sAICallback->UnitDef_MoveData_getPathType(teamId, unitDefId);
			unitDef->movedata->moveMath = 0;
			unitDef->movedata->crushStrength = sAICallback->UnitDef_MoveData_getCrushStrength(teamId, unitDefId);
			unitDef->movedata->moveType = (enum MoveData::MoveType) sAICallback->UnitDef_MoveData_getMoveType(teamId, unitDefId);
			unitDef->movedata->moveFamily = (enum MoveData::MoveFamily) sAICallback->UnitDef_MoveData_getMoveFamily(teamId, unitDefId);
			unitDef->movedata->terrainClass = (enum MoveData::TerrainClass) sAICallback->UnitDef_MoveData_getTerrainClass(teamId, unitDefId);

			unitDef->movedata->followGround = sAICallback->UnitDef_MoveData_getFollowGround(teamId, unitDefId);
			unitDef->movedata->subMarine = sAICallback->UnitDef_MoveData_isSubMarine(teamId, unitDefId);
			unitDef->movedata->name = std::string(sAICallback->UnitDef_MoveData_getName(teamId, unitDefId));
		} else {
			unitDef->movedata = NULL;
		}

		const int numWeapons = sAICallback->UnitDef_getWeaponMounts(teamId, unitDefId);
		for (int w = 0; w < numWeapons; ++w) {
			unitDef->weapons.push_back(UnitDef::UnitDefWeapon());
			unitDef->weapons[w].name = sAICallback->UnitDef_WeaponMount_getName(teamId, unitDefId, w);
			int weaponDefId = sAICallback->UnitDef_WeaponMount_getWeaponDef(teamId, unitDefId, w);
			unitDef->weapons[w].def = this->GetWeaponDefById(weaponDefId);
			unitDef->weapons[w].slavedTo = sAICallback->UnitDef_WeaponMount_getSlavedTo(teamId, unitDefId, w);
			sAICallback->UnitDef_WeaponMount_getMainDir(teamId, unitDefId, w, pos_cache);
			unitDef->weapons[w].mainDir = pos_cache;
			unitDef->weapons[w].maxAngleDif = sAICallback->UnitDef_WeaponMount_getMaxAngleDif(teamId, unitDefId, w);
			unitDef->weapons[w].fuelUsage = sAICallback->UnitDef_WeaponMount_getFuelUsage(teamId, unitDefId, w);
			unitDef->weapons[w].badTargetCat = sAICallback->UnitDef_WeaponMount_getBadTargetCategory(teamId, unitDefId, w);
			unitDef->weapons[w].onlyTargetCat = sAICallback->UnitDef_WeaponMount_getOnlyTargetCategory(teamId, unitDefId, w);
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
	return sAICallback->getEnemyUnits(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getEnemyUnitsIn(teamId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max) {
	return sAICallback->getEnemyUnitsInRadarAndLos(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getFriendlyUnits(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getFriendlyUnitsIn(teamId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getNeutralUnits(teamId, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getNeutralUnitsIn(teamId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetMapWidth() {
	return sAICallback->Map_getWidth(teamId);
}

int CAIAICallback::GetMapHeight() {
	return sAICallback->Map_getHeight(teamId);
}

const float* CAIAICallback::GetHeightMap() {

	static float* heightMap = NULL;

	if (heightMap == NULL) {
		const int size = sAICallback->Map_getHeightMap(teamId, NULL, 0);
		heightMap = new float[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Map_getHeightMap(teamId, heightMap, size);
	}

	return heightMap;
}

const float* CAIAICallback::GetCornersHeightMap() {

	static float* cornersHeightMap = NULL;

	if (cornersHeightMap == NULL) {
		const int size = sAICallback->Map_getCornersHeightMap(teamId, NULL, 0);
		cornersHeightMap = new float[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Map_getCornersHeightMap(teamId, cornersHeightMap, size);
	}

	return cornersHeightMap;
}

float CAIAICallback::GetMinHeight() {
	return sAICallback->Map_getMinHeight(teamId);
}

float CAIAICallback::GetMaxHeight() {
	return sAICallback->Map_getMaxHeight(teamId);
}

const float* CAIAICallback::GetSlopeMap() {

	static float* slopeMap = NULL;

	if (slopeMap == NULL) {
		const int size = sAICallback->Map_getSlopeMap(teamId, NULL, 0);
		slopeMap = new float[size]; // NOTE: memory leack, but will be used till end of the game anyway
		sAICallback->Map_getSlopeMap(teamId, slopeMap, size);
	}

	return slopeMap;
}

const unsigned short* CAIAICallback::GetLosMap() {

	static unsigned short* losMap = NULL;

	if (losMap == NULL) {
		const int size = sAICallback->Map_getLosMap(teamId, NULL, 0);
		int tmpLosMap[size];
		sAICallback->Map_getLosMap(teamId, tmpLosMap, size);
		losMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		copyIntToUShortArray(tmpLosMap, losMap, size);
	}

	return losMap;
}

int CAIAICallback::GetLosMapResolution() {

	int fullSize = GetMapWidth() * GetMapHeight();
	int losSize = sAICallback->Map_getLosMap(teamId, NULL, 0);

	return fullSize / losSize;
}

const unsigned short* CAIAICallback::GetRadarMap() {

	static unsigned short* radarMap = NULL;

	if (radarMap == NULL) {
		const int size = sAICallback->Map_getRadarMap(teamId, NULL, 0);
		int tmpRadarMap[size];
		sAICallback->Map_getRadarMap(teamId, tmpRadarMap, size);
		radarMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		copyIntToUShortArray(tmpRadarMap, radarMap, size);
	}

	return radarMap;
}

const unsigned short* CAIAICallback::GetJammerMap() {

	static unsigned short* jammerMap = NULL;

	if (jammerMap == NULL) {
		const int size = sAICallback->Map_getJammerMap(teamId, NULL, 0);
		int tmpJammerMap[size];
		sAICallback->Map_getJammerMap(teamId, tmpJammerMap, size);
		jammerMap = new unsigned short[size]; // NOTE: memory leack, but will be used till end of the game anyway
		copyIntToUShortArray(tmpJammerMap, jammerMap, size);
	}

	return jammerMap;
}

const unsigned char* CAIAICallback::GetMetalMap() {

	static unsigned char* metalMap = NULL;
	static const int m = getResourceId_Metal(sAICallback, teamId);

	if (metalMap == NULL) {
		const int size = sAICallback->Map_getResourceMapRaw(teamId, m, NULL, 0);
		short tmpMetalMap[size];
		sAICallback->Map_getResourceMapRaw(teamId, m, tmpMetalMap, size);
		metalMap = new unsigned char[size]; // NOTE: memory leack, but will be used till end of the game anyway
		copyShortToUCharArray(tmpMetalMap, metalMap, size);
	}

	return metalMap;
}

const char* CAIAICallback::GetMapName() {
	return sAICallback->Map_getName(teamId);
}

const char* CAIAICallback::GetModName() {
	return sAICallback->Mod_getFileName(teamId);
}

float CAIAICallback::GetElevation(float x, float z) {
	return sAICallback->Map_getElevationAt(teamId, x, z);
}


float CAIAICallback::GetMaxMetal() const {
	static const int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Map_getMaxResource(teamId, m);
}
float CAIAICallback::GetExtractorRadius() const {
	static const int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Map_getExtractorRadius(teamId, m);
}

float CAIAICallback::GetMinWind() const { return sAICallback->Map_getMinWind(teamId); }
float CAIAICallback::GetMaxWind() const { return sAICallback->Map_getMaxWind(teamId); }
float CAIAICallback::GetCurWind() const { return sAICallback->Map_getCurWind(teamId); }

float CAIAICallback::GetTidalStrength() const  { return sAICallback->Map_getTidalStrength(teamId); }
float CAIAICallback::GetGravity() const { return sAICallback->Map_getGravity(teamId); }


bool CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->Map_isPossibleToBuildAt(teamId, unitDef->id, pos_param, facing);
}

float3 CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing) {

	float pos_param[3];
	pos.copyInto(pos_param);

	float pos_cache[3];
	sAICallback->Map_findClosestBuildSite(teamId, unitDef->id, pos_param, searchRadius, minDist, facing, pos_cache);
	return pos_cache;
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
			*((int*)data) = sAICallback->WeaponDef_getNumDamageTypes(teamId);
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
			float pos_cache[3];
			sAICallback->Gui_Camera_getDirection(teamId, pos_cache);
			*(float3*)data = pos_cache;
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			float pos_cache[3];
			sAICallback->Gui_Camera_getPosition(teamId, pos_cache);
			*(float3*)data = pos_cache;
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			//sAICallback->File_locateForReading(teamId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->DataDirs_locatePath(teamId, absPath, absPath_sizeMax, (const char*) data, false, false, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}case AIVAL_LOCATE_FILE_W:{
			//sAICallback->File_locateForWriting(teamId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->DataDirs_locatePath(teamId, absPath, absPath_sizeMax, (const char*) data, true, true, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = sAICallback->Unit_getLimit(teamId);
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

int CAIAICallback::GetSelectedUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getSelectedUnits(teamId, unitIds, unitIds_max);
}

float3 CAIAICallback::GetMousePos() {

	float pos_cache[3];
	sAICallback->Map_getMousePos(teamId, pos_cache);
	return pos_cache;
}

int CAIAICallback::GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies) {

	const int numPoints = sAICallback->Map_getPoints(teamId, includeAllies);
	float pos_cache[3];
	short color_cache[3];
	for (int p=0; p < numPoints; ++p) {
		sAICallback->Map_Point_getPosition(teamId, p, pos_cache);
		pm[p].pos = pos_cache;
		sAICallback->Map_Point_getColor(teamId, p, color_cache);
		pm[p].color = (unsigned char*) calloc(3, sizeof(unsigned char));
		pm[p].color[0] = (unsigned char) color_cache[0];
		pm[p].color[1] = (unsigned char) color_cache[1];
		pm[p].color[2] = (unsigned char) color_cache[2];
		pm[p].label = sAICallback->Map_Point_getLabel(teamId, p);
	}

	return numPoints;
}

int CAIAICallback::GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies) {

	const int numLines = sAICallback->Map_getLines(teamId, includeAllies);
	float pos_cache[3];
	short color_cache[3];
	for (int l=0; l < numLines; ++l) {
		sAICallback->Map_Line_getFirstPosition(teamId, l, pos_cache);
		lm[l].pos = pos_cache;
		sAICallback->Map_Line_getSecondPosition(teamId, l, pos_cache);
		lm[l].pos2 = pos_cache;
		sAICallback->Map_Line_getColor(teamId, l, color_cache);
		lm[l].color = (unsigned char*) calloc(3, sizeof(unsigned char));
		lm[l].color[0] = (unsigned char) color_cache[0];
		lm[l].color[1] = (unsigned char) color_cache[1];
		lm[l].color[2] = (unsigned char) color_cache[2];
	}

	return numLines;
}

float CAIAICallback::GetMetal() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Economy_getCurrent(teamId, m);
}

float CAIAICallback::GetMetalIncome() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Economy_getIncome(teamId, m);
}

float CAIAICallback::GetMetalUsage() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Economy_getUsage(teamId, m);
}

float CAIAICallback::GetMetalStorage() {
	int m = getResourceId_Metal(sAICallback, teamId);
	return sAICallback->Economy_getStorage(teamId, m);
}

float CAIAICallback::GetEnergy() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Economy_getCurrent(teamId, e);
}

float CAIAICallback::GetEnergyIncome() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Economy_getIncome(teamId, e);
}

float CAIAICallback::GetEnergyUsage() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Economy_getUsage(teamId, e);
}

float CAIAICallback::GetEnergyStorage() {
	int e = getResourceId_Energy(sAICallback, teamId);
	return sAICallback->Economy_getStorage(teamId, e);
}

int CAIAICallback::GetFeatures(int* featureIds, int featureIds_max) {
	return sAICallback->getFeatures(teamId, featureIds, featureIds_max);
}

int CAIAICallback::GetFeatures(int *featureIds, int featureIds_max, const float3& pos, float radius) {

	float aiPos[3];
	pos.copyInto(aiPos);
	return sAICallback->getFeaturesIn(teamId, aiPos, radius, featureIds, featureIds_max);
}

const FeatureDef* CAIAICallback::GetFeatureDef(int featureId) {
	int featureDefId = sAICallback->Feature_getDef(teamId, featureId);
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
		float pos_cache[3];
	FeatureDef* featureDef = new FeatureDef();
featureDef->myName = sAICallback->FeatureDef_getName(teamId, featureDefId);
featureDef->description = sAICallback->FeatureDef_getDescription(teamId, featureDefId);
featureDef->filename = sAICallback->FeatureDef_getFileName(teamId, featureDefId);
//featureDef->id = sAICallback->FeatureDef_getId(teamId, featureDefId);
featureDef->id = featureDefId;
featureDef->metal = sAICallback->FeatureDef_getContainedResource(teamId, featureDefId, m);
featureDef->energy = sAICallback->FeatureDef_getContainedResource(teamId, featureDefId, e);
featureDef->maxHealth = sAICallback->FeatureDef_getMaxHealth(teamId, featureDefId);
featureDef->reclaimTime = sAICallback->FeatureDef_getReclaimTime(teamId, featureDefId);
featureDef->mass = sAICallback->FeatureDef_getMass(teamId, featureDefId);
featureDef->collisionVolumeTypeStr = sAICallback->FeatureDef_CollisionVolume_getType(teamId, featureDefId);
sAICallback->FeatureDef_CollisionVolume_getScales(teamId, featureDefId, pos_cache);
featureDef->collisionVolumeScales = pos_cache;
sAICallback->FeatureDef_CollisionVolume_getOffsets(teamId, featureDefId, pos_cache);
featureDef->collisionVolumeOffsets = pos_cache;
featureDef->collisionVolumeTest = sAICallback->FeatureDef_CollisionVolume_getTest(teamId, featureDefId);
featureDef->upright = sAICallback->FeatureDef_isUpright(teamId, featureDefId);
featureDef->drawType = sAICallback->FeatureDef_getDrawType(teamId, featureDefId);
featureDef->modelname = sAICallback->FeatureDef_getModelName(teamId, featureDefId);
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
featureDef->xsize = sAICallback->FeatureDef_getXSize(teamId, featureDefId);
featureDef->zsize = sAICallback->FeatureDef_getZSize(teamId, featureDefId);
{
	const int size = sAICallback->FeatureDef_getCustomParams(teamId, featureDefId, NULL, NULL);
	featureDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->FeatureDef_getCustomParams(teamId, featureDefId, cKeys, cValues);
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
	return sAICallback->Feature_getHealth(teamId, featureId);
}

float CAIAICallback::GetFeatureReclaimLeft(int featureId) {
	return sAICallback->Feature_getReclaimLeft(teamId, featureId);
}

float3 CAIAICallback::GetFeaturePos(int featureId) {
	
	float pos_cache[3];
	sAICallback->Feature_getPosition(teamId, featureId, pos_cache);
	return pos_cache;
}

int CAIAICallback::GetNumUnitDefs() {
	return sAICallback->getUnitDefs(teamId, NULL, NULL);
}

void CAIAICallback::GetUnitDefList(const UnitDef** list) {

	int size = sAICallback->getUnitDefs(teamId, NULL, NULL);
	int* unitDefIds = new int[size];
	size = sAICallback->getUnitDefs(teamId, unitDefIds, size);
	for (int i=0; i < size; ++i) {
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
	int weaponDefId = sAICallback->getWeaponDefByName(teamId, weaponName);
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
//weaponDef->damages = sAICallback->WeaponDef_getDamages(teamId, weaponDefId);
//{
int numTypes = sAICallback->WeaponDef_Damage_getTypes(teamId, weaponDefId, NULL, 0);
//	logT("GetWeaponDefById 1");
//float* typeDamages = new float[numTypes];
float* typeDamages = (float*) calloc(numTypes, sizeof(float));
numTypes = sAICallback->WeaponDef_Damage_getTypes(teamId, weaponDefId, typeDamages, numTypes);
//	logT("GetWeaponDefById 2");
//for(int i=0; i < numTypes; ++i) {
//	typeDamages[i] = sAICallback->WeaponDef_Damages_getType(teamId, weaponDefId, i);
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
da.paralyzeDamageTime = sAICallback->WeaponDef_Damage_getParalyzeDamageTime(teamId, weaponDefId);
da.impulseFactor = sAICallback->WeaponDef_Damage_getImpulseFactor(teamId, weaponDefId);
da.impulseBoost = sAICallback->WeaponDef_Damage_getImpulseBoost(teamId, weaponDefId);
da.craterMult = sAICallback->WeaponDef_Damage_getCraterMult(teamId, weaponDefId);
da.craterBoost = sAICallback->WeaponDef_Damage_getCraterBoost(teamId, weaponDefId);
//	logT("GetWeaponDefById 4");
//}

	short color_cache[3];
	WeaponDef* weaponDef = new WeaponDef(da);
//	WeaponDef* weaponDef = new WeaponDef();
//	logT("GetWeaponDefById 5");
//	logI("GetWeaponDefById 5 defId: %d", weaponDefId);
weaponDef->name = sAICallback->WeaponDef_getName(teamId, weaponDefId);
weaponDef->type = sAICallback->WeaponDef_getType(teamId, weaponDefId);
weaponDef->description = sAICallback->WeaponDef_getDescription(teamId, weaponDefId);
weaponDef->filename = sAICallback->WeaponDef_getFileName(teamId, weaponDefId);
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
weaponDef->metalcost = sAICallback->WeaponDef_getCost(teamId, weaponDefId, m);
weaponDef->energycost = sAICallback->WeaponDef_getCost(teamId, weaponDefId, e);
weaponDef->supplycost = sAICallback->WeaponDef_getSupplyCost(teamId, weaponDefId);
weaponDef->projectilespershot = sAICallback->WeaponDef_getProjectilesPerShot(teamId, weaponDefId);
//weaponDef->id = sAICallback->WeaponDef_getId(teamId, weaponDefId);
weaponDef->id = weaponDefId;
//weaponDef->tdfId = sAICallback->WeaponDef_getTdfId(teamId, weaponDefId);
weaponDef->tdfId = -1;
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
weaponDef->shieldEnergyUse = sAICallback->WeaponDef_Shield_getResourceUse(teamId, weaponDefId, e);
weaponDef->shieldRadius = sAICallback->WeaponDef_Shield_getRadius(teamId, weaponDefId);
weaponDef->shieldForce = sAICallback->WeaponDef_Shield_getForce(teamId, weaponDefId);
weaponDef->shieldMaxSpeed = sAICallback->WeaponDef_Shield_getMaxSpeed(teamId, weaponDefId);
weaponDef->shieldPower = sAICallback->WeaponDef_Shield_getPower(teamId, weaponDefId);
weaponDef->shieldPowerRegen = sAICallback->WeaponDef_Shield_getPowerRegen(teamId, weaponDefId);
weaponDef->shieldPowerRegenEnergy = sAICallback->WeaponDef_Shield_getPowerRegenResource(teamId, weaponDefId, e);
weaponDef->shieldStartingPower = sAICallback->WeaponDef_Shield_getStartingPower(teamId, weaponDefId);
weaponDef->shieldRechargeDelay = sAICallback->WeaponDef_Shield_getRechargeDelay(teamId, weaponDefId);
sAICallback->WeaponDef_Shield_getGoodColor(teamId, weaponDefId, color_cache);
weaponDef->shieldGoodColor = float3((float)color_cache[0], (float)color_cache[1], (float)color_cache[2]);
sAICallback->WeaponDef_Shield_getBadColor(teamId, weaponDefId, color_cache);
weaponDef->shieldBadColor = float3((float)color_cache[0], (float)color_cache[1], (float)color_cache[2]);
weaponDef->shieldAlpha = sAICallback->WeaponDef_Shield_getAlpha(teamId, weaponDefId);
weaponDef->shieldInterceptType = sAICallback->WeaponDef_Shield_getInterceptType(teamId, weaponDefId);
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
weaponDef->canAttackGround = sAICallback->WeaponDef_isAbleToAttackGround(teamId, weaponDefId);
weaponDef->cameraShake = sAICallback->WeaponDef_getCameraShake(teamId, weaponDefId);
weaponDef->dynDamageExp = sAICallback->WeaponDef_getDynDamageExp(teamId, weaponDefId);
weaponDef->dynDamageMin = sAICallback->WeaponDef_getDynDamageMin(teamId, weaponDefId);
weaponDef->dynDamageRange = sAICallback->WeaponDef_getDynDamageRange(teamId, weaponDefId);
weaponDef->dynDamageInverted = sAICallback->WeaponDef_isDynDamageInverted(teamId, weaponDefId);
{
	const int size = sAICallback->WeaponDef_getCustomParams(teamId, weaponDefId, NULL, NULL);
	weaponDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->WeaponDef_getCustomParams(teamId, weaponDefId, cKeys, cValues);
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

	float pos_cache[3];
	sAICallback->Map_getStartPos(teamId, pos_cache);
	startPos = pos_cache;

	return &startPos;
}







void CAIAICallback::SendTextMsg(const char* text, int zone) {

	SSendTextMessageCommand cmd = {text, zone};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &cmd);
}

void CAIAICallback::SetLastMsgPos(float3 pos) {

	float pos_f3[3];
	pos.copyInto(pos_f3);

	SSetLastPosMessageCommand cmd = {pos_f3};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &cmd);
}

void CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0];
	color_s3[1] = (short) color[1];
	color_s3[2] = (short) color[2];

	SAddNotificationDrawerCommand cmd = {pos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &cmd);
}

bool CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {

	SSendResourcesCommand cmd = {mAmount, eAmount, receivingTeam};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &cmd);
	return cmd.ret_isExecuted;
}

int CAIAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeam) {

	int* arr_unitIds = (int*) calloc(unitIds.size(), sizeof(int));
	for (size_t i=0; i < unitIds.size(); ++i) {
		arr_unitIds[i] = unitIds[i];
	}
	SSendUnitsCommand cmd = {arr_unitIds, unitIds.size(), receivingTeam};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &cmd);
	free(arr_unitIds);
	return cmd.ret_sentUnits;
}

void* CAIAICallback::CreateSharedMemArea(char* name, int size) {

	//SCreateSharedMemAreaCommand cmd = {name, size};
	//sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_CREATE, &cmd);
	//return cmd.ret_sharedMemArea;
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
	return NULL;
}

void CAIAICallback::ReleasedSharedMemArea(char* name) {

	//SReleaseSharedMemAreaCommand cmd = {name};
	//sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_RELEASE, &cmd);
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
}

int CAIAICallback::CreateGroup() {

	SCreateGroupCommand cmd = {};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &cmd);
	return cmd.ret_groupId;
}

void CAIAICallback::EraseGroup(int groupId) {

	SEraseGroupCommand cmd = {groupId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &cmd);
}

bool CAIAICallback::AddUnitToGroup(int unitId, int groupId) {

	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0, groupId};
	const int ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_ADD, &cmd);
	return (ret == 0);
}

bool CAIAICallback::RemoveUnitFromGroup(int unitId) {

	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0};
	const int ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_CLEAR, &cmd);
	return (ret == 0);
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

	int ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, sCommandId, sCommandData);

	freeSUnitCommand(sCommandData, sCommandId);

	return ret;
}

int CAIAICallback::InitPath(float3 start, float3 end, int pathType) {

	float start_f3[3];
	start.copyInto(start_f3);
	float end_f3[3];
	end.copyInto(end_f3);

	SInitPathCommand cmd = {start_f3, end_f3, pathType};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &cmd);
	return cmd.ret_pathId;
}

float3 CAIAICallback::GetNextWaypoint(int pathId) {

	SGetNextWaypointPathCommand cmd = {pathId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &cmd);
	return cmd.ret_nextWaypoint_posF3_out;
}

float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType) {

	float start_f3[3];
	start.copyInto(start_f3);
	float end_f3[3];
	end.copyInto(end_f3);

	SGetApproximateLengthPathCommand cmd = {start_f3, end_f3, pathType};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &cmd); return cmd.ret_approximatePathLength;
}

void CAIAICallback::FreePath(int pathId) {

	SFreePathCommand cmd = {pathId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &cmd);
}

void CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0];
	color_s3[1] = (short) color[1];
	color_s3[2] = (short) color[2];
	const short alpha = (short) color[3];

	SStartPathDrawerCommand cmd = {pos_f3, color_s3, alpha}; sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &cmd);
}

void CAIAICallback::LineDrawerFinishPath() {

	SFinishPathDrawerCommand cmd = {};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &cmd);
}

void CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0];
	color_s3[1] = (short) color[1];
	color_s3[2] = (short) color[2];
	const short alpha = (short) color[3];

	SDrawLinePathDrawerCommand cmd = {endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &cmd);
}

void CAIAICallback::LineDrawerDrawLineAndIcon(int cmdId, const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0];
	color_s3[1] = (short) color[1];
	color_s3[2] = (short) color[2];
	const short alpha = (short) color[3];

	SDrawLineAndIconPathDrawerCommand cmd = {cmdId, endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &cmd);
}

void CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdId) {

	SDrawIconAtLastPosPathDrawerCommand cmd = {cmdId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &cmd);
}

void CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0];
	color_s3[1] = (short) color[1];
	color_s3[2] = (short) color[2];
	const short alpha = (short) color[3];

	SBreakPathDrawerCommand cmd = {endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &cmd);
}

void CAIAICallback::LineDrawerRestart() {

	SRestartPathDrawerCommand cmd = {false};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

void CAIAICallback::LineDrawerRestartSameColor() {

	SRestartPathDrawerCommand cmd = {true};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

int CAIAICallback::CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifeTime, int figureGroupId) {

	float pos1_f3[3];
	pos1.copyInto(pos1_f3);
	float pos2_f3[3];
	pos2.copyInto(pos2_f3);
	float pos3_f3[3];
	pos3.copyInto(pos3_f3);
	float pos4_f3[3];
	pos4.copyInto(pos4_f3);

	SCreateSplineFigureDrawerCommand cmd = {pos1_f3, pos2_f3, pos3_f3, pos4_f3, width, arrow, lifeTime, figureGroupId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

int CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) {

	float pos1_f3[3];
	pos1.copyInto(pos1_f3);
	float pos2_f3[3];
	pos2.copyInto(pos2_f3);

	SCreateLineFigureDrawerCommand cmd = {pos1_f3, pos2_f3, width, arrow, lifeTime, figureGroupId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

void CAIAICallback::SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) {

	short color_s3[3];
	color_s3[0] = (short) red;
	color_s3[1] = (short) green;
	color_s3[2] = (short) blue;

	SSetColorFigureDrawerCommand cmd = {figureGroupId, color_s3, alpha};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &cmd);
}

void CAIAICallback::DeleteFigureGroup(int figureGroupId) {

	SDeleteFigureDrawerCommand cmd = {figureGroupId};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &cmd);
}

void CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifeTime, int unitTeamId, bool transparent, bool drawBorder, int facing) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	SDrawUnitDrawerCommand cmd = {sAICallback->getUnitDefByName(teamId, name), pos_f3, rotation, lifeTime, unitTeamId, transparent, drawBorder, facing};
	sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &cmd);
}

int CAIAICallback::HandleCommand(int commandId, void* data) {
	int ret = -99;

	switch (commandId) {
		case AIHCQuerySubVersionId: {
//			SQuerySubVersionCommand cmd;
//			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, cmdTopicId, &cmd);
			ret = sAICallback->Game_getAiInterfaceVersion(teamId);
			break;
		}
		case AIHCAddMapPointId: {
			AIHCAddMapPoint* myData = (AIHCAddMapPoint*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SAddPointDrawCommand cmd = {pos, myData->label};
			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_ADD, &cmd);
			break;
		}
		case AIHCAddMapLineId: {
			AIHCAddMapLine* myData = (AIHCAddMapLine*) data;
			float posfrom[3];
			myData->posfrom.copyInto(posfrom);
			float posto[3];
			myData->posto.copyInto(posto);
			SAddLineDrawCommand cmd = {posfrom, posto};
			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_LINE_ADD, &cmd);
			break;
		}
		case AIHCRemoveMapPointId: {
			AIHCRemoveMapPoint* myData = (AIHCRemoveMapPoint*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SRemovePointDrawCommand cmd = {pos};
			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_REMOVE, &cmd);
			break;
		}
		case AIHCSendStartPosId: {
			AIHCSendStartPos* myData = (AIHCSendStartPos*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SSendStartPosCommand cmd = {myData->ready, pos};
			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_START_POS, &cmd);
			break;
		}

		case AIHCTraceRayId: {
			AIHCTraceRay* myData = (AIHCTraceRay*) data;
			float rayPos[3];
			myData->rayPos.copyInto(rayPos);
			float rayDir[3];
			myData->rayDir.copyInto(rayDir);
			STraceRayCommand cCmdData = {
				rayPos,
				rayDir,
				myData->rayLen,
				myData->srcUID,
				myData->hitUID,
				myData->flags
			};

			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY, &cCmdData);

			myData->rayLen = cCmdData.rayLen;
			myData->hitUID = cCmdData.hitUID;
			break;
		}

		case AIHCPauseId: {
			AIHCPause* cppCmdData = (AIHCPause*) data;
			SPauseCommand cCmdData = {
				cppCmdData->enable,
				cppCmdData->reason
			};
			ret = sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PAUSE, &cCmdData);
			break;
		}

		case AIHCGetDataDirId: {
			AIHCGetDataDir* cppCmdData = (AIHCGetDataDir*) data;
			cppCmdData->ret_path = sAICallback->DataDirs_allocatePath(
					teamId,
					cppCmdData->relPath,
					cppCmdData->writeable,
					cppCmdData->create,
					cppCmdData->dir,
					cppCmdData->common);
			ret = (cppCmdData->ret_path == NULL) ? 0 : 1;
			break;
		}
	}

	return ret;
}

bool CAIAICallback::ReadFile(const char* filename, void* buffer, int bufferLen) {
//		SReadFileCommand cmd = {name, buffer, bufferLen}; sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_READ_FILE, &cmd); return cmd.ret_isExecuted;
	return sAICallback->File_getContent(teamId, filename, buffer, bufferLen);
}

const char* CAIAICallback::CallLuaRules(const char* data, int inSize, int* outSize) {
		SCallLuaRulesCommand cmd = {data, inSize, outSize}; sAICallback->Engine_handleCommand(teamId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_RULES, &cmd); return cmd.ret_outData;
}


