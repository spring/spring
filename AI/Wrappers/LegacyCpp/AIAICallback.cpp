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

// these are all copies from the engine, so we do not have to adjust
// to each minor change done there
#include "MoveData.h"
#include "UnitDef.h"
#include "WeaponDef.h"
#include "FeatureDef.h"
#include "Command.h"
#include "CommandQueue.h"

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
static inline int getResourceId_Metal(const SSkirmishAICallback* sAICallback, int skirmishAIId) {

	if (resIndMetal == -1) {
		resIndMetal = sAICallback->getResourceByName(skirmishAIId, "Metal");
	}

	return resIndMetal;
}
static inline int getResourceId_Energy(const SSkirmishAICallback* sAICallback, int skirmishAIId) {

	if (resIndEnergy == -1) {
		resIndEnergy = sAICallback->getResourceByName(skirmishAIId, "Energy");
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

// FIXME: group ID's have no runtime bound
const int maxGroups = MAX_UNITS;

// FIXME: these are far too generous, but we
// don't have easy access to the right values
// on the AI side (so better waste memory than
// risk SEGVs)
static const int numUnitDefs = MAX_UNITS; // unitDefHandler->numUnitDefs;
static const int numFeatDefs = MAX_UNITS; // featureHandler->numFeatureDefs;
static const int numWeapDefs = MAX_UNITS; // weaponDefHandler->numWeaponDefs;


size_t CAIAICallback::numClbInstances = 0;
float* CAIAICallback::heightMap = NULL;
float* CAIAICallback::cornersHeightMap = NULL;
float* CAIAICallback::slopeMap = NULL;
unsigned short* CAIAICallback::losMap = NULL;
unsigned short* CAIAICallback::radarMap = NULL;
unsigned short* CAIAICallback::jammerMap = NULL;
unsigned char* CAIAICallback::metalMap = NULL;


CAIAICallback::CAIAICallback()
	: IAICallback(), skirmishAIId(-1), sAICallback(NULL) {
	init();
}

CAIAICallback::CAIAICallback(int skirmishAIId, const SSkirmishAICallback* sAICallback)
	: IAICallback(), skirmishAIId(skirmishAIId), sAICallback(sAICallback) {
	init();
}

CAIAICallback::~CAIAICallback() {

	numClbInstances--;

	for (int i=0; i < numWeapDefs; ++i) {
		delete weaponDefs[i];
		weaponDefs[i] = NULL;
	}
	delete weaponDefs;
	weaponDefs = NULL;
	delete weaponDefFrames;
	weaponDefFrames = NULL;

	for (int i=0; i < numUnitDefs; ++i) {
		delete unitDefs[i];
		unitDefs[i] = NULL;
	}
	delete unitDefs;
	unitDefs = NULL;
	delete unitDefFrames;
	unitDefFrames = NULL;

	for (int i=0; i < numFeatDefs; ++i) {
		delete featureDefs[i];
		featureDefs[i] = NULL;
	}
	delete featureDefs;
	featureDefs = NULL;
	delete featureDefFrames;
	featureDefFrames = NULL;

	for (int i=0; i < maxGroups; ++i) {
		delete groupPossibleCommands[i];
		groupPossibleCommands[i] = NULL;
	}
	delete groupPossibleCommands;
	groupPossibleCommands = NULL;

	for (int i=0; i < MAX_UNITS; ++i) {
		delete unitPossibleCommands[i];
		unitPossibleCommands[i] = NULL;
	}
	delete unitPossibleCommands;
	unitPossibleCommands = NULL;

	for (int i=0; i < MAX_UNITS; ++i) {
		delete unitCurrentCommandQueues[i];
		unitCurrentCommandQueues[i] = NULL;
	}
	delete unitCurrentCommandQueues;
	unitCurrentCommandQueues = NULL;

	if (numClbInstances == 0) {
		delete[] heightMap; heightMap = NULL;
		delete[] cornersHeightMap; cornersHeightMap = NULL;
		delete[] slopeMap; slopeMap = NULL;
		delete[] losMap; losMap = NULL;
		delete[] radarMap; radarMap = NULL;
		delete[] jammerMap; jammerMap = NULL;
		delete[] metalMap; metalMap = NULL;
	}
}


void CAIAICallback::init() {

	numClbInstances++;

	weaponDefs      = new WeaponDef*[numWeapDefs];
	weaponDefFrames = new int[numWeapDefs];
	fillWithNULL((void**) weaponDefs, numWeapDefs);
	fillWithMinusOne(weaponDefFrames, numWeapDefs);

	unitDefs      = new UnitDef*[numUnitDefs];
	unitDefFrames = new int[numUnitDefs];
	fillWithNULL((void**) unitDefs, numUnitDefs);
	fillWithMinusOne(unitDefFrames, numUnitDefs);

	featureDefs      = new FeatureDef*[numFeatDefs];
	featureDefFrames = new int[numFeatDefs];
	fillWithNULL((void**) featureDefs, numFeatDefs);
	fillWithMinusOne(featureDefFrames, numFeatDefs);

	groupPossibleCommands    = new std::vector<CommandDescription>*[maxGroups];
	unitPossibleCommands     = new std::vector<CommandDescription>*[MAX_UNITS];
	unitCurrentCommandQueues = new CCommandQueue*[MAX_UNITS];
	fillWithNULL((void**) groupPossibleCommands,    maxGroups);
	fillWithNULL((void**) unitPossibleCommands,     MAX_UNITS);
	fillWithNULL((void**) unitCurrentCommandQueues, MAX_UNITS);
}


bool CAIAICallback::PosInCamera(float3 pos, float radius) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->Map_isPosInCamera(skirmishAIId, pos_param, radius);
}

int CAIAICallback::GetCurrentFrame() {
	return sAICallback->Game_getCurrentFrame(skirmishAIId);
}

int CAIAICallback::GetMySkirmishAIId() {
	return skirmishAIId;
}

int CAIAICallback::GetMyTeam() {
	return sAICallback->Game_getMyTeam(skirmishAIId);
}

int CAIAICallback::GetMyAllyTeam() {
	return sAICallback->Game_getMyAllyTeam(skirmishAIId);
}

int CAIAICallback::GetPlayerTeam(int player) {
	return sAICallback->Game_getPlayerTeam(skirmishAIId, player);
}

int CAIAICallback::GetTeams() {
	return sAICallback->Game_getTeams(skirmishAIId);
}

const char* CAIAICallback::GetTeamSide(int otherTeamId) {
	return sAICallback->Game_getTeamSide(skirmishAIId, otherTeamId);
}

int CAIAICallback::GetTeamAllyTeam(int otherTeamId) {
	return sAICallback->Game_getTeamAllyTeam(skirmishAIId, otherTeamId);
}

float CAIAICallback::GetTeamMetalCurrent(int otherTeamId) {
	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceCurrent(skirmishAIId, otherTeamId, m);
}

float CAIAICallback::GetTeamMetalIncome(int otherTeamId) {
	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceIncome(skirmishAIId, otherTeamId, m);
}

float CAIAICallback::GetTeamMetalUsage(int otherTeamId) {
	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceUsage(skirmishAIId, otherTeamId, m);
}

float CAIAICallback::GetTeamMetalStorage(int otherTeamId) {
	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceStorage(skirmishAIId, otherTeamId, m);
}

float CAIAICallback::GetTeamEnergyCurrent(int otherTeamId) {
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceCurrent(skirmishAIId, otherTeamId, e);
}

float CAIAICallback::GetTeamEnergyIncome(int otherTeamId) {
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceIncome(skirmishAIId, otherTeamId, e);
}

float CAIAICallback::GetTeamEnergyUsage(int otherTeamId) {
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceUsage(skirmishAIId, otherTeamId, e);
}

float CAIAICallback::GetTeamEnergyStorage(int otherTeamId) {
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Game_getTeamResourceStorage(skirmishAIId, otherTeamId, e);
}

bool CAIAICallback::IsAllied(int firstAllyTeamId, int secondAllyTeamId) {
	return sAICallback->Game_isAllied(skirmishAIId, firstAllyTeamId, secondAllyTeamId);
}

int CAIAICallback::GetUnitGroup(int unitId) {
	return sAICallback->Unit_getGroup(skirmishAIId, unitId);
}

const std::vector<CommandDescription>* CAIAICallback::GetGroupCommands(int groupId) {

	int numCmds = sAICallback->Group_getSupportedCommands(skirmishAIId, groupId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Group_SupportedCommand_getId(skirmishAIId, groupId, c);
		commandDescription.name = sAICallback->Group_SupportedCommand_getName(skirmishAIId, groupId, c);
		commandDescription.tooltip = sAICallback->Group_SupportedCommand_getToolTip(skirmishAIId, groupId, c);
		commandDescription.showUnique = sAICallback->Group_SupportedCommand_isShowUnique(skirmishAIId, groupId, c);
		commandDescription.disabled = sAICallback->Group_SupportedCommand_isDisabled(skirmishAIId, groupId, c);

		int numParams = sAICallback->Group_SupportedCommand_getParams(skirmishAIId, groupId, c, NULL, 0);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Group_SupportedCommand_getParams(skirmishAIId, groupId, c, params, numParams);
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

	int numCmds = sAICallback->Unit_getSupportedCommands(skirmishAIId, unitId);

	std::vector<CommandDescription>* cmdDescVec = new std::vector<CommandDescription>();
	for (int c=0; c < numCmds; c++) {
		CommandDescription commandDescription;
		commandDescription.id = sAICallback->Unit_SupportedCommand_getId(skirmishAIId, unitId, c);
		commandDescription.name = sAICallback->Unit_SupportedCommand_getName(skirmishAIId, unitId, c);
		commandDescription.tooltip = sAICallback->Unit_SupportedCommand_getToolTip(skirmishAIId, unitId, c);
		commandDescription.showUnique = sAICallback->Unit_SupportedCommand_isShowUnique(skirmishAIId, unitId, c);
		commandDescription.disabled = sAICallback->Unit_SupportedCommand_isDisabled(skirmishAIId, unitId, c);

		int numParams = sAICallback->Unit_SupportedCommand_getParams(skirmishAIId, unitId, c, NULL, 0);
		const char** params = (const char**) calloc(numParams, sizeof(char*));
		numParams = sAICallback->Unit_SupportedCommand_getParams(skirmishAIId, unitId, c, params, numParams);
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

	const int numCmds = sAICallback->Unit_getCurrentCommands(skirmishAIId, unitId);
	const int type = sAICallback->Unit_CurrentCommand_getType(skirmishAIId, unitId);

	CCommandQueue* cc = new CCommandQueue();
	cc->queueType = (CCommandQueue::QueueType) type;
	for (int c=0; c < numCmds; c++) {
		Command command;
		command.id = sAICallback->Unit_CurrentCommand_getId(skirmishAIId, unitId, c);
		command.options = sAICallback->Unit_CurrentCommand_getOptions(skirmishAIId, unitId, c);
		command.tag = sAICallback->Unit_CurrentCommand_getTag(skirmishAIId, unitId, c);
		command.timeOut = sAICallback->Unit_CurrentCommand_getTimeOut(skirmishAIId, unitId, c);

		int numParams = sAICallback->Unit_CurrentCommand_getParams(skirmishAIId, unitId, c, NULL, 0);
		float* params = (float*) calloc(numParams, sizeof(float));
		numParams = sAICallback->Unit_CurrentCommand_getParams(skirmishAIId, unitId, c, params, numParams);
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
	return sAICallback->Unit_getAiHint(skirmishAIId, unitId);
}

int CAIAICallback::GetUnitTeam(int unitId) {
	return sAICallback->Unit_getTeam(skirmishAIId, unitId);
}

int CAIAICallback::GetUnitAllyTeam(int unitId) {
	return sAICallback->Unit_getAllyTeam(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitHealth(int unitId) {
	return sAICallback->Unit_getHealth(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitMaxHealth(int unitId) {
	return sAICallback->Unit_getMaxHealth(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitSpeed(int unitId) {
	return sAICallback->Unit_getSpeed(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitPower(int unitId) {
	return sAICallback->Unit_getPower(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitExperience(int unitId) {
	return sAICallback->Unit_getExperience(skirmishAIId, unitId);
}

float CAIAICallback::GetUnitMaxRange(int unitId) {
	return sAICallback->Unit_getMaxRange(skirmishAIId, unitId);
}

bool CAIAICallback::IsUnitActivated(int unitId) {
	return sAICallback->Unit_isActivated(skirmishAIId, unitId);
}

bool CAIAICallback::UnitBeingBuilt(int unitId) {
	return sAICallback->Unit_isBeingBuilt(skirmishAIId, unitId);
}

const UnitDef* CAIAICallback::GetUnitDef(int unitId) {
	int unitDefId = sAICallback->Unit_getDef(skirmishAIId, unitId);
	return this->GetUnitDefById(unitDefId);
}



float3 CAIAICallback::GetUnitPos(int unitId) {

	float pos_cache[3];
	sAICallback->Unit_getPos(skirmishAIId, unitId, pos_cache);
	return pos_cache;
}

float3 CAIAICallback::GetUnitVel(int unitId) {

	float pos_cache[3];
	sAICallback->Unit_getVel(skirmishAIId, unitId, pos_cache);
	return pos_cache;
}



int CAIAICallback::GetBuildingFacing(int unitId) {
	return sAICallback->Unit_getBuildingFacing(skirmishAIId, unitId);
}

bool CAIAICallback::IsUnitCloaked(int unitId) {
	return sAICallback->Unit_isCloaked(skirmishAIId, unitId);
}

bool CAIAICallback::IsUnitParalyzed(int unitId) {
	return sAICallback->Unit_isParalyzed(skirmishAIId, unitId);
}

bool CAIAICallback::IsUnitNeutral(int unitId) {
	return sAICallback->Unit_isNeutral(skirmishAIId, unitId);
}

bool CAIAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {

	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);
	resourceInfo->energyMake = sAICallback->Unit_getResourceMake(skirmishAIId, unitId, e);
	if (resourceInfo->energyMake < 0) { return false; }
	resourceInfo->energyUse = sAICallback->Unit_getResourceUse(skirmishAIId, unitId, e);
	resourceInfo->metalMake = sAICallback->Unit_getResourceMake(skirmishAIId, unitId, m);
	resourceInfo->metalUse = sAICallback->Unit_getResourceUse(skirmishAIId, unitId, m);
	return true;
}

const UnitDef* CAIAICallback::GetUnitDef(const char* unitName) {
	int unitDefId = sAICallback->getUnitDefByName(skirmishAIId, unitName);
	return this->GetUnitDefById(unitDefId);
}


const UnitDef* CAIAICallback::GetUnitDefById(int unitDefId) {
	//logT("entering: GetUnitDefById sAICallback");
	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);

	if (unitDefId < 0) {
		return NULL;
	}

	const bool doRecreate = (unitDefFrames[unitDefId] < 0);

	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;

		float pos_cache[3];

		UnitDef* unitDef = new UnitDef();

		unitDef->name = sAICallback->UnitDef_getName(skirmishAIId, unitDefId);
		unitDef->humanName = sAICallback->UnitDef_getHumanName(skirmishAIId, unitDefId);
		unitDef->filename = sAICallback->UnitDef_getFileName(skirmishAIId, unitDefId);
		//unitDef->id = sAICallback->UnitDef_getId(skirmishAIId, unitDefId);
		unitDef->id = unitDefId;
		unitDef->aihint = sAICallback->UnitDef_getAiHint(skirmishAIId, unitDefId);
		unitDef->cobID = sAICallback->UnitDef_getCobId(skirmishAIId, unitDefId);
		unitDef->techLevel = sAICallback->UnitDef_getTechLevel(skirmishAIId, unitDefId);
		unitDef->gaia = sAICallback->UnitDef_getGaia(skirmishAIId, unitDefId);
		unitDef->metalUpkeep = sAICallback->UnitDef_getUpkeep(skirmishAIId, unitDefId, m);
		unitDef->energyUpkeep = sAICallback->UnitDef_getUpkeep(skirmishAIId, unitDefId, e);
		unitDef->metalMake = sAICallback->UnitDef_getResourceMake(skirmishAIId, unitDefId, m);
		unitDef->makesMetal = sAICallback->UnitDef_getMakesResource(skirmishAIId, unitDefId, m);
		unitDef->energyMake = sAICallback->UnitDef_getResourceMake(skirmishAIId, unitDefId, e);
		unitDef->metalCost = sAICallback->UnitDef_getCost(skirmishAIId, unitDefId, m);
		unitDef->energyCost = sAICallback->UnitDef_getCost(skirmishAIId, unitDefId, e);
		unitDef->buildTime = sAICallback->UnitDef_getBuildTime(skirmishAIId, unitDefId);
		unitDef->extractsMetal = sAICallback->UnitDef_getExtractsResource(skirmishAIId, unitDefId, m);
		unitDef->extractRange = sAICallback->UnitDef_getResourceExtractorRange(skirmishAIId, unitDefId, m);
		unitDef->windGenerator = sAICallback->UnitDef_getWindResourceGenerator(skirmishAIId, unitDefId, e);
		unitDef->tidalGenerator = sAICallback->UnitDef_getTidalResourceGenerator(skirmishAIId, unitDefId, e);
		unitDef->metalStorage = sAICallback->UnitDef_getStorage(skirmishAIId, unitDefId, m);
		unitDef->energyStorage = sAICallback->UnitDef_getStorage(skirmishAIId, unitDefId, e);
		unitDef->autoHeal = sAICallback->UnitDef_getAutoHeal(skirmishAIId, unitDefId);
		unitDef->idleAutoHeal = sAICallback->UnitDef_getIdleAutoHeal(skirmishAIId, unitDefId);
		unitDef->idleTime = sAICallback->UnitDef_getIdleTime(skirmishAIId, unitDefId);
		unitDef->power = sAICallback->UnitDef_getPower(skirmishAIId, unitDefId);
		unitDef->health = sAICallback->UnitDef_getHealth(skirmishAIId, unitDefId);
		unitDef->category = sAICallback->UnitDef_getCategory(skirmishAIId, unitDefId);
		unitDef->speed = sAICallback->UnitDef_getSpeed(skirmishAIId, unitDefId);
		unitDef->turnRate = sAICallback->UnitDef_getTurnRate(skirmishAIId, unitDefId);
		unitDef->turnInPlace = sAICallback->UnitDef_isTurnInPlace(skirmishAIId, unitDefId);
		unitDef->upright = sAICallback->UnitDef_isUpright(skirmishAIId, unitDefId);
		unitDef->collide = sAICallback->UnitDef_isCollide(skirmishAIId, unitDefId);
		unitDef->losRadius = sAICallback->UnitDef_getLosRadius(skirmishAIId, unitDefId);
		unitDef->airLosRadius = sAICallback->UnitDef_getAirLosRadius(skirmishAIId, unitDefId);
		unitDef->losHeight = sAICallback->UnitDef_getLosHeight(skirmishAIId, unitDefId);
		unitDef->radarRadius = sAICallback->UnitDef_getRadarRadius(skirmishAIId, unitDefId);
		unitDef->sonarRadius = sAICallback->UnitDef_getSonarRadius(skirmishAIId, unitDefId);
		unitDef->jammerRadius = sAICallback->UnitDef_getJammerRadius(skirmishAIId, unitDefId);
		unitDef->sonarJamRadius = sAICallback->UnitDef_getSonarJamRadius(skirmishAIId, unitDefId);
		unitDef->seismicRadius = sAICallback->UnitDef_getSeismicRadius(skirmishAIId, unitDefId);
		unitDef->seismicSignature = sAICallback->UnitDef_getSeismicSignature(skirmishAIId, unitDefId);
		unitDef->stealth = sAICallback->UnitDef_isStealth(skirmishAIId, unitDefId);
		unitDef->sonarStealth = sAICallback->UnitDef_isSonarStealth(skirmishAIId, unitDefId);
		unitDef->buildRange3D = sAICallback->UnitDef_isBuildRange3D(skirmishAIId, unitDefId);
		unitDef->buildDistance = sAICallback->UnitDef_getBuildDistance(skirmishAIId, unitDefId);
		unitDef->buildSpeed = sAICallback->UnitDef_getBuildSpeed(skirmishAIId, unitDefId);
		unitDef->reclaimSpeed = sAICallback->UnitDef_getReclaimSpeed(skirmishAIId, unitDefId);
		unitDef->repairSpeed = sAICallback->UnitDef_getRepairSpeed(skirmishAIId, unitDefId);
		unitDef->maxRepairSpeed = sAICallback->UnitDef_getMaxRepairSpeed(skirmishAIId, unitDefId);
		unitDef->resurrectSpeed = sAICallback->UnitDef_getResurrectSpeed(skirmishAIId, unitDefId);
		unitDef->captureSpeed = sAICallback->UnitDef_getCaptureSpeed(skirmishAIId, unitDefId);
		unitDef->terraformSpeed = sAICallback->UnitDef_getTerraformSpeed(skirmishAIId, unitDefId);
		unitDef->mass = sAICallback->UnitDef_getMass(skirmishAIId, unitDefId);
		unitDef->pushResistant = sAICallback->UnitDef_isPushResistant(skirmishAIId, unitDefId);
		unitDef->strafeToAttack = sAICallback->UnitDef_isStrafeToAttack(skirmishAIId, unitDefId);
		unitDef->minCollisionSpeed = sAICallback->UnitDef_getMinCollisionSpeed(skirmishAIId, unitDefId);
		unitDef->slideTolerance = sAICallback->UnitDef_getSlideTolerance(skirmishAIId, unitDefId);
		unitDef->maxSlope = sAICallback->UnitDef_getMaxSlope(skirmishAIId, unitDefId);
		unitDef->maxHeightDif = sAICallback->UnitDef_getMaxHeightDif(skirmishAIId, unitDefId);
		unitDef->minWaterDepth = sAICallback->UnitDef_getMinWaterDepth(skirmishAIId, unitDefId);
		unitDef->waterline = sAICallback->UnitDef_getWaterline(skirmishAIId, unitDefId);
		unitDef->maxWaterDepth = sAICallback->UnitDef_getMaxWaterDepth(skirmishAIId, unitDefId);
		unitDef->armoredMultiple = sAICallback->UnitDef_getArmoredMultiple(skirmishAIId, unitDefId);
		unitDef->armorType = sAICallback->UnitDef_getArmorType(skirmishAIId, unitDefId);
		unitDef->flankingBonusMode = sAICallback->UnitDef_FlankingBonus_getMode(skirmishAIId, unitDefId);
		sAICallback->UnitDef_FlankingBonus_getDir(skirmishAIId, unitDefId, pos_cache);
		unitDef->flankingBonusDir = pos_cache;
		unitDef->flankingBonusMax = sAICallback->UnitDef_FlankingBonus_getMax(skirmishAIId, unitDefId);
		unitDef->flankingBonusMin = sAICallback->UnitDef_FlankingBonus_getMin(skirmishAIId, unitDefId);
		unitDef->flankingBonusMobilityAdd = sAICallback->UnitDef_FlankingBonus_getMobilityAdd(skirmishAIId, unitDefId);
		unitDef->maxWeaponRange = sAICallback->UnitDef_getMaxWeaponRange(skirmishAIId, unitDefId);
		unitDef->type = sAICallback->UnitDef_getType(skirmishAIId, unitDefId);
		unitDef->tooltip = sAICallback->UnitDef_getTooltip(skirmishAIId, unitDefId);
		unitDef->wreckName = sAICallback->UnitDef_getWreckName(skirmishAIId, unitDefId);
		unitDef->deathExplosion = sAICallback->UnitDef_getDeathExplosion(skirmishAIId, unitDefId);
		unitDef->selfDExplosion = sAICallback->UnitDef_getSelfDExplosion(skirmishAIId, unitDefId);
		unitDef->categoryString = sAICallback->UnitDef_getCategoryString(skirmishAIId, unitDefId);
		unitDef->canSelfD = sAICallback->UnitDef_isAbleToSelfD(skirmishAIId, unitDefId);
		unitDef->selfDCountdown = sAICallback->UnitDef_getSelfDCountdown(skirmishAIId, unitDefId);
		unitDef->canSubmerge = sAICallback->UnitDef_isAbleToSubmerge(skirmishAIId, unitDefId);
		unitDef->canfly = sAICallback->UnitDef_isAbleToFly(skirmishAIId, unitDefId);
		unitDef->canmove = sAICallback->UnitDef_isAbleToMove(skirmishAIId, unitDefId);
		unitDef->canhover = sAICallback->UnitDef_isAbleToHover(skirmishAIId, unitDefId);
		unitDef->floater = sAICallback->UnitDef_isFloater(skirmishAIId, unitDefId);
		unitDef->builder = sAICallback->UnitDef_isBuilder(skirmishAIId, unitDefId);
		unitDef->activateWhenBuilt = sAICallback->UnitDef_isActivateWhenBuilt(skirmishAIId, unitDefId);
		unitDef->onoffable = sAICallback->UnitDef_isOnOffable(skirmishAIId, unitDefId);
		unitDef->fullHealthFactory = sAICallback->UnitDef_isFullHealthFactory(skirmishAIId, unitDefId);
		unitDef->factoryHeadingTakeoff = sAICallback->UnitDef_isFactoryHeadingTakeoff(skirmishAIId, unitDefId);
		unitDef->reclaimable = sAICallback->UnitDef_isReclaimable(skirmishAIId, unitDefId);
		unitDef->capturable = sAICallback->UnitDef_isCapturable(skirmishAIId, unitDefId);
		unitDef->canRestore = sAICallback->UnitDef_isAbleToRestore(skirmishAIId, unitDefId);
		unitDef->canRepair = sAICallback->UnitDef_isAbleToRepair(skirmishAIId, unitDefId);
		unitDef->canSelfRepair = sAICallback->UnitDef_isAbleToSelfRepair(skirmishAIId, unitDefId);
		unitDef->canReclaim = sAICallback->UnitDef_isAbleToReclaim(skirmishAIId, unitDefId);
		unitDef->canAttack = sAICallback->UnitDef_isAbleToAttack(skirmishAIId, unitDefId);
		unitDef->canPatrol = sAICallback->UnitDef_isAbleToPatrol(skirmishAIId, unitDefId);
		unitDef->canFight = sAICallback->UnitDef_isAbleToFight(skirmishAIId, unitDefId);
		unitDef->canGuard = sAICallback->UnitDef_isAbleToGuard(skirmishAIId, unitDefId);
		unitDef->canAssist = sAICallback->UnitDef_isAbleToAssist(skirmishAIId, unitDefId);
		unitDef->canBeAssisted = sAICallback->UnitDef_isAssistable(skirmishAIId, unitDefId);
		unitDef->canRepeat = sAICallback->UnitDef_isAbleToRepeat(skirmishAIId, unitDefId);
		unitDef->canFireControl = sAICallback->UnitDef_isAbleToFireControl(skirmishAIId, unitDefId);
		unitDef->fireState = sAICallback->UnitDef_getFireState(skirmishAIId, unitDefId);
		unitDef->moveState = sAICallback->UnitDef_getMoveState(skirmishAIId, unitDefId);
		unitDef->wingDrag = sAICallback->UnitDef_getWingDrag(skirmishAIId, unitDefId);
		unitDef->wingAngle = sAICallback->UnitDef_getWingAngle(skirmishAIId, unitDefId);
		unitDef->drag = sAICallback->UnitDef_getDrag(skirmishAIId, unitDefId);
		unitDef->frontToSpeed = sAICallback->UnitDef_getFrontToSpeed(skirmishAIId, unitDefId);
		unitDef->speedToFront = sAICallback->UnitDef_getSpeedToFront(skirmishAIId, unitDefId);
		unitDef->myGravity = sAICallback->UnitDef_getMyGravity(skirmishAIId, unitDefId);
		unitDef->maxBank = sAICallback->UnitDef_getMaxBank(skirmishAIId, unitDefId);
		unitDef->maxPitch = sAICallback->UnitDef_getMaxPitch(skirmishAIId, unitDefId);
		unitDef->turnRadius = sAICallback->UnitDef_getTurnRadius(skirmishAIId, unitDefId);
		unitDef->wantedHeight = sAICallback->UnitDef_getWantedHeight(skirmishAIId, unitDefId);
		unitDef->verticalSpeed = sAICallback->UnitDef_getVerticalSpeed(skirmishAIId, unitDefId);
		unitDef->canCrash = sAICallback->UnitDef_isAbleToCrash(skirmishAIId, unitDefId);
		unitDef->hoverAttack = sAICallback->UnitDef_isHoverAttack(skirmishAIId, unitDefId);
		unitDef->airStrafe = sAICallback->UnitDef_isAirStrafe(skirmishAIId, unitDefId);
		unitDef->dlHoverFactor = sAICallback->UnitDef_getDlHoverFactor(skirmishAIId, unitDefId);
		unitDef->maxAcc = sAICallback->UnitDef_getMaxAcceleration(skirmishAIId, unitDefId);
		unitDef->maxDec = sAICallback->UnitDef_getMaxDeceleration(skirmishAIId, unitDefId);
		unitDef->maxAileron = sAICallback->UnitDef_getMaxAileron(skirmishAIId, unitDefId);
		unitDef->maxElevator = sAICallback->UnitDef_getMaxElevator(skirmishAIId, unitDefId);
		unitDef->maxRudder = sAICallback->UnitDef_getMaxRudder(skirmishAIId, unitDefId);
		{
			static const size_t facings = 4;
			const int yardMap_size = sAICallback->UnitDef_getYardMap(skirmishAIId, unitDefId, 0, NULL, 0);
			short* tmpYardMap = new short[yardMap_size];

			for (int ym = 0 ; ym < facings; ++ym) {
				sAICallback->UnitDef_getYardMap(skirmishAIId, unitDefId, ym, tmpYardMap, yardMap_size);
				unitDef->yardmaps[ym] = new unsigned char[yardMap_size]; // this will be deleted in the dtor
				for (int i = 0; i < yardMap_size; ++i) {
					unitDef->yardmaps[ym][i] = (const char) tmpYardMap[i];
				}
			}

			delete[] tmpYardMap;
		}
		unitDef->xsize = sAICallback->UnitDef_getXSize(skirmishAIId, unitDefId);
		unitDef->zsize = sAICallback->UnitDef_getZSize(skirmishAIId, unitDefId);
		unitDef->buildangle = sAICallback->UnitDef_getBuildAngle(skirmishAIId, unitDefId);
		unitDef->loadingRadius = sAICallback->UnitDef_getLoadingRadius(skirmishAIId, unitDefId);
		unitDef->unloadSpread = sAICallback->UnitDef_getUnloadSpread(skirmishAIId, unitDefId);
		unitDef->transportCapacity = sAICallback->UnitDef_getTransportCapacity(skirmishAIId, unitDefId);
		unitDef->transportSize = sAICallback->UnitDef_getTransportSize(skirmishAIId, unitDefId);
		unitDef->minTransportSize = sAICallback->UnitDef_getMinTransportSize(skirmishAIId, unitDefId);
		unitDef->isAirBase = sAICallback->UnitDef_isAirBase(skirmishAIId, unitDefId);
		unitDef->transportMass = sAICallback->UnitDef_getTransportMass(skirmishAIId, unitDefId);
		unitDef->minTransportMass = sAICallback->UnitDef_getMinTransportMass(skirmishAIId, unitDefId);
		unitDef->holdSteady = sAICallback->UnitDef_isHoldSteady(skirmishAIId, unitDefId);
		unitDef->releaseHeld = sAICallback->UnitDef_isReleaseHeld(skirmishAIId, unitDefId);
		unitDef->cantBeTransported = sAICallback->UnitDef_isNotTransportable(skirmishAIId, unitDefId);
		unitDef->transportByEnemy = sAICallback->UnitDef_isTransportByEnemy(skirmishAIId, unitDefId);
		unitDef->transportUnloadMethod = sAICallback->UnitDef_getTransportUnloadMethod(skirmishAIId, unitDefId);
		unitDef->fallSpeed = sAICallback->UnitDef_getFallSpeed(skirmishAIId, unitDefId);
		unitDef->unitFallSpeed = sAICallback->UnitDef_getUnitFallSpeed(skirmishAIId, unitDefId);
		unitDef->canCloak = sAICallback->UnitDef_isAbleToCloak(skirmishAIId, unitDefId);
		unitDef->startCloaked = sAICallback->UnitDef_isStartCloaked(skirmishAIId, unitDefId);
		unitDef->cloakCost = sAICallback->UnitDef_getCloakCost(skirmishAIId, unitDefId);
		unitDef->cloakCostMoving = sAICallback->UnitDef_getCloakCostMoving(skirmishAIId, unitDefId);
		unitDef->decloakDistance = sAICallback->UnitDef_getDecloakDistance(skirmishAIId, unitDefId);
		unitDef->decloakSpherical = sAICallback->UnitDef_isDecloakSpherical(skirmishAIId, unitDefId);
		unitDef->decloakOnFire = sAICallback->UnitDef_isDecloakOnFire(skirmishAIId, unitDefId);
		unitDef->canKamikaze = sAICallback->UnitDef_isAbleToKamikaze(skirmishAIId, unitDefId);
		unitDef->kamikazeDist = sAICallback->UnitDef_getKamikazeDist(skirmishAIId, unitDefId);
		unitDef->targfac = sAICallback->UnitDef_isTargetingFacility(skirmishAIId, unitDefId);
		unitDef->canDGun = sAICallback->UnitDef_isAbleToDGun(skirmishAIId, unitDefId);
		unitDef->needGeo = sAICallback->UnitDef_isNeedGeo(skirmishAIId, unitDefId);
		unitDef->isFeature = sAICallback->UnitDef_isFeature(skirmishAIId, unitDefId);
		unitDef->hideDamage = sAICallback->UnitDef_isHideDamage(skirmishAIId, unitDefId);
		unitDef->isCommander = sAICallback->UnitDef_isCommander(skirmishAIId, unitDefId);
		unitDef->showPlayerName = sAICallback->UnitDef_isShowPlayerName(skirmishAIId, unitDefId);
		unitDef->canResurrect = sAICallback->UnitDef_isAbleToResurrect(skirmishAIId, unitDefId);
		unitDef->canCapture = sAICallback->UnitDef_isAbleToCapture(skirmishAIId, unitDefId);
		unitDef->highTrajectoryType = sAICallback->UnitDef_getHighTrajectoryType(skirmishAIId, unitDefId);
		unitDef->noChaseCategory = sAICallback->UnitDef_getNoChaseCategory(skirmishAIId, unitDefId);
		unitDef->leaveTracks = sAICallback->UnitDef_isLeaveTracks(skirmishAIId, unitDefId);
		unitDef->trackWidth = sAICallback->UnitDef_getTrackWidth(skirmishAIId, unitDefId);
		unitDef->trackOffset = sAICallback->UnitDef_getTrackOffset(skirmishAIId, unitDefId);
		unitDef->trackStrength = sAICallback->UnitDef_getTrackStrength(skirmishAIId, unitDefId);
		unitDef->trackStretch = sAICallback->UnitDef_getTrackStretch(skirmishAIId, unitDefId);
		unitDef->trackType = sAICallback->UnitDef_getTrackType(skirmishAIId, unitDefId);
		unitDef->canDropFlare = sAICallback->UnitDef_isAbleToDropFlare(skirmishAIId, unitDefId);
		unitDef->flareReloadTime = sAICallback->UnitDef_getFlareReloadTime(skirmishAIId, unitDefId);
		unitDef->flareEfficiency = sAICallback->UnitDef_getFlareEfficiency(skirmishAIId, unitDefId);
		unitDef->flareDelay = sAICallback->UnitDef_getFlareDelay(skirmishAIId, unitDefId);
		sAICallback->UnitDef_getFlareDropVector(skirmishAIId, unitDefId, pos_cache);
		unitDef->flareDropVector = pos_cache;
		unitDef->flareTime = sAICallback->UnitDef_getFlareTime(skirmishAIId, unitDefId);
		unitDef->flareSalvoSize = sAICallback->UnitDef_getFlareSalvoSize(skirmishAIId, unitDefId);
		unitDef->flareSalvoDelay = sAICallback->UnitDef_getFlareSalvoDelay(skirmishAIId, unitDefId);
		//unitDef->smoothAnim = sAICallback->UnitDef_isSmoothAnim(skirmishAIId, unitDefId);
		unitDef->smoothAnim = false;
		unitDef->canLoopbackAttack = sAICallback->UnitDef_isAbleToLoopbackAttack(skirmishAIId, unitDefId);
		unitDef->levelGround = sAICallback->UnitDef_isLevelGround(skirmishAIId, unitDefId);
		unitDef->useBuildingGroundDecal = sAICallback->UnitDef_isUseBuildingGroundDecal(skirmishAIId, unitDefId);
		unitDef->buildingDecalType = sAICallback->UnitDef_getBuildingDecalType(skirmishAIId, unitDefId);
		unitDef->buildingDecalSizeX = sAICallback->UnitDef_getBuildingDecalSizeX(skirmishAIId, unitDefId);
		unitDef->buildingDecalSizeY = sAICallback->UnitDef_getBuildingDecalSizeY(skirmishAIId, unitDefId);
		unitDef->buildingDecalDecaySpeed = sAICallback->UnitDef_getBuildingDecalDecaySpeed(skirmishAIId, unitDefId);
		unitDef->isFirePlatform = sAICallback->UnitDef_isFirePlatform(skirmishAIId, unitDefId);
		unitDef->maxFuel = sAICallback->UnitDef_getMaxFuel(skirmishAIId, unitDefId);
		unitDef->refuelTime = sAICallback->UnitDef_getRefuelTime(skirmishAIId, unitDefId);
		unitDef->minAirBasePower = sAICallback->UnitDef_getMinAirBasePower(skirmishAIId, unitDefId);
		unitDef->maxThisUnit = sAICallback->UnitDef_getMaxThisUnit(skirmishAIId, unitDefId);
		//unitDef->decoyDef = sAICallback->UnitDef_getDecoyDefId(skirmishAIId, unitDefId);
		unitDef->shieldWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getShieldDef(skirmishAIId, unitDefId));
		unitDef->stockpileWeaponDef = this->GetWeaponDefById(sAICallback->UnitDef_getStockpileDef(skirmishAIId, unitDefId));

		{
			int numBuildOpts = sAICallback->UnitDef_getBuildOptions(skirmishAIId, unitDefId, NULL, 0);
			int* buildOpts = new int[numBuildOpts];

			numBuildOpts = sAICallback->UnitDef_getBuildOptions(skirmishAIId, unitDefId, buildOpts, numBuildOpts);

			for (int b=0; b < numBuildOpts; b++) {
				unitDef->buildOptions[b] = sAICallback->UnitDef_getName(skirmishAIId, buildOpts[b]);
			}

			delete[] buildOpts;
		}
		{
			const int size = sAICallback->UnitDef_getCustomParams(skirmishAIId, unitDefId, NULL, NULL);
			const char** cKeys = (const char**) calloc(size, sizeof(char*));
			const char** cValues = (const char**) calloc(size, sizeof(char*));
			sAICallback->UnitDef_getCustomParams(skirmishAIId, unitDefId, cKeys, cValues);
			int i;
			for (i = 0; i < size; ++i) {
				unitDef->customParams[cKeys[i]] = cValues[i];
			}
			free(cKeys);
			free(cValues);
		}

		if (sAICallback->UnitDef_isMoveDataAvailable(skirmishAIId, unitDefId)) {
			unitDef->movedata = new MoveData();
			unitDef->movedata->maxAcceleration = sAICallback->UnitDef_MoveData_getMaxAcceleration(skirmishAIId, unitDefId);
			unitDef->movedata->maxBreaking = sAICallback->UnitDef_MoveData_getMaxBreaking(skirmishAIId, unitDefId);
			unitDef->movedata->maxSpeed = sAICallback->UnitDef_MoveData_getMaxSpeed(skirmishAIId, unitDefId);
			unitDef->movedata->maxTurnRate = sAICallback->UnitDef_MoveData_getMaxTurnRate(skirmishAIId, unitDefId);

			unitDef->movedata->xsize = sAICallback->UnitDef_MoveData_getXSize(skirmishAIId, unitDefId);
			unitDef->movedata->zsize = sAICallback->UnitDef_MoveData_getZSize(skirmishAIId, unitDefId);
			unitDef->movedata->depth = sAICallback->UnitDef_MoveData_getDepth(skirmishAIId, unitDefId);
			unitDef->movedata->maxSlope = sAICallback->UnitDef_MoveData_getMaxSlope(skirmishAIId, unitDefId);
			unitDef->movedata->slopeMod = sAICallback->UnitDef_MoveData_getSlopeMod(skirmishAIId, unitDefId);
			unitDef->movedata->depthMod = sAICallback->UnitDef_MoveData_getDepthMod(skirmishAIId, unitDefId);
			unitDef->movedata->pathType = sAICallback->UnitDef_MoveData_getPathType(skirmishAIId, unitDefId);
			unitDef->movedata->crushStrength = sAICallback->UnitDef_MoveData_getCrushStrength(skirmishAIId, unitDefId);
			unitDef->movedata->moveType = (enum MoveData::MoveType) sAICallback->UnitDef_MoveData_getMoveType(skirmishAIId, unitDefId);
			unitDef->movedata->moveFamily = (enum MoveData::MoveFamily) sAICallback->UnitDef_MoveData_getMoveFamily(skirmishAIId, unitDefId);
			unitDef->movedata->terrainClass = (enum MoveData::TerrainClass) sAICallback->UnitDef_MoveData_getTerrainClass(skirmishAIId, unitDefId);

			unitDef->movedata->followGround = sAICallback->UnitDef_MoveData_getFollowGround(skirmishAIId, unitDefId);
			unitDef->movedata->subMarine = sAICallback->UnitDef_MoveData_isSubMarine(skirmishAIId, unitDefId);
			unitDef->movedata->name = std::string(sAICallback->UnitDef_MoveData_getName(skirmishAIId, unitDefId));
		} else {
			unitDef->movedata = NULL;
		}

		const int numWeapons = sAICallback->UnitDef_getWeaponMounts(skirmishAIId, unitDefId);
		for (int w = 0; w < numWeapons; ++w) {
			unitDef->weapons.push_back(UnitDef::UnitDefWeapon());
			unitDef->weapons[w].name = sAICallback->UnitDef_WeaponMount_getName(skirmishAIId, unitDefId, w);
			int weaponDefId = sAICallback->UnitDef_WeaponMount_getWeaponDef(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].def = this->GetWeaponDefById(weaponDefId);
			unitDef->weapons[w].slavedTo = sAICallback->UnitDef_WeaponMount_getSlavedTo(skirmishAIId, unitDefId, w);
			sAICallback->UnitDef_WeaponMount_getMainDir(skirmishAIId, unitDefId, w, pos_cache);
			unitDef->weapons[w].mainDir = pos_cache;
			unitDef->weapons[w].maxAngleDif = sAICallback->UnitDef_WeaponMount_getMaxAngleDif(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].fuelUsage = sAICallback->UnitDef_WeaponMount_getFuelUsage(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].badTargetCat = sAICallback->UnitDef_WeaponMount_getBadTargetCategory(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].onlyTargetCat = sAICallback->UnitDef_WeaponMount_getOnlyTargetCategory(skirmishAIId, unitDefId, w);
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
	return sAICallback->getEnemyUnits(skirmishAIId, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getEnemyUnitsIn(skirmishAIId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max) {
	return sAICallback->getEnemyUnitsInRadarAndLos(skirmishAIId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getFriendlyUnits(skirmishAIId, unitIds, unitIds_max);
}

int CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getFriendlyUnitsIn(skirmishAIId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getNeutralUnits(skirmishAIId, unitIds, unitIds_max);
}

int CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->getNeutralUnitsIn(skirmishAIId, pos_param, radius, unitIds, unitIds_max);
}

int CAIAICallback::GetMapWidth() {
	return sAICallback->Map_getWidth(skirmishAIId);
}

int CAIAICallback::GetMapHeight() {
	return sAICallback->Map_getHeight(skirmishAIId);
}

const float* CAIAICallback::GetHeightMap() {

	if (heightMap == NULL) {
		const int size = sAICallback->Map_getHeightMap(skirmishAIId, NULL, 0);
		heightMap = new float[size]; // NOTE: memory leak, but will be used till end of the game anyway
		sAICallback->Map_getHeightMap(skirmishAIId, heightMap, size);
	}

	return heightMap;
}

const float* CAIAICallback::GetCornersHeightMap() {

	if (cornersHeightMap == NULL) {
		const int size = sAICallback->Map_getCornersHeightMap(skirmishAIId, NULL, 0);
		cornersHeightMap = new float[size]; // NOTE: memory leak, but will be used till end of the game anyway
		sAICallback->Map_getCornersHeightMap(skirmishAIId, cornersHeightMap, size);
	}

	return cornersHeightMap;
}

float CAIAICallback::GetMinHeight() {
	return sAICallback->Map_getMinHeight(skirmishAIId);
}

float CAIAICallback::GetMaxHeight() {
	return sAICallback->Map_getMaxHeight(skirmishAIId);
}

const float* CAIAICallback::GetSlopeMap() {

	if (slopeMap == NULL) {
		const int size = sAICallback->Map_getSlopeMap(skirmishAIId, NULL, 0);
		slopeMap = new float[size]; // NOTE: memory leak, but will be used till end of the game anyway
		sAICallback->Map_getSlopeMap(skirmishAIId, slopeMap, size);
	}

	return slopeMap;
}

const unsigned short* CAIAICallback::GetLosMap() {

	if (losMap == NULL) {
		const int size = sAICallback->Map_getLosMap(skirmishAIId, NULL, 0);
		int* tmpLosMap = new int[size];
		sAICallback->Map_getLosMap(skirmishAIId, tmpLosMap, size);
		losMap = new unsigned short[size]; // NOTE: memory leak, but will be used till end of the game anyway
		copyIntToUShortArray(tmpLosMap, losMap, size);
		delete[] tmpLosMap;
	}

	return losMap;
}

int CAIAICallback::GetLosMapResolution() {

	int fullSize = GetMapWidth() * GetMapHeight();
	int losSize = sAICallback->Map_getLosMap(skirmishAIId, NULL, 0);

	return fullSize / losSize;
}

const unsigned short* CAIAICallback::GetRadarMap() {

	if (radarMap == NULL) {
		const int size = sAICallback->Map_getRadarMap(skirmishAIId, NULL, 0);
		int* tmpRadarMap = new int[size];
		sAICallback->Map_getRadarMap(skirmishAIId, tmpRadarMap, size);
		radarMap = new unsigned short[size]; // NOTE: memory leak, but will be used till end of the game anyway
		copyIntToUShortArray(tmpRadarMap, radarMap, size);
		delete[] tmpRadarMap;
	}

	return radarMap;
}

const unsigned short* CAIAICallback::GetJammerMap() {

	if (jammerMap == NULL) {
		const int size = sAICallback->Map_getJammerMap(skirmishAIId, NULL, 0);
		int* tmpJammerMap = new int[size];
		sAICallback->Map_getJammerMap(skirmishAIId, tmpJammerMap, size);
		jammerMap = new unsigned short[size]; // NOTE: memory leak, but will be used till end of the game anyway
		copyIntToUShortArray(tmpJammerMap, jammerMap, size);
		delete[] tmpJammerMap;
	}

	return jammerMap;
}

const unsigned char* CAIAICallback::GetMetalMap() {

	static const int m = getResourceId_Metal(sAICallback, skirmishAIId);

	if (metalMap == NULL) {
		const int size = sAICallback->Map_getResourceMapRaw(skirmishAIId, m, NULL, 0);
		short* tmpMetalMap = new short[size];
		sAICallback->Map_getResourceMapRaw(skirmishAIId, m, tmpMetalMap, size);
		metalMap = new unsigned char[size]; // NOTE: memory leak, but will be used till end of the game anyway
		copyShortToUCharArray(tmpMetalMap, metalMap, size);
		delete[] tmpMetalMap;
	}

	return metalMap;
}

int CAIAICallback::GetMapHash() {
	return sAICallback->Map_getHash(skirmishAIId);
}

const char* CAIAICallback::GetMapName() {
	return sAICallback->Map_getName(skirmishAIId);
}

const char* CAIAICallback::GetMapHumanName() {
	return sAICallback->Map_getHumanName(skirmishAIId);
}

int CAIAICallback::GetModHash() {
	return sAICallback->Mod_getHash(skirmishAIId);
}

const char* CAIAICallback::GetModName() {
	return sAICallback->Mod_getFileName(skirmishAIId);
}

const char* CAIAICallback::GetModHumanName() {
	return sAICallback->Mod_getHumanName(skirmishAIId);
}

const char* CAIAICallback::GetModShortName() {
	return sAICallback->Mod_getShortName(skirmishAIId);
}

const char* CAIAICallback::GetModVersion() {
	return sAICallback->Mod_getVersion(skirmishAIId);
}

float CAIAICallback::GetElevation(float x, float z) {
	return sAICallback->Map_getElevationAt(skirmishAIId, x, z);
}


float CAIAICallback::GetMaxMetal() const {
	static const int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Map_getMaxResource(skirmishAIId, m);
}
float CAIAICallback::GetExtractorRadius() const {
	static const int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Map_getExtractorRadius(skirmishAIId, m);
}

float CAIAICallback::GetMinWind() const { return sAICallback->Map_getMinWind(skirmishAIId); }
float CAIAICallback::GetMaxWind() const { return sAICallback->Map_getMaxWind(skirmishAIId); }
float CAIAICallback::GetCurWind() const { return sAICallback->Map_getCurWind(skirmishAIId); }

float CAIAICallback::GetTidalStrength() const  { return sAICallback->Map_getTidalStrength(skirmishAIId); }
float CAIAICallback::GetGravity() const { return sAICallback->Map_getGravity(skirmishAIId); }


bool CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {

	float pos_param[3];
	pos.copyInto(pos_param);

	return sAICallback->Map_isPossibleToBuildAt(skirmishAIId, unitDef->id, pos_param, facing);
}

float3 CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing) {

	float pos_param[3];
	pos.copyInto(pos_param);

	float pos_cache[3];
	sAICallback->Map_findClosestBuildSite(skirmishAIId, unitDef->id, pos_param, searchRadius, minDist, facing, pos_cache);
	return pos_cache;
}

/*
bool CAIAICallback::GetProperty(int id, int property, void* dst) {
//	return sAICallback->getProperty(skirmishAIId, id, property, dst);
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
			(*(float*)data) = sAICallback->Unit_getCurrentFuel(skirmishAIId, unitId);
			return (*(float*)data) != -1.0f;
		}
		case AIVAL_STOCKPILED: {
			(*(int*)data) = sAICallback->Unit_getStockpile(skirmishAIId, unitId);
			return (*(int*)data) != -1;
		}
		case AIVAL_STOCKPILE_QUED: {
			(*(int*)data) = sAICallback->Unit_getStockpileQueued(skirmishAIId, unitId);
			return (*(int*)data) != -1;
		}
		case AIVAL_UNIT_MAXSPEED: {
			(*(float*) data) = sAICallback->Unit_getMaxSpeed(skirmishAIId, unitId);
			return (*(float*)data) != -1.0f;
		}
		default:
			return false;
	}
	return false;
}

/*
bool CAIAICallback::GetValue(int valueId, void* dst) {
//	return sAICallback->getValue(skirmishAIId, valueId, dst);
	return false;
}
*/
bool CAIAICallback::GetValue(int valueId, void *data)
{
	switch (valueId) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = sAICallback->WeaponDef_getNumDamageTypes(skirmishAIId);
			return true;
		}case AIVAL_EXCEPTION_HANDLING:{
			*(bool*)data = sAICallback->Game_isExceptionHandlingEnabled(skirmishAIId);
			return true;
		}case AIVAL_MAP_CHECKSUM:{
			*(unsigned int*)data = sAICallback->Map_getChecksum(skirmishAIId);
			return true;
		}case AIVAL_DEBUG_MODE:{
			*(bool*)data = sAICallback->Game_isDebugModeEnabled(skirmishAIId);
			return true;
		}case AIVAL_GAME_PAUSED:{
			*(bool*)data = sAICallback->Game_isPaused(skirmishAIId);
			return true;
		}case AIVAL_GAME_SPEED_FACTOR:{
			*(float*)data = sAICallback->Game_getSpeedFactor(skirmishAIId);
			return true;
		}case AIVAL_GUI_VIEW_RANGE:{
			*(float*)data = sAICallback->Gui_getViewRange(skirmishAIId);
			return true;
		}case AIVAL_GUI_SCREENX:{
			*(float*)data = sAICallback->Gui_getScreenX(skirmishAIId);
			return true;
		}case AIVAL_GUI_SCREENY:{
			*(float*)data = sAICallback->Gui_getScreenY(skirmishAIId);
			return true;
		}case AIVAL_GUI_CAMERA_DIR:{
			float pos_cache[3];
			sAICallback->Gui_Camera_getDirection(skirmishAIId, pos_cache);
			*(float3*)data = pos_cache;
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			float pos_cache[3];
			sAICallback->Gui_Camera_getPosition(skirmishAIId, pos_cache);
			*(float3*)data = pos_cache;
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			//sAICallback->File_locateForReading(skirmishAIId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->DataDirs_locatePath(skirmishAIId, absPath, absPath_sizeMax, (const char*) data, false, false, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}case AIVAL_LOCATE_FILE_W:{
			//sAICallback->File_locateForWriting(skirmishAIId, (char*) data);
			static const size_t absPath_sizeMax = 2048;
			char absPath[absPath_sizeMax];
			bool located = sAICallback->DataDirs_locatePath(skirmishAIId, absPath, absPath_sizeMax, (const char*) data, true, true, false, false);
			STRCPYS((char*)data, absPath_sizeMax, absPath);
			return located;
		}
		case AIVAL_UNIT_LIMIT: {
			*(int*) data = sAICallback->Unit_getLimit(skirmishAIId);
			return true;
		}
		case AIVAL_SCRIPT: {
			*(const char**) data = sAICallback->Game_getSetupScript(skirmishAIId);
			return true;
		}
		default:
			return false;
	}
}

int CAIAICallback::GetFileSize(const char* name) {
	return sAICallback->File_getSize(skirmishAIId, name);
}

int CAIAICallback::GetSelectedUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getSelectedUnits(skirmishAIId, unitIds, unitIds_max);
}

float3 CAIAICallback::GetMousePos() {

	float pos_cache[3];
	sAICallback->Map_getMousePos(skirmishAIId, pos_cache);
	return pos_cache;
}

int CAIAICallback::GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies) {

	const int numPoints = sAICallback->Map_getPoints(skirmishAIId, includeAllies);
	float pos_cache[3];
	short color_cache[3];
	for (int p=0; p < numPoints; ++p) {
		sAICallback->Map_Point_getPosition(skirmishAIId, p, pos_cache);
		pm[p].pos = pos_cache; // float[3] -> float3
		sAICallback->Map_Point_getColor(skirmishAIId, p, color_cache);
		unsigned char* color = (unsigned char*) calloc(3, sizeof(unsigned char));
		color[0] = (unsigned char) color_cache[0];
		color[1] = (unsigned char) color_cache[1];
		color[2] = (unsigned char) color_cache[2];
		pm[p].color = color;
		pm[p].label = sAICallback->Map_Point_getLabel(skirmishAIId, p);
	}

	return numPoints;
}

int CAIAICallback::GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies) {

	const int numLines = sAICallback->Map_getLines(skirmishAIId, includeAllies);
	float pos_cache[3];
	short color_cache[3];
	for (int l=0; l < numLines; ++l) {
		sAICallback->Map_Line_getFirstPosition(skirmishAIId, l, pos_cache);
		lm[l].pos = pos_cache; // float[3] -> float3
		sAICallback->Map_Line_getSecondPosition(skirmishAIId, l, pos_cache);
		lm[l].pos2 = pos_cache; // float[3] -> float3
		sAICallback->Map_Line_getColor(skirmishAIId, l, color_cache);
		unsigned char* color = (unsigned char*) calloc(3, sizeof(unsigned char));
		color[0] = (unsigned char) color_cache[0];
		color[1] = (unsigned char) color_cache[1];
		color[2] = (unsigned char) color_cache[2];
		lm[l].color = color;
	}

	return numLines;
}

float CAIAICallback::GetMetal() {
	int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Economy_getCurrent(skirmishAIId, m);
}

float CAIAICallback::GetMetalIncome() {
	int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Economy_getIncome(skirmishAIId, m);
}

float CAIAICallback::GetMetalUsage() {
	int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Economy_getUsage(skirmishAIId, m);
}

float CAIAICallback::GetMetalStorage() {
	int m = getResourceId_Metal(sAICallback, skirmishAIId);
	return sAICallback->Economy_getStorage(skirmishAIId, m);
}

float CAIAICallback::GetEnergy() {
	int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Economy_getCurrent(skirmishAIId, e);
}

float CAIAICallback::GetEnergyIncome() {
	int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Economy_getIncome(skirmishAIId, e);
}

float CAIAICallback::GetEnergyUsage() {
	int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Economy_getUsage(skirmishAIId, e);
}

float CAIAICallback::GetEnergyStorage() {
	int e = getResourceId_Energy(sAICallback, skirmishAIId);
	return sAICallback->Economy_getStorage(skirmishAIId, e);
}

int CAIAICallback::GetFeatures(int* featureIds, int featureIds_max) {
	return sAICallback->getFeatures(skirmishAIId, featureIds, featureIds_max);
}

int CAIAICallback::GetFeatures(int *featureIds, int featureIds_max, const float3& pos, float radius) {

	float aiPos[3];
	pos.copyInto(aiPos);
	return sAICallback->getFeaturesIn(skirmishAIId, aiPos, radius, featureIds, featureIds_max);
}

const FeatureDef* CAIAICallback::GetFeatureDef(int featureId) {
	int featureDefId = sAICallback->Feature_getDef(skirmishAIId, featureId);
	return this->GetFeatureDefById(featureDefId);
}

const FeatureDef* CAIAICallback::GetFeatureDefById(int featureDefId) {

	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);

	if (featureDefId < 0) {
		return NULL;
	}

	bool doRecreate = featureDefFrames[featureDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
	FeatureDef* featureDef = new FeatureDef();
featureDef->myName = sAICallback->FeatureDef_getName(skirmishAIId, featureDefId);
featureDef->description = sAICallback->FeatureDef_getDescription(skirmishAIId, featureDefId);
featureDef->filename = sAICallback->FeatureDef_getFileName(skirmishAIId, featureDefId);
//featureDef->id = sAICallback->FeatureDef_getId(skirmishAIId, featureDefId);
featureDef->id = featureDefId;
featureDef->metal = sAICallback->FeatureDef_getContainedResource(skirmishAIId, featureDefId, m);
featureDef->energy = sAICallback->FeatureDef_getContainedResource(skirmishAIId, featureDefId, e);
featureDef->maxHealth = sAICallback->FeatureDef_getMaxHealth(skirmishAIId, featureDefId);
featureDef->reclaimTime = sAICallback->FeatureDef_getReclaimTime(skirmishAIId, featureDefId);
featureDef->mass = sAICallback->FeatureDef_getMass(skirmishAIId, featureDefId);
featureDef->upright = sAICallback->FeatureDef_isUpright(skirmishAIId, featureDefId);
featureDef->drawType = sAICallback->FeatureDef_getDrawType(skirmishAIId, featureDefId);
featureDef->modelname = sAICallback->FeatureDef_getModelName(skirmishAIId, featureDefId);
featureDef->resurrectable = sAICallback->FeatureDef_getResurrectable(skirmishAIId, featureDefId);
featureDef->smokeTime = sAICallback->FeatureDef_getSmokeTime(skirmishAIId, featureDefId);
featureDef->destructable = sAICallback->FeatureDef_isDestructable(skirmishAIId, featureDefId);
featureDef->reclaimable = sAICallback->FeatureDef_isReclaimable(skirmishAIId, featureDefId);
featureDef->blocking = sAICallback->FeatureDef_isBlocking(skirmishAIId, featureDefId);
featureDef->burnable = sAICallback->FeatureDef_isBurnable(skirmishAIId, featureDefId);
featureDef->floating = sAICallback->FeatureDef_isFloating(skirmishAIId, featureDefId);
featureDef->noSelect = sAICallback->FeatureDef_isNoSelect(skirmishAIId, featureDefId);
featureDef->geoThermal = sAICallback->FeatureDef_isGeoThermal(skirmishAIId, featureDefId);
featureDef->deathFeature = sAICallback->FeatureDef_getDeathFeature(skirmishAIId, featureDefId);
featureDef->xsize = sAICallback->FeatureDef_getXSize(skirmishAIId, featureDefId);
featureDef->zsize = sAICallback->FeatureDef_getZSize(skirmishAIId, featureDefId);
{
	const int size = sAICallback->FeatureDef_getCustomParams(skirmishAIId, featureDefId, NULL, NULL);
	featureDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->FeatureDef_getCustomParams(skirmishAIId, featureDefId, cKeys, cValues);
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
	return sAICallback->Feature_getHealth(skirmishAIId, featureId);
}

float CAIAICallback::GetFeatureReclaimLeft(int featureId) {
	return sAICallback->Feature_getReclaimLeft(skirmishAIId, featureId);
}

float3 CAIAICallback::GetFeaturePos(int featureId) {
	
	float pos_cache[3];
	sAICallback->Feature_getPosition(skirmishAIId, featureId, pos_cache);
	return pos_cache;
}

int CAIAICallback::GetNumUnitDefs() {
	return sAICallback->getUnitDefs(skirmishAIId, NULL, NULL);
}

void CAIAICallback::GetUnitDefList(const UnitDef** list) {

	int size = sAICallback->getUnitDefs(skirmishAIId, NULL, NULL);
	int* unitDefIds = new int[size];

	// get actual number of IDs
	size = sAICallback->getUnitDefs(skirmishAIId, unitDefIds, size);

	for (int i = 0; i < size; ++i) {
		list[i] = this->GetUnitDefById(unitDefIds[i]);
	}

	delete[] unitDefIds;
}

float CAIAICallback::GetUnitDefHeight(int def) {
	return sAICallback->UnitDef_getHeight(skirmishAIId, def);
}

float CAIAICallback::GetUnitDefRadius(int def) {
	return sAICallback->UnitDef_getRadius(skirmishAIId, def);
}

const WeaponDef* CAIAICallback::GetWeapon(const char* weaponName) {
	int weaponDefId = sAICallback->getWeaponDefByName(skirmishAIId, weaponName);
	return this->GetWeaponDefById(weaponDefId);
}

const WeaponDef* CAIAICallback::GetWeaponDefById(int weaponDefId) {

	static int m = getResourceId_Metal(sAICallback, skirmishAIId);
	static int e = getResourceId_Energy(sAICallback, skirmishAIId);

//	logT("entering: GetWeaponDefById sAICallback");
	if (weaponDefId < 0) {
		return NULL;
	}

	bool doRecreate = weaponDefFrames[weaponDefId] < 0;
	if (doRecreate) {
//		int currentFrame = this->GetCurrentFrame();
		int currentFrame = 1;
//weaponDef->damages = sAICallback->WeaponDef_getDamages(skirmishAIId, weaponDefId);
//{
int numTypes = sAICallback->WeaponDef_Damage_getTypes(skirmishAIId, weaponDefId, NULL, 0);
//	logT("GetWeaponDefById 1");
//float* typeDamages = new float[numTypes];
float* typeDamages = (float*) calloc(numTypes, sizeof(float));
numTypes = sAICallback->WeaponDef_Damage_getTypes(skirmishAIId, weaponDefId, typeDamages, numTypes);
//	logT("GetWeaponDefById 2");
//for(int i=0; i < numTypes; ++i) {
//	typeDamages[i] = sAICallback->WeaponDef_Damages_getType(skirmishAIId, weaponDefId, i);
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
da.paralyzeDamageTime = sAICallback->WeaponDef_Damage_getParalyzeDamageTime(skirmishAIId, weaponDefId);
da.impulseFactor = sAICallback->WeaponDef_Damage_getImpulseFactor(skirmishAIId, weaponDefId);
da.impulseBoost = sAICallback->WeaponDef_Damage_getImpulseBoost(skirmishAIId, weaponDefId);
da.craterMult = sAICallback->WeaponDef_Damage_getCraterMult(skirmishAIId, weaponDefId);
da.craterBoost = sAICallback->WeaponDef_Damage_getCraterBoost(skirmishAIId, weaponDefId);
//	logT("GetWeaponDefById 4");
//}

	short color_cache[3];
	WeaponDef* weaponDef = new WeaponDef(da);
//	WeaponDef* weaponDef = new WeaponDef();
//	logT("GetWeaponDefById 5");
//	logI("GetWeaponDefById 5 defId: %d", weaponDefId);
weaponDef->name = sAICallback->WeaponDef_getName(skirmishAIId, weaponDefId);
weaponDef->type = sAICallback->WeaponDef_getType(skirmishAIId, weaponDefId);
weaponDef->description = sAICallback->WeaponDef_getDescription(skirmishAIId, weaponDefId);
weaponDef->filename = sAICallback->WeaponDef_getFileName(skirmishAIId, weaponDefId);
weaponDef->cegTag = sAICallback->WeaponDef_getCegTag(skirmishAIId, weaponDefId);
weaponDef->range = sAICallback->WeaponDef_getRange(skirmishAIId, weaponDefId);
weaponDef->heightmod = sAICallback->WeaponDef_getHeightMod(skirmishAIId, weaponDefId);
weaponDef->accuracy = sAICallback->WeaponDef_getAccuracy(skirmishAIId, weaponDefId);
weaponDef->sprayAngle = sAICallback->WeaponDef_getSprayAngle(skirmishAIId, weaponDefId);
weaponDef->movingAccuracy = sAICallback->WeaponDef_getMovingAccuracy(skirmishAIId, weaponDefId);
weaponDef->targetMoveError = sAICallback->WeaponDef_getTargetMoveError(skirmishAIId, weaponDefId);
weaponDef->leadLimit = sAICallback->WeaponDef_getLeadLimit(skirmishAIId, weaponDefId);
weaponDef->leadBonus = sAICallback->WeaponDef_getLeadBonus(skirmishAIId, weaponDefId);
weaponDef->predictBoost = sAICallback->WeaponDef_getPredictBoost(skirmishAIId, weaponDefId);
weaponDef->areaOfEffect = sAICallback->WeaponDef_getAreaOfEffect(skirmishAIId, weaponDefId);
weaponDef->noSelfDamage = sAICallback->WeaponDef_isNoSelfDamage(skirmishAIId, weaponDefId);
weaponDef->fireStarter = sAICallback->WeaponDef_getFireStarter(skirmishAIId, weaponDefId);
weaponDef->edgeEffectiveness = sAICallback->WeaponDef_getEdgeEffectiveness(skirmishAIId, weaponDefId);
weaponDef->size = sAICallback->WeaponDef_getSize(skirmishAIId, weaponDefId);
weaponDef->sizeGrowth = sAICallback->WeaponDef_getSizeGrowth(skirmishAIId, weaponDefId);
weaponDef->collisionSize = sAICallback->WeaponDef_getCollisionSize(skirmishAIId, weaponDefId);
weaponDef->salvosize = sAICallback->WeaponDef_getSalvoSize(skirmishAIId, weaponDefId);
weaponDef->salvodelay = sAICallback->WeaponDef_getSalvoDelay(skirmishAIId, weaponDefId);
weaponDef->reload = sAICallback->WeaponDef_getReload(skirmishAIId, weaponDefId);
weaponDef->beamtime = sAICallback->WeaponDef_getBeamTime(skirmishAIId, weaponDefId);
weaponDef->beamburst = sAICallback->WeaponDef_isBeamBurst(skirmishAIId, weaponDefId);
weaponDef->waterBounce = sAICallback->WeaponDef_isWaterBounce(skirmishAIId, weaponDefId);
weaponDef->groundBounce = sAICallback->WeaponDef_isGroundBounce(skirmishAIId, weaponDefId);
weaponDef->bounceRebound = sAICallback->WeaponDef_getBounceRebound(skirmishAIId, weaponDefId);
weaponDef->bounceSlip = sAICallback->WeaponDef_getBounceSlip(skirmishAIId, weaponDefId);
weaponDef->numBounce = sAICallback->WeaponDef_getNumBounce(skirmishAIId, weaponDefId);
weaponDef->maxAngle = sAICallback->WeaponDef_getMaxAngle(skirmishAIId, weaponDefId);
weaponDef->restTime = sAICallback->WeaponDef_getRestTime(skirmishAIId, weaponDefId);
weaponDef->uptime = sAICallback->WeaponDef_getUpTime(skirmishAIId, weaponDefId);
weaponDef->flighttime = sAICallback->WeaponDef_getFlightTime(skirmishAIId, weaponDefId);
weaponDef->metalcost = sAICallback->WeaponDef_getCost(skirmishAIId, weaponDefId, m);
weaponDef->energycost = sAICallback->WeaponDef_getCost(skirmishAIId, weaponDefId, e);
weaponDef->supplycost = sAICallback->WeaponDef_getSupplyCost(skirmishAIId, weaponDefId);
weaponDef->projectilespershot = sAICallback->WeaponDef_getProjectilesPerShot(skirmishAIId, weaponDefId);
//weaponDef->id = sAICallback->WeaponDef_getId(skirmishAIId, weaponDefId);
weaponDef->id = weaponDefId;
//weaponDef->tdfId = sAICallback->WeaponDef_getTdfId(skirmishAIId, weaponDefId);
weaponDef->tdfId = -1;
weaponDef->turret = sAICallback->WeaponDef_isTurret(skirmishAIId, weaponDefId);
weaponDef->onlyForward = sAICallback->WeaponDef_isOnlyForward(skirmishAIId, weaponDefId);
weaponDef->fixedLauncher = sAICallback->WeaponDef_isFixedLauncher(skirmishAIId, weaponDefId);
weaponDef->waterweapon = sAICallback->WeaponDef_isWaterWeapon(skirmishAIId, weaponDefId);
weaponDef->fireSubmersed = sAICallback->WeaponDef_isFireSubmersed(skirmishAIId, weaponDefId);
weaponDef->submissile = sAICallback->WeaponDef_isSubMissile(skirmishAIId, weaponDefId);
weaponDef->tracks = sAICallback->WeaponDef_isTracks(skirmishAIId, weaponDefId);
weaponDef->dropped = sAICallback->WeaponDef_isDropped(skirmishAIId, weaponDefId);
weaponDef->paralyzer = sAICallback->WeaponDef_isParalyzer(skirmishAIId, weaponDefId);
weaponDef->impactOnly = sAICallback->WeaponDef_isImpactOnly(skirmishAIId, weaponDefId);
weaponDef->noAutoTarget = sAICallback->WeaponDef_isNoAutoTarget(skirmishAIId, weaponDefId);
weaponDef->manualfire = sAICallback->WeaponDef_isManualFire(skirmishAIId, weaponDefId);
weaponDef->interceptor = sAICallback->WeaponDef_getInterceptor(skirmishAIId, weaponDefId);
weaponDef->targetable = sAICallback->WeaponDef_getTargetable(skirmishAIId, weaponDefId);
weaponDef->stockpile = sAICallback->WeaponDef_isStockpileable(skirmishAIId, weaponDefId);
weaponDef->coverageRange = sAICallback->WeaponDef_getCoverageRange(skirmishAIId, weaponDefId);
weaponDef->stockpileTime = sAICallback->WeaponDef_getStockpileTime(skirmishAIId, weaponDefId);
weaponDef->intensity = sAICallback->WeaponDef_getIntensity(skirmishAIId, weaponDefId);
weaponDef->thickness = sAICallback->WeaponDef_getThickness(skirmishAIId, weaponDefId);
weaponDef->laserflaresize = sAICallback->WeaponDef_getLaserFlareSize(skirmishAIId, weaponDefId);
weaponDef->corethickness = sAICallback->WeaponDef_getCoreThickness(skirmishAIId, weaponDefId);
weaponDef->duration = sAICallback->WeaponDef_getDuration(skirmishAIId, weaponDefId);
weaponDef->lodDistance = sAICallback->WeaponDef_getLodDistance(skirmishAIId, weaponDefId);
weaponDef->falloffRate = sAICallback->WeaponDef_getFalloffRate(skirmishAIId, weaponDefId);
weaponDef->graphicsType = sAICallback->WeaponDef_getGraphicsType(skirmishAIId, weaponDefId);
weaponDef->soundTrigger = sAICallback->WeaponDef_isSoundTrigger(skirmishAIId, weaponDefId);
weaponDef->selfExplode = sAICallback->WeaponDef_isSelfExplode(skirmishAIId, weaponDefId);
weaponDef->gravityAffected = sAICallback->WeaponDef_isGravityAffected(skirmishAIId, weaponDefId);
weaponDef->highTrajectory = sAICallback->WeaponDef_getHighTrajectory(skirmishAIId, weaponDefId);
weaponDef->myGravity = sAICallback->WeaponDef_getMyGravity(skirmishAIId, weaponDefId);
weaponDef->noExplode = sAICallback->WeaponDef_isNoExplode(skirmishAIId, weaponDefId);
weaponDef->startvelocity = sAICallback->WeaponDef_getStartVelocity(skirmishAIId, weaponDefId);
weaponDef->weaponacceleration = sAICallback->WeaponDef_getWeaponAcceleration(skirmishAIId, weaponDefId);
weaponDef->turnrate = sAICallback->WeaponDef_getTurnRate(skirmishAIId, weaponDefId);
weaponDef->maxvelocity = sAICallback->WeaponDef_getMaxVelocity(skirmishAIId, weaponDefId);
weaponDef->projectilespeed = sAICallback->WeaponDef_getProjectileSpeed(skirmishAIId, weaponDefId);
weaponDef->explosionSpeed = sAICallback->WeaponDef_getExplosionSpeed(skirmishAIId, weaponDefId);
weaponDef->onlyTargetCategory = sAICallback->WeaponDef_getOnlyTargetCategory(skirmishAIId, weaponDefId);
weaponDef->wobble = sAICallback->WeaponDef_getWobble(skirmishAIId, weaponDefId);
weaponDef->dance = sAICallback->WeaponDef_getDance(skirmishAIId, weaponDefId);
weaponDef->trajectoryHeight = sAICallback->WeaponDef_getTrajectoryHeight(skirmishAIId, weaponDefId);
weaponDef->largeBeamLaser = sAICallback->WeaponDef_isLargeBeamLaser(skirmishAIId, weaponDefId);
weaponDef->isShield = sAICallback->WeaponDef_isShield(skirmishAIId, weaponDefId);
weaponDef->shieldRepulser = sAICallback->WeaponDef_isShieldRepulser(skirmishAIId, weaponDefId);
weaponDef->smartShield = sAICallback->WeaponDef_isSmartShield(skirmishAIId, weaponDefId);
weaponDef->exteriorShield = sAICallback->WeaponDef_isExteriorShield(skirmishAIId, weaponDefId);
weaponDef->visibleShield = sAICallback->WeaponDef_isVisibleShield(skirmishAIId, weaponDefId);
weaponDef->visibleShieldRepulse = sAICallback->WeaponDef_isVisibleShieldRepulse(skirmishAIId, weaponDefId);
weaponDef->visibleShieldHitFrames = sAICallback->WeaponDef_getVisibleShieldHitFrames(skirmishAIId, weaponDefId);
weaponDef->shieldEnergyUse = sAICallback->WeaponDef_Shield_getResourceUse(skirmishAIId, weaponDefId, e);
weaponDef->shieldRadius = sAICallback->WeaponDef_Shield_getRadius(skirmishAIId, weaponDefId);
weaponDef->shieldForce = sAICallback->WeaponDef_Shield_getForce(skirmishAIId, weaponDefId);
weaponDef->shieldMaxSpeed = sAICallback->WeaponDef_Shield_getMaxSpeed(skirmishAIId, weaponDefId);
weaponDef->shieldPower = sAICallback->WeaponDef_Shield_getPower(skirmishAIId, weaponDefId);
weaponDef->shieldPowerRegen = sAICallback->WeaponDef_Shield_getPowerRegen(skirmishAIId, weaponDefId);
weaponDef->shieldPowerRegenEnergy = sAICallback->WeaponDef_Shield_getPowerRegenResource(skirmishAIId, weaponDefId, e);
weaponDef->shieldStartingPower = sAICallback->WeaponDef_Shield_getStartingPower(skirmishAIId, weaponDefId);
weaponDef->shieldRechargeDelay = sAICallback->WeaponDef_Shield_getRechargeDelay(skirmishAIId, weaponDefId);
sAICallback->WeaponDef_Shield_getGoodColor(skirmishAIId, weaponDefId, color_cache);
weaponDef->shieldGoodColor = float3((float)color_cache[0] / 256.0f, (float)color_cache[1] / 256.0f, (float)color_cache[2] / 256.0f);
sAICallback->WeaponDef_Shield_getBadColor(skirmishAIId, weaponDefId, color_cache);
weaponDef->shieldBadColor = float3((float)color_cache[0] / 256.0f, (float)color_cache[1] / 256.0f, (float)color_cache[2] / 256.0f);
weaponDef->shieldAlpha = sAICallback->WeaponDef_Shield_getAlpha(skirmishAIId, weaponDefId) / 256.0f;
weaponDef->shieldInterceptType = sAICallback->WeaponDef_Shield_getInterceptType(skirmishAIId, weaponDefId);
weaponDef->interceptedByShieldType = sAICallback->WeaponDef_getInterceptedByShieldType(skirmishAIId, weaponDefId);
weaponDef->avoidFriendly = sAICallback->WeaponDef_isAvoidFriendly(skirmishAIId, weaponDefId);
weaponDef->avoidFeature = sAICallback->WeaponDef_isAvoidFeature(skirmishAIId, weaponDefId);
weaponDef->avoidNeutral = sAICallback->WeaponDef_isAvoidNeutral(skirmishAIId, weaponDefId);
weaponDef->targetBorder = sAICallback->WeaponDef_getTargetBorder(skirmishAIId, weaponDefId);
weaponDef->cylinderTargetting = sAICallback->WeaponDef_getCylinderTargetting(skirmishAIId, weaponDefId);
weaponDef->minIntensity = sAICallback->WeaponDef_getMinIntensity(skirmishAIId, weaponDefId);
weaponDef->heightBoostFactor = sAICallback->WeaponDef_getHeightBoostFactor(skirmishAIId, weaponDefId);
weaponDef->proximityPriority = sAICallback->WeaponDef_getProximityPriority(skirmishAIId, weaponDefId);
weaponDef->collisionFlags = sAICallback->WeaponDef_getCollisionFlags(skirmishAIId, weaponDefId);
weaponDef->sweepFire = sAICallback->WeaponDef_isSweepFire(skirmishAIId, weaponDefId);
weaponDef->canAttackGround = sAICallback->WeaponDef_isAbleToAttackGround(skirmishAIId, weaponDefId);
weaponDef->cameraShake = sAICallback->WeaponDef_getCameraShake(skirmishAIId, weaponDefId);
weaponDef->dynDamageExp = sAICallback->WeaponDef_getDynDamageExp(skirmishAIId, weaponDefId);
weaponDef->dynDamageMin = sAICallback->WeaponDef_getDynDamageMin(skirmishAIId, weaponDefId);
weaponDef->dynDamageRange = sAICallback->WeaponDef_getDynDamageRange(skirmishAIId, weaponDefId);
weaponDef->dynDamageInverted = sAICallback->WeaponDef_isDynDamageInverted(skirmishAIId, weaponDefId);
{
	const int size = sAICallback->WeaponDef_getCustomParams(skirmishAIId, weaponDefId, NULL, NULL);
	weaponDef->customParams = std::map<std::string,std::string>();
	const char** cKeys = (const char**) calloc(size, sizeof(char*));
	const char** cValues = (const char**) calloc(size, sizeof(char*));
	sAICallback->WeaponDef_getCustomParams(skirmishAIId, weaponDefId, cKeys, cValues);
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
	sAICallback->Map_getStartPos(skirmishAIId, pos_cache);
	startPos = pos_cache;

	return &startPos;
}

unsigned int CAIAICallback::GetCategoryFlag(const char* categoryName) {
	return sAICallback->Game_getCategoryFlag(skirmishAIId, categoryName);
}

unsigned int CAIAICallback::GetCategoriesFlag(const char* categoryNames) {
	return sAICallback->Game_getCategoriesFlag(skirmishAIId, categoryNames);
}

void CAIAICallback::GetCategoryName(int categoryFlag, char* name, int name_sizeMax) {
	sAICallback->Game_getCategoryName(skirmishAIId, categoryFlag, name, name_sizeMax);
}







void CAIAICallback::SendTextMsg(const char* text, int zone) {

	SSendTextMessageCommand cmd = {text, zone};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &cmd);
}

void CAIAICallback::SetLastMsgPos(float3 pos) {

	float pos_f3[3];
	pos.copyInto(pos_f3);

	SSetLastPosMessageCommand cmd = {pos_f3};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &cmd);
}

void CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;
	const short alpha_s = (short) alpha * 256;

	SAddNotificationDrawerCommand cmd = {pos_f3, color_s3, alpha_s};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &cmd);
}

bool CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {

	SSendResourcesCommand cmd = {mAmount, eAmount, receivingTeam};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &cmd);
	return cmd.ret_isExecuted;
}

int CAIAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeam) {

	int* arr_unitIds = (int*) calloc(unitIds.size(), sizeof(int));
	for (size_t i=0; i < unitIds.size(); ++i) {
		arr_unitIds[i] = unitIds[i];
	}
	SSendUnitsCommand cmd = {arr_unitIds, unitIds.size(), receivingTeam};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &cmd);
	free(arr_unitIds);
	return cmd.ret_sentUnits;
}

void* CAIAICallback::CreateSharedMemArea(char* name, int size) {

	//SCreateSharedMemAreaCommand cmd = {name, size};
	//sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_CREATE, &cmd);
	//return cmd.ret_sharedMemArea;
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
	return NULL;
}

void CAIAICallback::ReleasedSharedMemArea(char* name) {

	//SReleaseSharedMemAreaCommand cmd = {name};
	//sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_RELEASE, &cmd);
	static const bool deprecatedMethod = true;
	assert(!deprecatedMethod);
}

int CAIAICallback::CreateGroup() {

	SCreateGroupCommand cmd = {};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &cmd);
	return cmd.ret_groupId;
}

void CAIAICallback::EraseGroup(int groupId) {

	SEraseGroupCommand cmd = {groupId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &cmd);
}

bool CAIAICallback::AddUnitToGroup(int unitId, int groupId) {

	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0, groupId};
	const int ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_ADD, &cmd);
	return (ret == 0);
}

bool CAIAICallback::RemoveUnitFromGroup(int unitId) {

	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0};
	const int ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_CLEAR, &cmd);
	return (ret == 0);
}

int CAIAICallback::GiveGroupOrder(int groupId, Command* c) {
	return this->Internal_GiveOrder(-1, groupId, c);
}

int CAIAICallback::GiveOrder(int unitId, Command* c) {
	return this->Internal_GiveOrder(unitId, -1, c);
}

int CAIAICallback::Internal_GiveOrder(int unitId, int groupId, Command* c) {

	const int maxUnits = sAICallback->Unit_getMax(skirmishAIId);

	int sCommandId;
	void* sCommandData = mallocSUnitCommand(unitId, groupId, c, &sCommandId, maxUnits);

	int ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, sCommandId, sCommandData);

	freeSUnitCommand(sCommandData, sCommandId);

	return ret;
}

int CAIAICallback::InitPath(float3 start, float3 end, int pathType, float goalRadius) {

	float start_f3[3];
	start.copyInto(start_f3);
	float end_f3[3];
	end.copyInto(end_f3);

	SInitPathCommand cmd = {start_f3, end_f3, pathType, goalRadius};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &cmd);
	return cmd.ret_pathId;
}

float3 CAIAICallback::GetNextWaypoint(int pathId) {

	float ret_posF3[3];
	SGetNextWaypointPathCommand cmd = {pathId, ret_posF3};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &cmd);
	return float3(cmd.ret_nextWaypoint_posF3_out);
}

float CAIAICallback::GetPathLength(float3 start, float3 end, int pathType, float goalRadius) {

	float start_f3[3];
	start.copyInto(start_f3);
	float end_f3[3];
	end.copyInto(end_f3);

	SGetApproximateLengthPathCommand cmd = {start_f3, end_f3, pathType, goalRadius};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &cmd); return cmd.ret_approximatePathLength;
}

void CAIAICallback::FreePath(int pathId) {

	SFreePathCommand cmd = {pathId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &cmd);
}

void CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;
	const short alpha = (short) color[3] * 256;

	SStartPathDrawerCommand cmd = {pos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &cmd);
}

void CAIAICallback::LineDrawerFinishPath() {

	SFinishPathDrawerCommand cmd = {};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &cmd);
}

void CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;
	const short alpha = (short) color[3] * 256;

	SDrawLinePathDrawerCommand cmd = {endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &cmd);
}

void CAIAICallback::LineDrawerDrawLineAndIcon(int cmdId, const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;
	const short alpha = (short) color[3] * 256;

	SDrawLineAndIconPathDrawerCommand cmd = {cmdId, endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &cmd);
}

void CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdId) {

	SDrawIconAtLastPosPathDrawerCommand cmd = {cmdId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &cmd);
}

void CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {

	float endPos_f3[3];
	endPos.copyInto(endPos_f3);
	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;
	const short alpha = (short) color[3] * 256;

	SBreakPathDrawerCommand cmd = {endPos_f3, color_s3, alpha};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &cmd);
}

void CAIAICallback::LineDrawerRestart() {

	SRestartPathDrawerCommand cmd = {false};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

void CAIAICallback::LineDrawerRestartSameColor() {

	SRestartPathDrawerCommand cmd = {true};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
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

	SCreateSplineFigureDrawerCommand cmd = {
		pos1_f3,
		pos2_f3,
		pos3_f3,
		pos4_f3,
		width,
		arrow,
		lifeTime,
		figureGroupId
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

int CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) {

	float pos1_f3[3];
	pos1.copyInto(pos1_f3);
	float pos2_f3[3];
	pos2.copyInto(pos2_f3);

	SCreateLineFigureDrawerCommand cmd = {
		pos1_f3,
		pos2_f3,
		width,
		arrow,
		lifeTime,
		figureGroupId
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

void CAIAICallback::SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) {

	short color_s3[3];
	color_s3[0] = (short) red * 256;
	color_s3[1] = (short) green * 256;
	color_s3[2] = (short) blue * 256;
	const short alpha_s = (short) alpha * 256;

	SSetColorFigureDrawerCommand cmd = {figureGroupId, color_s3, alpha_s};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &cmd);
}

void CAIAICallback::DeleteFigureGroup(int figureGroupId) {

	SDeleteFigureDrawerCommand cmd = {figureGroupId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &cmd);
}

void CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifeTime, int unitTeamId, bool transparent, bool drawBorder, int facing) {

	float pos_f3[3];
	pos.copyInto(pos_f3);
	SDrawUnitDrawerCommand cmd = {
		sAICallback->getUnitDefByName(skirmishAIId, name),
		pos_f3,
		rotation,
		lifeTime,
		unitTeamId,
		transparent,
		drawBorder,
		facing
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &cmd);
}



bool CAIAICallback::IsDebugDrawerEnabled() const {
	return sAICallback->Debug_GraphDrawer_isEnabled(skirmishAIId);
}

void CAIAICallback::DebugDrawerAddGraphPoint(int lineId, float x, float y) {
	SAddPointLineGraphDrawerDebugCommand cmd = {lineId, x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT, &cmd);
}
void CAIAICallback::DebugDrawerDelGraphPoints(int lineId, int numPoints) {
	SDeletePointsLineGraphDrawerDebugCommand cmd = {lineId, numPoints};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS, &cmd);
}

void CAIAICallback::DebugDrawerSetGraphPos(float x, float y) {
	SSetPositionGraphDrawerDebugCommand cmd = {x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_POS, &cmd);
}
void CAIAICallback::DebugDrawerSetGraphSize(float w, float h) {
	SSetSizeGraphDrawerDebugCommand cmd = {w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE, &cmd);
}
void CAIAICallback::DebugDrawerSetGraphLineColor(int lineId, const float3& color) {

	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;

	SSetColorLineGraphDrawerDebugCommand cmd = {lineId, color_s3};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR, &cmd);
}
void CAIAICallback::DebugDrawerSetGraphLineLabel(int lineId, const char* label) {
	SSetLabelLineGraphDrawerDebugCommand cmd = {lineId, label};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL, &cmd);
}

int CAIAICallback::DebugDrawerAddOverlayTexture(const float* texData, int w, int h) {
	SAddOverlayTextureDrawerDebugCommand cmd = {0, texData, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD, &cmd);
	return cmd.ret_overlayTextureId;
}
void CAIAICallback::DebugDrawerUpdateOverlayTexture(int overlayTextureId, const float* texData, int x, int y, int w, int h) {
	SUpdateOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, texData, x, y, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE, &cmd);
}
void CAIAICallback::DebugDrawerDelOverlayTexture(int overlayTextureId) {
	SDeleteOverlayTextureDrawerDebugCommand cmd = {overlayTextureId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE, &cmd);
}
void CAIAICallback::DebugDrawerSetOverlayTexturePos(int overlayTextureId, float x, float y) {
	SSetPositionOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS, &cmd);
}
void CAIAICallback::DebugDrawerSetOverlayTextureSize(int overlayTextureId, float w, float h) {
	SSetSizeOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE, &cmd);
}
void CAIAICallback::DebugDrawerSetOverlayTextureLabel(int overlayTextureId, const char* texLabel) {
	SSetLabelOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, texLabel};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL, &cmd);
}



int CAIAICallback::HandleCommand(int commandId, void* data) {
	int ret = -99;

	switch (commandId) {
		case AIHCQuerySubVersionId: {
//			SQuerySubVersionCommand cmd;
//			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, cmdTopicId, &cmd);
			ret = sAICallback->Game_getAiInterfaceVersion(skirmishAIId);
			break;
		}
		case AIHCAddMapPointId: {
			AIHCAddMapPoint* myData = (AIHCAddMapPoint*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SAddPointDrawCommand cmd = {pos, myData->label};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_ADD, &cmd);
			break;
		}
		case AIHCAddMapLineId: {
			AIHCAddMapLine* myData = (AIHCAddMapLine*) data;
			float posfrom[3];
			myData->posfrom.copyInto(posfrom);
			float posto[3];
			myData->posto.copyInto(posto);
			SAddLineDrawCommand cmd = {posfrom, posto};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_LINE_ADD, &cmd);
			break;
		}
		case AIHCRemoveMapPointId: {
			AIHCRemoveMapPoint* myData = (AIHCRemoveMapPoint*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SRemovePointDrawCommand cmd = {pos};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_REMOVE, &cmd);
			break;
		}
		case AIHCSendStartPosId: {
			AIHCSendStartPos* myData = (AIHCSendStartPos*) data;
			float pos[3];
			myData->pos.copyInto(pos);
			SSendStartPosCommand cmd = {myData->ready, pos};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_START_POS, &cmd);
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

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY, &cCmdData);

			myData->rayLen = cCmdData.rayLen;
			myData->hitUID = cCmdData.ret_hitUnitId;
			break;
		}

		case AIHCFeatureTraceRayId: {
			AIHCFeatureTraceRay* myData = (AIHCFeatureTraceRay*) data;
			float rayPos[3];
			myData->rayPos.copyInto(rayPos);
			float rayDir[3];
			myData->rayDir.copyInto(rayDir);
			SFeatureTraceRayCommand cCmdData = {
				rayPos,
				rayDir,
				myData->rayLen,
				myData->srcUID,
				myData->hitFID,
				myData->flags
			};

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_TRACE_RAY_FEATURE, &cCmdData);

			myData->rayLen = cCmdData.rayLen;
			myData->hitFID = cCmdData.ret_hitFeatureId;
			break;
		}

		case AIHCPauseId: {
			AIHCPause* cppCmdData = (AIHCPause*) data;
			SPauseCommand cCmdData = {
				cppCmdData->enable,
				cppCmdData->reason
			};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PAUSE, &cCmdData);
			break;
		}

		case AIHCGetDataDirId: {
			AIHCGetDataDir* cppCmdData = (AIHCGetDataDir*) data;
			cppCmdData->ret_path = sAICallback->DataDirs_allocatePath(
					skirmishAIId,
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

//	SReadFileCommand cmd = {name, buffer, bufferLen};
//	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_READ_FILE, &cmd); return cmd.ret_isExecuted;
	return sAICallback->File_getContent(skirmishAIId, filename, buffer, bufferLen);
}

const char* CAIAICallback::CallLuaRules(const char* data, int inSize, int* outSize) {

	SCallLuaRulesCommand cmd = {data, inSize};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_RULES, &cmd);
	*outSize = strlen(cmd.ret_outData);
	return cmd.ret_outData;
}

std::map<std::string, std::string> CAIAICallback::GetMyInfo()
{
	std::map<std::string, std::string> info;

	const int info_size = sAICallback->SkirmishAI_Info_getSize(skirmishAIId);
	for (int ii = 0; ii < info_size; ++ii) {
		const char* key   = sAICallback->SkirmishAI_Info_getKey(skirmishAIId, ii);
		const char* value = sAICallback->SkirmishAI_Info_getValue(skirmishAIId, ii);
		if ((key != NULL) && (value != NULL)) {
			info[key] = value;
		}
	}

	return info;
}

std::map<std::string, std::string> CAIAICallback::GetMyOptionValues()
{
	std::map<std::string, std::string> optionVals;

	const int optionVals_size = sAICallback->SkirmishAI_OptionValues_getSize(skirmishAIId);
	for (int ovi = 0; ovi < optionVals_size; ++ovi) {
		const char* key   = sAICallback->SkirmishAI_OptionValues_getKey(skirmishAIId, ovi);
		const char* value = sAICallback->SkirmishAI_OptionValues_getValue(skirmishAIId, ovi);
		if ((key != NULL) && (value != NULL)) {
			optionVals[key] = value;
		}
	}

	return optionVals;
}
