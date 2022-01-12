/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIAICallback.h"

#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "System/StringUtil.h"

// engine copies, so we do not have to adjust
// to each minor change done there
#include "MoveData.h"
#include "CommandQueue.h"

#include <string>
#include <cassert>

// strcpy
#include "string.h"

#define  METAL_RES_INDEX 0
#define ENERGY_RES_INDEX 1
#define  METAL_RES_IDENT (getResourceId_Metal(sAICallback, skirmishAIId))
#define ENERGY_RES_IDENT (getResourceId_Energy(sAICallback, skirmishAIId))

static int resourceIds[2] = {-1, -1};

static inline int getResourceId_Metal(const SSkirmishAICallback* sAICallback, int skirmishAIId) {
	if (resourceIds[METAL_RES_INDEX] == -1)
		resourceIds[METAL_RES_INDEX] = sAICallback->getResourceByName(skirmishAIId, "Metal");

	return resourceIds[METAL_RES_INDEX];
}

static inline int getResourceId_Energy(const SSkirmishAICallback* sAICallback, int skirmishAIId) {
	if (resourceIds[ENERGY_RES_INDEX] == -1)
		resourceIds[ENERGY_RES_INDEX] = sAICallback->getResourceByName(skirmishAIId, "Energy");

	return resourceIds[ENERGY_RES_INDEX];
}



template<typename SrcType, typename DstType>
static inline void CopyArray(const SrcType* src, DstType* dst, const size_t size)
{
	for (size_t n = 0; n < size; n++) {
		dst[n] = static_cast<DstType>(src[n]);
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


size_t springLegacyAI::CAIAICallback::numClbInstances = 0;

std::vector<float> springLegacyAI::CAIAICallback::heightMap;
std::vector<float> springLegacyAI::CAIAICallback::cornersHeightMap;
std::vector<float> springLegacyAI::CAIAICallback::slopeMap;
std::vector<unsigned short> springLegacyAI::CAIAICallback::losMap;
std::vector<unsigned short> springLegacyAI::CAIAICallback::radarMap;
std::vector<unsigned short> springLegacyAI::CAIAICallback::jammerMap;
std::vector<unsigned char> springLegacyAI::CAIAICallback::metalMap;


springLegacyAI::CAIAICallback::CAIAICallback():
	IAICallback(),
	skirmishAIId(-1),
	sAICallback(nullptr)
{
	init();
}

springLegacyAI::CAIAICallback::CAIAICallback(int skirmishAIId, const SSkirmishAICallback* sAICallback):
	IAICallback(),
	skirmishAIId(skirmishAIId),
	sAICallback(sAICallback)
{
	init();
}

springLegacyAI::CAIAICallback::~CAIAICallback() {
	numClbInstances--;

	unitDefs.clear();
	featureDefs.clear();
	weaponDefs.clear();
	unitDefFrames.clear();
	featureDefFrames.clear();
	weaponDefFrames.clear();

	groupPossibleCommands.clear();
	unitPossibleCommands.clear();
	unitCurrentCommandQueues.clear();

	if (numClbInstances == 0) {
		heightMap.clear();
		cornersHeightMap.clear();
		slopeMap.clear();
		losMap.clear();
		radarMap.clear();
		jammerMap.clear();
		metalMap.clear();
	}
}


void springLegacyAI::CAIAICallback::init() {
	numClbInstances++;

	unitDefs.resize(numUnitDefs);
	featureDefs.resize(numFeatDefs);
	weaponDefs.resize(numWeapDefs);

	unitDefFrames.resize(numUnitDefs, -1);
	featureDefFrames.resize(numFeatDefs, -1);
	weaponDefFrames.resize(numWeapDefs, -1);

	groupPossibleCommands.resize(maxGroups);
	unitPossibleCommands.resize(MAX_UNITS);
	unitCurrentCommandQueues.resize(MAX_UNITS);
}


bool springLegacyAI::CAIAICallback::PosInCamera(float3 pos, float radius) {
	return sAICallback->Map_isPosInCamera(skirmishAIId, &pos[0], radius);
}

int springLegacyAI::CAIAICallback::GetCurrentFrame() {
	return sAICallback->Game_getCurrentFrame(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetMySkirmishAIId() {
	return skirmishAIId;
}

int springLegacyAI::CAIAICallback::GetMyTeam() {
	return sAICallback->Game_getMyTeam(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetMyAllyTeam() {
	return sAICallback->Game_getMyAllyTeam(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetPlayerTeam(int player) {
	return sAICallback->Game_getPlayerTeam(skirmishAIId, player);
}

int springLegacyAI::CAIAICallback::GetTeams() {
	return sAICallback->Game_getTeams(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetTeamSide(int otherTeamId) {
	return sAICallback->Game_getTeamSide(skirmishAIId, otherTeamId);
}

int springLegacyAI::CAIAICallback::GetTeamAllyTeam(int otherTeamId) {
	return sAICallback->Game_getTeamAllyTeam(skirmishAIId, otherTeamId);
}

float springLegacyAI::CAIAICallback::GetTeamMetalCurrent(int otherTeamId) {
	return sAICallback->Game_getTeamResourceCurrent(skirmishAIId, otherTeamId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamMetalIncome(int otherTeamId) {
	return sAICallback->Game_getTeamResourceIncome(skirmishAIId, otherTeamId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamMetalUsage(int otherTeamId) {
	return sAICallback->Game_getTeamResourceUsage(skirmishAIId, otherTeamId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamMetalStorage(int otherTeamId) {
	return sAICallback->Game_getTeamResourceStorage(skirmishAIId, otherTeamId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamEnergyCurrent(int otherTeamId) {
	return sAICallback->Game_getTeamResourceCurrent(skirmishAIId, otherTeamId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamEnergyIncome(int otherTeamId) {
	return sAICallback->Game_getTeamResourceIncome(skirmishAIId, otherTeamId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamEnergyUsage(int otherTeamId) {
	return sAICallback->Game_getTeamResourceUsage(skirmishAIId, otherTeamId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetTeamEnergyStorage(int otherTeamId) {
	return sAICallback->Game_getTeamResourceStorage(skirmishAIId, otherTeamId, ENERGY_RES_IDENT);
}

bool springLegacyAI::CAIAICallback::IsAllied(int firstAllyTeamId, int secondAllyTeamId) {
	return sAICallback->Game_isAllied(skirmishAIId, firstAllyTeamId, secondAllyTeamId);
}

int springLegacyAI::CAIAICallback::GetUnitGroup(int unitId) {
	return sAICallback->Unit_getGroup(skirmishAIId, unitId);
}

const std::vector<SCommandDescription>* springLegacyAI::CAIAICallback::GetGroupCommands(int groupId)
{
	const int numCmds = sAICallback->Group_getSupportedCommands(skirmishAIId, groupId);

	std::vector<SCommandDescription>* cmdDescVec = &groupPossibleCommands[groupId];
	cmdDescVec->clear();

	for (int c = 0; c < numCmds; c++) {
		SCommandDescription commandDescription;
		commandDescription.id = sAICallback->Group_SupportedCommand_getId(skirmishAIId, groupId, c);
		commandDescription.name = sAICallback->Group_SupportedCommand_getName(skirmishAIId, groupId, c);
		commandDescription.tooltip = sAICallback->Group_SupportedCommand_getToolTip(skirmishAIId, groupId, c);
		commandDescription.showUnique = sAICallback->Group_SupportedCommand_isShowUnique(skirmishAIId, groupId, c);
		commandDescription.disabled = sAICallback->Group_SupportedCommand_isDisabled(skirmishAIId, groupId, c);

		std::vector<const char*> params(sAICallback->Group_SupportedCommand_getParams(skirmishAIId, groupId, c, nullptr, 0), "");

		if (!params.empty()) {
			const int numParams = sAICallback->Group_SupportedCommand_getParams(skirmishAIId, groupId, c, &params[0], params.size());

			for (int p = 0; p < numParams; p++) {
				commandDescription.params.push_back(params[p]);
			}
		}

		cmdDescVec->push_back(commandDescription);
	}

	return cmdDescVec;
}


const std::vector<SCommandDescription>* springLegacyAI::CAIAICallback::GetUnitCommands(int unitId)
{
	const int numCmds = sAICallback->Unit_getSupportedCommands(skirmishAIId, unitId);

	std::vector<SCommandDescription>* cmdDescVec = &unitPossibleCommands[unitId];
	cmdDescVec->clear();

	for (int c = 0; c < numCmds; c++) {
		SCommandDescription commandDescription;
		commandDescription.id = sAICallback->Unit_SupportedCommand_getId(skirmishAIId, unitId, c);
		commandDescription.name = sAICallback->Unit_SupportedCommand_getName(skirmishAIId, unitId, c);
		commandDescription.tooltip = sAICallback->Unit_SupportedCommand_getToolTip(skirmishAIId, unitId, c);
		commandDescription.showUnique = sAICallback->Unit_SupportedCommand_isShowUnique(skirmishAIId, unitId, c);
		commandDescription.disabled = sAICallback->Unit_SupportedCommand_isDisabled(skirmishAIId, unitId, c);

		std::vector<const char*> params(sAICallback->Unit_SupportedCommand_getParams(skirmishAIId, unitId, c, nullptr, 0), "");

		if (!params.empty()) {
			const int numParams = sAICallback->Unit_SupportedCommand_getParams(skirmishAIId, unitId, c, &params[0], params.size());

			for (int p = 0; p < numParams; p++) {
				commandDescription.params.push_back(params[p]);
			}
		}

		cmdDescVec->push_back(commandDescription);
	}

	return cmdDescVec;
}

const springLegacyAI::CCommandQueue* springLegacyAI::CAIAICallback::GetCurrentUnitCommands(int unitId)
{
	const int numCmds = sAICallback->Unit_getCurrentCommands(skirmishAIId, unitId);
	const int type = sAICallback->Unit_CurrentCommand_getType(skirmishAIId, unitId);

	if (unitCurrentCommandQueues[unitId].get() == nullptr)
		unitCurrentCommandQueues[unitId].reset(new CCommandQueue());

	CCommandQueue* cc = unitCurrentCommandQueues[unitId].get();
	cc->clear();
	cc->queueType = (CCommandQueue::QueueType) type;

	for (int c = 0; c < numCmds; c++) {
		const int cmd_id            = sAICallback->Unit_CurrentCommand_getId(skirmishAIId, unitId, c);
		const unsigned char cmd_opt = sAICallback->Unit_CurrentCommand_getOptions(skirmishAIId, unitId, c);

		Command command(cmd_id, cmd_opt);
		command.SetTag(sAICallback->Unit_CurrentCommand_getTag(skirmishAIId, unitId, c));
		command.SetTimeOut(sAICallback->Unit_CurrentCommand_getTimeOut(skirmishAIId, unitId, c));

		std::vector<float> params(sAICallback->Unit_CurrentCommand_getParams(skirmishAIId, unitId, c, nullptr, 0));

		if (!params.empty()) {
			const int numParams = sAICallback->Unit_CurrentCommand_getParams(skirmishAIId, unitId, c, &params[0], params.size());

			for (int p = 0; p < numParams; p++) {
				command.PushParam(params[p]);
			}
		}

		cc->push_back(command);
	}

	return cc;
}



int springLegacyAI::CAIAICallback::GetMaxUnits() {
	return sAICallback->Unit_getMax(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetUnitTeam(int unitId) {
	return sAICallback->Unit_getTeam(skirmishAIId, unitId);
}

int springLegacyAI::CAIAICallback::GetUnitAllyTeam(int unitId) {
	return sAICallback->Unit_getAllyTeam(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitHealth(int unitId) {
	return sAICallback->Unit_getHealth(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitMaxHealth(int unitId) {
	return sAICallback->Unit_getMaxHealth(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitSpeed(int unitId) {
	return sAICallback->Unit_getSpeed(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitPower(int unitId) {
	return sAICallback->Unit_getPower(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitExperience(int unitId) {
	return sAICallback->Unit_getExperience(skirmishAIId, unitId);
}

float springLegacyAI::CAIAICallback::GetUnitMaxRange(int unitId) {
	return sAICallback->Unit_getMaxRange(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::IsUnitActivated(int unitId) {
	return sAICallback->Unit_isActivated(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::UnitBeingBuilt(int unitId) {
	return sAICallback->Unit_isBeingBuilt(skirmishAIId, unitId);
}

const springLegacyAI::UnitDef* springLegacyAI::CAIAICallback::GetUnitDef(int unitId) {
	return GetUnitDefById(sAICallback->Unit_getDef(skirmishAIId, unitId));
}



float3 springLegacyAI::CAIAICallback::GetUnitPos(int unitId) {
	float3 pos;
	sAICallback->Unit_getPos(skirmishAIId, unitId, &pos[0]);
	return pos;
}

float3 springLegacyAI::CAIAICallback::GetUnitVel(int unitId) {
	float3 vel;
	sAICallback->Unit_getVel(skirmishAIId, unitId, &vel[0]);
	return vel;
}



int springLegacyAI::CAIAICallback::GetBuildingFacing(int unitId) {
	return sAICallback->Unit_getBuildingFacing(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::IsUnitCloaked(int unitId) {
	return sAICallback->Unit_isCloaked(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::IsUnitParalyzed(int unitId) {
	return sAICallback->Unit_isParalyzed(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::IsUnitNeutral(int unitId) {
	return sAICallback->Unit_isNeutral(skirmishAIId, unitId);
}

bool springLegacyAI::CAIAICallback::GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo) {

	resourceInfo->energyMake = sAICallback->Unit_getResourceMake(skirmishAIId, unitId, ENERGY_RES_IDENT);

	if (resourceInfo->energyMake < 0.0f)
		return false;

	resourceInfo->energyUse = sAICallback->Unit_getResourceUse(skirmishAIId, unitId, ENERGY_RES_IDENT);
	resourceInfo->metalMake = sAICallback->Unit_getResourceMake(skirmishAIId, unitId, METAL_RES_IDENT);
	resourceInfo->metalUse = sAICallback->Unit_getResourceUse(skirmishAIId, unitId, METAL_RES_IDENT);
	return true;
}

const springLegacyAI::UnitDef* springLegacyAI::CAIAICallback::GetUnitDef(const char* unitName) {
	return GetUnitDefById(sAICallback->getUnitDefByName(skirmishAIId, unitName));
}


const springLegacyAI::UnitDef* springLegacyAI::CAIAICallback::GetUnitDefById(int unitDefId)
{
	if (unitDefId < 0)
		return nullptr;

	if (unitDefFrames[unitDefId] < 0) {
		const int currentFrame = 1; // GetCurrentFrame();

		UnitDef* unitDef = &unitDefs[unitDefId];

		unitDef->name = sAICallback->UnitDef_getName(skirmishAIId, unitDefId);
		unitDef->humanName = sAICallback->UnitDef_getHumanName(skirmishAIId, unitDefId);
		//unitDef->id = sAICallback->UnitDef_getId(skirmishAIId, unitDefId);
		unitDef->id = unitDefId;

		unitDef->metalUpkeep = sAICallback->UnitDef_getUpkeep(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->energyUpkeep = sAICallback->UnitDef_getUpkeep(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
		unitDef->metalMake = sAICallback->UnitDef_getResourceMake(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->makesMetal = sAICallback->UnitDef_getMakesResource(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->energyMake = sAICallback->UnitDef_getResourceMake(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
		unitDef->metalCost = sAICallback->UnitDef_getCost(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->energyCost = sAICallback->UnitDef_getCost(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
		unitDef->buildTime = sAICallback->UnitDef_getBuildTime(skirmishAIId, unitDefId);
		unitDef->extractsMetal = sAICallback->UnitDef_getExtractsResource(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->extractRange = sAICallback->UnitDef_getResourceExtractorRange(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->windGenerator = sAICallback->UnitDef_getWindResourceGenerator(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
		unitDef->tidalGenerator = sAICallback->UnitDef_getTidalResourceGenerator(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
		unitDef->metalStorage = sAICallback->UnitDef_getStorage(skirmishAIId, unitDefId, METAL_RES_IDENT);
		unitDef->energyStorage = sAICallback->UnitDef_getStorage(skirmishAIId, unitDefId, ENERGY_RES_IDENT);
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
		unitDef->maxHeightDif = sAICallback->UnitDef_getMaxHeightDif(skirmishAIId, unitDefId);
		unitDef->minWaterDepth = sAICallback->UnitDef_getMinWaterDepth(skirmishAIId, unitDefId);
		unitDef->waterline = sAICallback->UnitDef_getWaterline(skirmishAIId, unitDefId);
		unitDef->maxWaterDepth = sAICallback->UnitDef_getMaxWaterDepth(skirmishAIId, unitDefId);
		unitDef->armoredMultiple = sAICallback->UnitDef_getArmoredMultiple(skirmishAIId, unitDefId);
		unitDef->armorType = sAICallback->UnitDef_getArmorType(skirmishAIId, unitDefId);
		unitDef->flankingBonusMode = sAICallback->UnitDef_FlankingBonus_getMode(skirmishAIId, unitDefId);

		sAICallback->UnitDef_FlankingBonus_getDir(skirmishAIId, unitDefId, &unitDef->flankingBonusDir[0]);

		unitDef->flankingBonusMax = sAICallback->UnitDef_FlankingBonus_getMax(skirmishAIId, unitDefId);
		unitDef->flankingBonusMin = sAICallback->UnitDef_FlankingBonus_getMin(skirmishAIId, unitDefId);
		unitDef->flankingBonusMobilityAdd = sAICallback->UnitDef_FlankingBonus_getMobilityAdd(skirmishAIId, unitDefId);

		unitDef->maxWeaponRange = sAICallback->UnitDef_getMaxWeaponRange(skirmishAIId, unitDefId);
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
		unitDef->frontToSpeed = sAICallback->UnitDef_getFrontToSpeed(skirmishAIId, unitDefId);
		unitDef->speedToFront = sAICallback->UnitDef_getSpeedToFront(skirmishAIId, unitDefId);
		unitDef->myGravity = sAICallback->UnitDef_getMyGravity(skirmishAIId, unitDefId);
		unitDef->maxBank = sAICallback->UnitDef_getMaxBank(skirmishAIId, unitDefId);
		unitDef->maxPitch = sAICallback->UnitDef_getMaxPitch(skirmishAIId, unitDefId);
		unitDef->turnRadius = sAICallback->UnitDef_getTurnRadius(skirmishAIId, unitDefId);
		unitDef->wantedHeight = sAICallback->UnitDef_getWantedHeight(skirmishAIId, unitDefId);
		unitDef->verticalSpeed = sAICallback->UnitDef_getVerticalSpeed(skirmishAIId, unitDefId);

		unitDef->hoverAttack = sAICallback->UnitDef_isHoverAttack(skirmishAIId, unitDefId);
		unitDef->airStrafe = sAICallback->UnitDef_isAirStrafe(skirmishAIId, unitDefId);

		unitDef->dlHoverFactor = sAICallback->UnitDef_getDlHoverFactor(skirmishAIId, unitDefId);
		unitDef->maxAcc = sAICallback->UnitDef_getMaxAcceleration(skirmishAIId, unitDefId);
		unitDef->maxDec = sAICallback->UnitDef_getMaxDeceleration(skirmishAIId, unitDefId);
		unitDef->maxAileron = sAICallback->UnitDef_getMaxAileron(skirmishAIId, unitDefId);
		unitDef->maxElevator = sAICallback->UnitDef_getMaxElevator(skirmishAIId, unitDefId);
		unitDef->maxRudder = sAICallback->UnitDef_getMaxRudder(skirmishAIId, unitDefId);

		{
			const size_t NUM_FACINGS = sizeof(unitDef->yardmaps) / sizeof(unitDef->yardmaps[0]);
			const int yardMapSize = sAICallback->UnitDef_getYardMap(skirmishAIId, unitDefId, 0, nullptr, 0);

			if (yardMapSize > 0) {
				std::vector<short> tmpYardMap(yardMapSize);

				for (int ym = 0; ym < NUM_FACINGS; ++ym) {
					sAICallback->UnitDef_getYardMap(skirmishAIId, unitDefId, ym, &tmpYardMap[0], tmpYardMap.size());
					unitDef->yardmaps[ym].resize(tmpYardMap.size());

					for (int i = 0; i < tmpYardMap.size(); ++i) {
						unitDef->yardmaps[ym][i] = (unsigned char) tmpYardMap[i];
					}
				}
			}
		}

		unitDef->xsize = sAICallback->UnitDef_getXSize(skirmishAIId, unitDefId);
		unitDef->zsize = sAICallback->UnitDef_getZSize(skirmishAIId, unitDefId);

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
		unitDef->canDGun = sAICallback->UnitDef_canManualFire(skirmishAIId, unitDefId);
		unitDef->needGeo = sAICallback->UnitDef_isNeedGeo(skirmishAIId, unitDefId);
		unitDef->isFeature = sAICallback->UnitDef_isFeature(skirmishAIId, unitDefId);
		unitDef->hideDamage = sAICallback->UnitDef_isHideDamage(skirmishAIId, unitDefId);
		unitDef->showPlayerName = sAICallback->UnitDef_isShowPlayerName(skirmishAIId, unitDefId);
		unitDef->canResurrect = sAICallback->UnitDef_isAbleToResurrect(skirmishAIId, unitDefId);
		unitDef->canCapture = sAICallback->UnitDef_isAbleToCapture(skirmishAIId, unitDefId);
		unitDef->highTrajectoryType = sAICallback->UnitDef_getHighTrajectoryType(skirmishAIId, unitDefId);
		unitDef->noChaseCategory = sAICallback->UnitDef_getNoChaseCategory(skirmishAIId, unitDefId);
		unitDef->canDropFlare = sAICallback->UnitDef_isAbleToDropFlare(skirmishAIId, unitDefId);
		unitDef->flareReloadTime = sAICallback->UnitDef_getFlareReloadTime(skirmishAIId, unitDefId);
		unitDef->flareEfficiency = sAICallback->UnitDef_getFlareEfficiency(skirmishAIId, unitDefId);
		unitDef->flareDelay = sAICallback->UnitDef_getFlareDelay(skirmishAIId, unitDefId);

		sAICallback->UnitDef_getFlareDropVector(skirmishAIId, unitDefId, &unitDef->flareDropVector[0]);

		unitDef->flareTime = sAICallback->UnitDef_getFlareTime(skirmishAIId, unitDefId);
		unitDef->flareSalvoSize = sAICallback->UnitDef_getFlareSalvoSize(skirmishAIId, unitDefId);
		unitDef->flareSalvoDelay = sAICallback->UnitDef_getFlareSalvoDelay(skirmishAIId, unitDefId);
		unitDef->smoothAnim = false;
		unitDef->canLoopbackAttack = sAICallback->UnitDef_isAbleToLoopbackAttack(skirmishAIId, unitDefId);
		unitDef->levelGround = sAICallback->UnitDef_isLevelGround(skirmishAIId, unitDefId);

		unitDef->isFirePlatform = sAICallback->UnitDef_isFirePlatform(skirmishAIId, unitDefId);
		unitDef->maxThisUnit = sAICallback->UnitDef_getMaxThisUnit(skirmishAIId, unitDefId);
		//unitDef->decoyDef = sAICallback->UnitDef_getDecoyDefId(skirmishAIId, unitDefId);
		unitDef->shieldWeaponDef = GetWeaponDefById(sAICallback->UnitDef_getShieldDef(skirmishAIId, unitDefId));
		unitDef->stockpileWeaponDef = GetWeaponDefById(sAICallback->UnitDef_getStockpileDef(skirmishAIId, unitDefId));

		{
			std::vector<int> buildOpts(sAICallback->UnitDef_getBuildOptions(skirmishAIId, unitDefId, nullptr, 0));

			if (!buildOpts.empty()) {
				const int numBuildOpts = sAICallback->UnitDef_getBuildOptions(skirmishAIId, unitDefId, &buildOpts[0], buildOpts.size());

				for (int b = 0; b < numBuildOpts; b++) {
					unitDef->buildOptions[b] = sAICallback->UnitDef_getName(skirmishAIId, buildOpts[b]);
				}
			}
		}
		{
			const int numParams = sAICallback->UnitDef_getCustomParams(skirmishAIId, unitDefId, nullptr, nullptr);

			if (numParams > 0) {
				std::vector<const char*> cKeys(numParams, "");
				std::vector<const char*> cValues(numParams, "");

				sAICallback->UnitDef_getCustomParams(skirmishAIId, unitDefId, &cKeys[0], &cValues[0]);

				for (int i = 0; i < numParams; ++i) {
					unitDef->customParams[cKeys[i]] = cValues[i];
				}
			}
		}

		if (sAICallback->UnitDef_isMoveDataAvailable(skirmishAIId, unitDefId)) {
			unitDef->movedata = new MoveData();

			unitDef->movedata->xsize = sAICallback->UnitDef_MoveData_getXSize(skirmishAIId, unitDefId);
			unitDef->movedata->zsize = sAICallback->UnitDef_MoveData_getZSize(skirmishAIId, unitDefId);
			unitDef->movedata->depth = sAICallback->UnitDef_MoveData_getDepth(skirmishAIId, unitDefId);
			unitDef->movedata->maxSlope = sAICallback->UnitDef_MoveData_getMaxSlope(skirmishAIId, unitDefId);
			unitDef->movedata->slopeMod = sAICallback->UnitDef_MoveData_getSlopeMod(skirmishAIId, unitDefId);
			// NOTE: If there will be more lambdas then it may be better to pass _this_ pointer into MoveData constructor
			unitDef->movedata->GetDepthMod = [this, unitDefId](float height) {
				return sAICallback->UnitDef_MoveData_getDepthMod(skirmishAIId, unitDefId, height);
			};
			unitDef->movedata->pathType = sAICallback->UnitDef_MoveData_getPathType(skirmishAIId, unitDefId);
			unitDef->movedata->crushStrength = sAICallback->UnitDef_MoveData_getCrushStrength(skirmishAIId, unitDefId);
			unitDef->movedata->moveFamily = (enum MoveData::MoveFamily) sAICallback->UnitDef_MoveData_getSpeedModClass(skirmishAIId, unitDefId);
			unitDef->movedata->terrainClass = (enum MoveData::TerrainClass) sAICallback->UnitDef_MoveData_getTerrainClass(skirmishAIId, unitDefId);

			unitDef->movedata->followGround = sAICallback->UnitDef_MoveData_getFollowGround(skirmishAIId, unitDefId);
			unitDef->movedata->subMarine = sAICallback->UnitDef_MoveData_isSubMarine(skirmishAIId, unitDefId);
			unitDef->movedata->name = sAICallback->UnitDef_MoveData_getName(skirmishAIId, unitDefId);
		} else {
			unitDef->movedata = nullptr;
		}

		const int numWeapons = sAICallback->UnitDef_getWeaponMounts(skirmishAIId, unitDefId);

		for (int w = 0; w < numWeapons; ++w) {
			unitDef->weapons.push_back(UnitDef::UnitDefWeapon());
			unitDef->weapons[w].name = sAICallback->UnitDef_WeaponMount_getName(skirmishAIId, unitDefId, w);

			unitDef->weapons[w].def = GetWeaponDefById(sAICallback->UnitDef_WeaponMount_getWeaponDef(skirmishAIId, unitDefId, w));
			unitDef->weapons[w].slavedTo = sAICallback->UnitDef_WeaponMount_getSlavedTo(skirmishAIId, unitDefId, w);

			sAICallback->UnitDef_WeaponMount_getMainDir(skirmishAIId, unitDefId, w, &unitDef->weapons[w].mainDir[0]);

			unitDef->weapons[w].maxAngleDif = sAICallback->UnitDef_WeaponMount_getMaxAngleDif(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].badTargetCat = sAICallback->UnitDef_WeaponMount_getBadTargetCategory(skirmishAIId, unitDefId, w);
			unitDef->weapons[w].onlyTargetCat = sAICallback->UnitDef_WeaponMount_getOnlyTargetCategory(skirmishAIId, unitDefId, w);
		}

		unitDefFrames[unitDefId] = currentFrame;
	}

	return &unitDefs[unitDefId];
}


int springLegacyAI::CAIAICallback::GetEnemyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getEnemyUnits(skirmishAIId, unitIds, unitIds_max);
}

int springLegacyAI::CAIAICallback::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	float3 cpyPos = pos;
	return sAICallback->getEnemyUnitsIn(skirmishAIId, &cpyPos[0], radius, unitIds, unitIds_max);
}


int springLegacyAI::CAIAICallback::GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max) {
	return sAICallback->getEnemyUnitsInRadarAndLos(skirmishAIId, unitIds, unitIds_max);
}


int springLegacyAI::CAIAICallback::GetFriendlyUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getFriendlyUnits(skirmishAIId, unitIds, unitIds_max);
}

int springLegacyAI::CAIAICallback::GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	float3 cpyPos = pos;
	return sAICallback->getFriendlyUnitsIn(skirmishAIId, &cpyPos[0], radius, unitIds, unitIds_max);
}


int springLegacyAI::CAIAICallback::GetNeutralUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getNeutralUnits(skirmishAIId, unitIds, unitIds_max);
}

int springLegacyAI::CAIAICallback::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max) {
	float3 cpyPos = pos;
	return sAICallback->getNeutralUnitsIn(skirmishAIId, &cpyPos[0], radius, unitIds, unitIds_max);
}


int springLegacyAI::CAIAICallback::GetMapWidth() {
	return sAICallback->Map_getWidth(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetMapHeight() {
	return sAICallback->Map_getHeight(skirmishAIId);
}


const float* springLegacyAI::CAIAICallback::GetHeightMap()
{
	if (heightMap.empty()) {
		heightMap.resize(sAICallback->Map_getHeightMap(skirmishAIId, nullptr, 0));
		sAICallback->Map_getHeightMap(skirmishAIId, &heightMap[0], heightMap.size());
	}

	return &heightMap[0];
}

const float* springLegacyAI::CAIAICallback::GetCornersHeightMap()
{
	if (cornersHeightMap.empty()) {
		cornersHeightMap.resize(sAICallback->Map_getCornersHeightMap(skirmishAIId, nullptr, 0));
		sAICallback->Map_getCornersHeightMap(skirmishAIId, &cornersHeightMap[0], cornersHeightMap.size());
	}

	return &cornersHeightMap[0];
}

float springLegacyAI::CAIAICallback::GetMinHeight() {
	return sAICallback->Map_getMinHeight(skirmishAIId);
}

float springLegacyAI::CAIAICallback::GetMaxHeight() {
	return sAICallback->Map_getMaxHeight(skirmishAIId);
}

const float* springLegacyAI::CAIAICallback::GetSlopeMap()
{
	if (slopeMap.empty()) {
		slopeMap.resize(sAICallback->Map_getSlopeMap(skirmishAIId, nullptr, 0));
		sAICallback->Map_getSlopeMap(skirmishAIId, &slopeMap[0], slopeMap.size());
	}

	return &slopeMap[0];
}

const unsigned short* springLegacyAI::CAIAICallback::GetLosMap()
{
	if (losMap.empty()) {
		losMap.resize(sAICallback->Map_getLosMap(skirmishAIId, nullptr, 0));

		std::vector<int> tmpMap(losMap.size());

		sAICallback->Map_getLosMap(skirmishAIId, &tmpMap[0], losMap.size());
		CopyArray(&tmpMap[0], &losMap[0], losMap.size());
	}

	return &losMap[0];
}

int springLegacyAI::CAIAICallback::GetLosMapResolution() {
	const int fullSize = GetMapWidth() * GetMapHeight();
	const int losSize = sAICallback->Map_getLosMap(skirmishAIId, nullptr, 0);

	return fullSize / losSize;
}

const unsigned short* springLegacyAI::CAIAICallback::GetRadarMap()
{
	if (radarMap.empty()) {
		radarMap.resize(sAICallback->Map_getRadarMap(skirmishAIId, nullptr, 0));

		std::vector<int> tmpMap(radarMap.size());

		sAICallback->Map_getRadarMap(skirmishAIId, &tmpMap[0], radarMap.size());
		CopyArray(&tmpMap[0], &radarMap[0], radarMap.size());
	}

	return &radarMap[0];
}

const unsigned short* springLegacyAI::CAIAICallback::GetJammerMap()
{
	if (jammerMap.empty()) {
		jammerMap.resize(sAICallback->Map_getJammerMap(skirmishAIId, nullptr, 0));

		std::vector<int> tmpMap(jammerMap.size());

		sAICallback->Map_getJammerMap(skirmishAIId, &tmpMap[0], jammerMap.size());
		CopyArray(&tmpMap[0], &jammerMap[0], jammerMap.size());
	}

	return &jammerMap[0];
}

const unsigned char* springLegacyAI::CAIAICallback::GetMetalMap()
{
	if (metalMap.empty()) {
		metalMap.resize(sAICallback->Map_getResourceMapRaw(skirmishAIId, METAL_RES_IDENT, nullptr, 0));

		std::vector<short> tmpMap(metalMap.size());

		sAICallback->Map_getResourceMapRaw(skirmishAIId, METAL_RES_IDENT, &tmpMap[0], metalMap.size());
		CopyArray(&tmpMap[0], &metalMap[0], metalMap.size());
	}

	return &metalMap[0];
}

int springLegacyAI::CAIAICallback::GetMapHash() {
	return sAICallback->Map_getHash(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetMapName() {
	return sAICallback->Map_getName(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetMapHumanName() {
	return sAICallback->Map_getHumanName(skirmishAIId);
}

int springLegacyAI::CAIAICallback::GetModHash() {
	return sAICallback->Mod_getHash(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetModName() {
	return sAICallback->Mod_getFileName(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetModHumanName() {
	return sAICallback->Mod_getHumanName(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetModShortName() {
	return sAICallback->Mod_getShortName(skirmishAIId);
}

const char* springLegacyAI::CAIAICallback::GetModVersion() {
	return sAICallback->Mod_getVersion(skirmishAIId);
}

float springLegacyAI::CAIAICallback::GetElevation(float x, float z) {
	return sAICallback->Map_getElevationAt(skirmishAIId, x, z);
}


float springLegacyAI::CAIAICallback::GetMaxMetal() const {
	return sAICallback->Map_getMaxResource(skirmishAIId, METAL_RES_IDENT);
}
float springLegacyAI::CAIAICallback::GetExtractorRadius() const {
	return sAICallback->Map_getExtractorRadius(skirmishAIId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetMinWind() const { return sAICallback->Map_getMinWind(skirmishAIId); }
float springLegacyAI::CAIAICallback::GetMaxWind() const { return sAICallback->Map_getMaxWind(skirmishAIId); }
float springLegacyAI::CAIAICallback::GetCurWind() const { return sAICallback->Map_getCurWind(skirmishAIId); }

float springLegacyAI::CAIAICallback::GetTidalStrength() const  { return sAICallback->Map_getTidalStrength(skirmishAIId); }
float springLegacyAI::CAIAICallback::GetGravity() const { return sAICallback->Map_getGravity(skirmishAIId); }


bool springLegacyAI::CAIAICallback::CanBuildAt(const UnitDef* unitDef, float3 pos, int facing) {
	return sAICallback->Map_isPossibleToBuildAt(skirmishAIId, unitDef->id, &pos[0], facing);
}

float3 springLegacyAI::CAIAICallback::ClosestBuildSite(const UnitDef* unitDef, float3 pos, float searchRadius, int minDist, int facing)
{
	float3 ret;
	sAICallback->Map_findClosestBuildSite(skirmishAIId, unitDef->id, &pos[0], searchRadius, minDist, facing, &ret[0]);
	return ret;
}

/*
bool springLegacyAI::CAIAICallback::GetProperty(int id, int property, void* dst) {
//	return sAICallback->getProperty(skirmishAIId, id, property, dst);
	return false;
}
*/
bool springLegacyAI::CAIAICallback::GetProperty(int unitId, int propertyId, void *data)
{
	switch (propertyId) {
		case AIVAL_UNITDEF: {
			return false;
		}
		case AIVAL_CURRENT_FUEL: {
			(*(float*)data) = 0.0f;
			return true;
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
bool springLegacyAI::CAIAICallback::GetValue(int valueId, void* dst) {
//	return sAICallback->getValue(skirmishAIId, valueId, dst);
	return false;
}
*/
bool springLegacyAI::CAIAICallback::GetValue(int valueId, void *data)
{
	switch (valueId) {
		case AIVAL_NUMDAMAGETYPES:{
			*((int*)data) = sAICallback->WeaponDef_getNumDamageTypes(skirmishAIId);
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
			float3 dir;
			sAICallback->Gui_Camera_getDirection(skirmishAIId, &dir[0]);
			*(static_cast<float3*>(data)) = dir;
			return true;
		}case AIVAL_GUI_CAMERA_POS:{
			float3 pos;
			sAICallback->Gui_Camera_getPosition(skirmishAIId, &pos[0]);
			*(static_cast<float3*>(data)) = pos;
			return true;
		}case AIVAL_LOCATE_FILE_R:{
			//sAICallback->File_locateForReading(skirmishAIId, (char*) data);
			char absPath[2048] = {0};

			const bool located = sAICallback->DataDirs_locatePath(skirmishAIId, absPath, sizeof(absPath), (const char*) data, false, false, false, false);

			// NOTE We can not use STRCPY_T or STRNCPY here, as we do not know
			//   the size of data. It might be below sizeof(absPath),
			//   and thus we would corrupt the stack.
			STRCPY((char*)data, absPath);
			return located;
		}case AIVAL_LOCATE_FILE_W:{
			//sAICallback->File_locateForWriting(skirmishAIId, (char*) data);
			char absPath[2048] = {0};

			const bool located = sAICallback->DataDirs_locatePath(skirmishAIId, absPath, sizeof(absPath), (const char*) data, true, true, false, false);

			// NOTE We can not use STRCPY_T or STRNCPY here, as we do not know
			//   the size of data. It might be below sizeof(absPath),
			//   and thus we would corrupt the stack.
			STRCPY((char*)data, absPath);
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

int springLegacyAI::CAIAICallback::GetFileSize(const char* name) {
	return sAICallback->File_getSize(skirmishAIId, name);
}

int springLegacyAI::CAIAICallback::GetSelectedUnits(int* unitIds, int unitIds_max) {
	return sAICallback->getSelectedUnits(skirmishAIId, unitIds, unitIds_max);
}

float3 springLegacyAI::CAIAICallback::GetMousePos() {
	float3 pos;
	sAICallback->Map_getMousePos(skirmishAIId, &pos[0]);
	return pos;
}


int springLegacyAI::CAIAICallback::GetMapPoints(PointMarker* pm, int, bool includeAllies)
{
	const int numPoints = sAICallback->Map_getPoints(skirmishAIId, includeAllies);

	float3 pos;
	short col[3];

	for (int p = 0; p < numPoints; ++p) {
		sAICallback->Map_Point_getPosition(skirmishAIId, p, &pm[p].pos[0]);
		sAICallback->Map_Point_getColor(skirmishAIId, p, &col[0]);

		pm[p].color = SColor((uint8_t) col[0], (uint8_t) col[1], (uint8_t) col[2], (uint8_t) 255);
		pm[p].label = sAICallback->Map_Point_getLabel(skirmishAIId, p);
	}

	return numPoints;
}

int springLegacyAI::CAIAICallback::GetMapLines(LineMarker* lm, int, bool includeAllies)
{
	const int numLines = sAICallback->Map_getLines(skirmishAIId, includeAllies);

	float3 pos;
	short col[3];

	for (int l = 0; l < numLines; ++l) {
		sAICallback->Map_Line_getFirstPosition(skirmishAIId, l, &lm[l].pos[0]);
		sAICallback->Map_Line_getSecondPosition(skirmishAIId, l, &lm[l].pos2[0]);
		sAICallback->Map_Line_getColor(skirmishAIId, l, &col[0]);

		lm[l].color = SColor((uint8_t) col[0], (uint8_t) col[1], (uint8_t) col[2], (uint8_t) 255);
	}

	return numLines;
}


float springLegacyAI::CAIAICallback::GetMetal() {
	return sAICallback->Economy_getCurrent(skirmishAIId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetMetalIncome() {
	return sAICallback->Economy_getIncome(skirmishAIId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetMetalUsage() {
	return sAICallback->Economy_getUsage(skirmishAIId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetMetalStorage() {
	return sAICallback->Economy_getStorage(skirmishAIId, METAL_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetEnergy() {
	return sAICallback->Economy_getCurrent(skirmishAIId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetEnergyIncome() {
	return sAICallback->Economy_getIncome(skirmishAIId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetEnergyUsage() {
	return sAICallback->Economy_getUsage(skirmishAIId, ENERGY_RES_IDENT);
}

float springLegacyAI::CAIAICallback::GetEnergyStorage() {
	return sAICallback->Economy_getStorage(skirmishAIId, ENERGY_RES_IDENT);
}

int springLegacyAI::CAIAICallback::GetFeatures(int* featureIds, int featureIds_max) {
	return sAICallback->getFeatures(skirmishAIId, featureIds, featureIds_max);
}

int springLegacyAI::CAIAICallback::GetFeatures(int *featureIds, int featureIds_max, const float3& pos, float radius) {
	float3 cpyPos = pos;
	return sAICallback->getFeaturesIn(skirmishAIId, &cpyPos[0], radius, featureIds, featureIds_max);
}

const springLegacyAI::FeatureDef* springLegacyAI::CAIAICallback::GetFeatureDef(int featureId) {
	return GetFeatureDefById(sAICallback->Feature_getDef(skirmishAIId, featureId));
}

const springLegacyAI::FeatureDef* springLegacyAI::CAIAICallback::GetFeatureDefById(int featureDefId)
{
	if (featureDefId < 0)
		return nullptr;

	if (featureDefFrames[featureDefId] < 0) {
		const int currentFrame = 1; // GetCurrentFrame();

		FeatureDef* featureDef = &featureDefs[featureDefId];

		featureDef->myName = sAICallback->FeatureDef_getName(skirmishAIId, featureDefId);
		featureDef->description = sAICallback->FeatureDef_getDescription(skirmishAIId, featureDefId);
		//featureDef->id = sAICallback->FeatureDef_getId(skirmishAIId, featureDefId);
		featureDef->id = featureDefId;
		featureDef->metal = sAICallback->FeatureDef_getContainedResource(skirmishAIId, featureDefId, METAL_RES_IDENT);
		featureDef->energy = sAICallback->FeatureDef_getContainedResource(skirmishAIId, featureDefId, ENERGY_RES_IDENT);
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
		featureDef->xsize = sAICallback->FeatureDef_getXSize(skirmishAIId, featureDefId);
		featureDef->zsize = sAICallback->FeatureDef_getZSize(skirmishAIId, featureDefId);

		{
			const int size = sAICallback->FeatureDef_getCustomParams(skirmishAIId, featureDefId, nullptr, nullptr);

			std::vector<const char*> cKeys(size, "");
			std::vector<const char*> cValues(size, "");

			sAICallback->FeatureDef_getCustomParams(skirmishAIId, featureDefId, &cKeys[0], &cValues[0]);

			for (int i = 0; i < size; ++i) {
				featureDef->customParams[cKeys[i]] = cValues[i];
			}
		}

		featureDefFrames[featureDefId] = currentFrame;
	}

	return &featureDefs[featureDefId];
}

float springLegacyAI::CAIAICallback::GetFeatureHealth(int featureId) {
	return sAICallback->Feature_getHealth(skirmishAIId, featureId);
}

float springLegacyAI::CAIAICallback::GetFeatureReclaimLeft(int featureId) {
	return sAICallback->Feature_getReclaimLeft(skirmishAIId, featureId);
}

float3 springLegacyAI::CAIAICallback::GetFeaturePos(int featureId) {
	float3 pos;
	sAICallback->Feature_getPosition(skirmishAIId, featureId, &pos[0]);
	return pos;
}

int springLegacyAI::CAIAICallback::GetNumUnitDefs() {
	return sAICallback->getUnitDefs(skirmishAIId, nullptr, 0);
}

void springLegacyAI::CAIAICallback::GetUnitDefList(const UnitDef** list) {
	std::vector<int> unitDefIds(sAICallback->getUnitDefs(skirmishAIId, nullptr, 0));

	// get actual number of IDs
	const int size = sAICallback->getUnitDefs(skirmishAIId, &unitDefIds[0], unitDefIds.size());

	for (int i = 0; i < size; ++i) {
		list[i] = GetUnitDefById(unitDefIds[i]);
	}
}

float springLegacyAI::CAIAICallback::GetUnitDefHeight(int def) {
	return sAICallback->UnitDef_getHeight(skirmishAIId, def);
}

float springLegacyAI::CAIAICallback::GetUnitDefRadius(int def) {
	return sAICallback->UnitDef_getRadius(skirmishAIId, def);
}

const springLegacyAI::WeaponDef* springLegacyAI::CAIAICallback::GetWeapon(const char* weaponName) {
	return GetWeaponDefById(sAICallback->getWeaponDefByName(skirmishAIId, weaponName));
}

const springLegacyAI::WeaponDef* springLegacyAI::CAIAICallback::GetWeaponDefById(int weaponDefId)
{
	if (weaponDefId < 0)
		return nullptr;

	if (weaponDefFrames[weaponDefId] < 0) {
		const int currentFrame = 1; // GetCurrentFrame();

		std::vector<float> typeDamages(sAICallback->WeaponDef_Damage_getTypes(skirmishAIId, weaponDefId, nullptr, 0));

		if (!typeDamages.empty())
			sAICallback->WeaponDef_Damage_getTypes(skirmishAIId, weaponDefId, &typeDamages[0], typeDamages.size());

		WeaponDef* weaponDef = &weaponDefs[weaponDefId];

		// weaponDef->damages = sAICallback->WeaponDef_getDamages(skirmishAIId, weaponDefId);
		weaponDef->damages = DamageArray(typeDamages);
		weaponDef->damages.paralyzeDamageTime = sAICallback->WeaponDef_Damage_getParalyzeDamageTime(skirmishAIId, weaponDefId);
		weaponDef->damages.impulseFactor = sAICallback->WeaponDef_Damage_getImpulseFactor(skirmishAIId, weaponDefId);
		weaponDef->damages.impulseBoost = sAICallback->WeaponDef_Damage_getImpulseBoost(skirmishAIId, weaponDefId);
		weaponDef->damages.craterMult = sAICallback->WeaponDef_Damage_getCraterMult(skirmishAIId, weaponDefId);
		weaponDef->damages.craterBoost = sAICallback->WeaponDef_Damage_getCraterBoost(skirmishAIId, weaponDefId);

		weaponDef->name = sAICallback->WeaponDef_getName(skirmishAIId, weaponDefId);
		weaponDef->type = sAICallback->WeaponDef_getType(skirmishAIId, weaponDefId);
		weaponDef->description = sAICallback->WeaponDef_getDescription(skirmishAIId, weaponDefId);
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
		weaponDef->uptime = sAICallback->WeaponDef_getUpTime(skirmishAIId, weaponDefId);
		weaponDef->flighttime = sAICallback->WeaponDef_getFlightTime(skirmishAIId, weaponDefId);
		weaponDef->metalcost = sAICallback->WeaponDef_getCost(skirmishAIId, weaponDefId, METAL_RES_IDENT);
		weaponDef->energycost = sAICallback->WeaponDef_getCost(skirmishAIId, weaponDefId, ENERGY_RES_IDENT);
		weaponDef->projectilespershot = sAICallback->WeaponDef_getProjectilesPerShot(skirmishAIId, weaponDefId);

		// weaponDef->id = sAICallback->WeaponDef_getId(skirmishAIId, weaponDefId);
		weaponDef->id = weaponDefId;
		// weaponDef->tdfId = sAICallback->WeaponDef_getTdfId(skirmishAIId, weaponDefId);
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
		weaponDef->duration = sAICallback->WeaponDef_getDuration(skirmishAIId, weaponDefId);
		weaponDef->falloffRate = sAICallback->WeaponDef_getFalloffRate(skirmishAIId, weaponDefId);
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
		weaponDef->shieldEnergyUse = sAICallback->WeaponDef_Shield_getResourceUse(skirmishAIId, weaponDefId, ENERGY_RES_IDENT);
		weaponDef->shieldRadius = sAICallback->WeaponDef_Shield_getRadius(skirmishAIId, weaponDefId);
		weaponDef->shieldForce = sAICallback->WeaponDef_Shield_getForce(skirmishAIId, weaponDefId);
		weaponDef->shieldMaxSpeed = sAICallback->WeaponDef_Shield_getMaxSpeed(skirmishAIId, weaponDefId);
		weaponDef->shieldPower = sAICallback->WeaponDef_Shield_getPower(skirmishAIId, weaponDefId);
		weaponDef->shieldPowerRegen = sAICallback->WeaponDef_Shield_getPowerRegen(skirmishAIId, weaponDefId);
		weaponDef->shieldPowerRegenEnergy = sAICallback->WeaponDef_Shield_getPowerRegenResource(skirmishAIId, weaponDefId, ENERGY_RES_IDENT);
		weaponDef->shieldStartingPower = sAICallback->WeaponDef_Shield_getStartingPower(skirmishAIId, weaponDefId);
		weaponDef->shieldRechargeDelay = sAICallback->WeaponDef_Shield_getRechargeDelay(skirmishAIId, weaponDefId);
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
			const int size = sAICallback->WeaponDef_getCustomParams(skirmishAIId, weaponDefId, nullptr, nullptr);

			std::vector<const char*> cKeys(size, "");
			std::vector<const char*> cValues(size, "");

			if (size > 0)
				sAICallback->WeaponDef_getCustomParams(skirmishAIId, weaponDefId, &cKeys[0], &cValues[0]);

			for (int i = 0; i < size; ++i) {
				weaponDef->customParams[cKeys[i]] = cValues[i];
			}
		}

		weaponDefFrames[weaponDefId] = currentFrame;
	}

	return &weaponDefs[weaponDefId];
}

const float3* springLegacyAI::CAIAICallback::GetStartPos() {
	sAICallback->Map_getStartPos(skirmishAIId, &startPos[0]);
	return &startPos;
}

unsigned int springLegacyAI::CAIAICallback::GetCategoryFlag(const char* categoryName) {
	return sAICallback->Game_getCategoryFlag(skirmishAIId, categoryName);
}

unsigned int springLegacyAI::CAIAICallback::GetCategoriesFlag(const char* categoryNames) {
	return sAICallback->Game_getCategoriesFlag(skirmishAIId, categoryNames);
}

void springLegacyAI::CAIAICallback::GetCategoryName(int categoryFlag, char* name, int nameMaxSize) {
	sAICallback->Game_getCategoryName(skirmishAIId, categoryFlag, name, nameMaxSize);
}






void springLegacyAI::CAIAICallback::SendTextMsg(const char* text, int zone) {
	SSendTextMessageCommand cmd = {text, zone};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_TEXT_MESSAGE, &cmd);
}

void springLegacyAI::CAIAICallback::SetLastMsgPos(float3 pos) {
	SSetLastPosMessageCommand cmd = {&pos[0]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SET_LAST_POS_MESSAGE, &cmd);
}

void springLegacyAI::CAIAICallback::AddNotification(float3 pos, float3 color, float alpha) {
	short cpyColor[] = {
		(short) (color[0] * 255),
		(short) (color[1] * 255),
		(short) (color[2] * 255),
		(short) (   alpha * 255),
	};

	SAddNotificationDrawerCommand cmd = {&pos[0], &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_ADD_NOTIFICATION, &cmd);
}

bool springLegacyAI::CAIAICallback::SendResources(float mAmount, float eAmount, int receivingTeam) {
	SSendResourcesCommand cmd = {static_cast<int>(mAmount), eAmount, receivingTeam};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_RESOURCES, &cmd);
	return cmd.ret_isExecuted;
}

int springLegacyAI::CAIAICallback::SendUnits(const std::vector<int>& unitIds, int receivingTeam) {
	if (unitIds.empty())
		return 0;

	std::vector<int> tmpUnitIds = unitIds;

	SSendUnitsCommand cmd = {&tmpUnitIds[0], static_cast<int>(unitIds.size()), receivingTeam};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_UNITS, &cmd);

	return cmd.ret_sentUnits;
}

void* springLegacyAI::CAIAICallback::CreateSharedMemArea(char* name, int size) {
	//SCreateSharedMemAreaCommand cmd = {name, size};
	//sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_CREATE, &cmd);
	//return cmd.ret_sharedMemArea;
	assert(false);
	return nullptr;
}

void springLegacyAI::CAIAICallback::ReleasedSharedMemArea(char* name) {
	//SReleaseSharedMemAreaCommand cmd = {name};
	//sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SHARED_MEM_AREA_RELEASE, &cmd);
	assert(false);
}

int springLegacyAI::CAIAICallback::CreateGroup() {
	SCreateGroupCommand cmd = {};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_CREATE, &cmd);
	return cmd.ret_groupId;
}

void springLegacyAI::CAIAICallback::EraseGroup(int groupId) {
	SEraseGroupCommand cmd = {groupId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_GROUP_ERASE, &cmd);
}

bool springLegacyAI::CAIAICallback::AddUnitToGroup(int unitId, int groupId) {
	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0, groupId};
	const int ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_ADD, &cmd);
	return (ret == 0);
}

bool springLegacyAI::CAIAICallback::RemoveUnitFromGroup(int unitId) {
	SGroupAddUnitCommand cmd = {unitId, -1, 0, 0};
	const int ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_UNIT_GROUP_CLEAR, &cmd);
	return (ret == 0);
}

int springLegacyAI::CAIAICallback::GiveGroupOrder(int groupId, Command* c) {
	return Internal_GiveOrder(-1, groupId, c);
}

int springLegacyAI::CAIAICallback::GiveOrder(int unitId, Command* c) {
	return Internal_GiveOrder(unitId, -1, c);
}

int springLegacyAI::CAIAICallback::Internal_GiveOrder(int unitId, int groupId, Command* c) {
	RawCommand rc = std::move(c->ToRawCommand());

	return (sAICallback->Engine_executeCommand(skirmishAIId, unitId, groupId, &rc));
}

int springLegacyAI::CAIAICallback::InitPath(float3 start, float3 end, int pathType, float goalRadius)
{
	SInitPathCommand cmd = {&start[0], &end[0], pathType, goalRadius};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_INIT, &cmd);
	return cmd.ret_pathId;
}

float3 springLegacyAI::CAIAICallback::GetNextWaypoint(int pathId) {
	float3 pos;
	SGetNextWaypointPathCommand cmd = {pathId, &pos[0]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_NEXT_WAYPOINT, &cmd);
	return float3(cmd.ret_nextWaypoint_posF3_out);
}

float springLegacyAI::CAIAICallback::GetPathLength(float3 start, float3 end, int pathType, float goalRadius) {
	SGetApproximateLengthPathCommand cmd = {&start[0], &end[0], pathType, goalRadius};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_GET_APPROXIMATE_LENGTH, &cmd);
	return cmd.ret_approximatePathLength;
}

void springLegacyAI::CAIAICallback::FreePath(int pathId) {
	SFreePathCommand cmd = {pathId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PATH_FREE, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerStartPath(const float3& pos, const float* color) {
	float3 cpyPos = pos;
	short cpyColor[] = {
		(short) (color[0] * 255),
		(short) (color[1] * 255),
		(short) (color[2] * 255),
		(short) (color[3] * 255),
	};

	SStartPathDrawerCommand cmd = {&cpyPos[0], &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_START, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerFinishPath() {
	SFinishPathDrawerCommand cmd = {};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_FINISH, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerDrawLine(const float3& endPos, const float* color) {
	float3 cpyEndPos = endPos;
	short cpyColor[] = {
		(short) (color[0] * 255),
		(short) (color[1] * 255),
		(short) (color[2] * 255),
		(short) (color[3] * 255),
	};

	SDrawLinePathDrawerCommand cmd = {&cpyEndPos[0], &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerDrawLineAndIcon(int cmdId, const float3& endPos, const float* color) {
	float3 cpyEndPos = endPos;
	short cpyColor[] = {
		(short) (color[0] * 255),
		(short) (color[1] * 255),
		(short) (color[2] * 255),
		(short) (color[3] * 255),
	};

	SDrawLineAndIconPathDrawerCommand cmd = {cmdId, &cpyEndPos[0], &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerDrawIconAtLastPos(int cmdId) {
	SDrawIconAtLastPosPathDrawerCommand cmd = {cmdId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerBreak(const float3& endPos, const float* color) {
	float3 cpyEndPos = endPos;
	short cpyColor[] = {
		(short) (color[0] * 255),
		(short) (color[1] * 255),
		(short) (color[2] * 255),
		(short) (color[3] * 255),
	};

	SBreakPathDrawerCommand cmd = {&cpyEndPos[0], &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_BREAK, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerRestart() {
	SRestartPathDrawerCommand cmd = {false};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

void springLegacyAI::CAIAICallback::LineDrawerRestartSameColor() {
	SRestartPathDrawerCommand cmd = {true};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_PATH_RESTART, &cmd);
}

int springLegacyAI::CAIAICallback::CreateSplineFigure(
	float3 pos1,
	float3 pos2,
	float3 pos3,
	float3 pos4,
	float width,
	int arrow,
	int lifeTime,
	int figureGroupId
) {
	SCreateSplineFigureDrawerCommand cmd = {
		&pos1[0],
		&pos2[0],
		&pos3[0],
		&pos4[0],
		width,
		static_cast<bool>(arrow),
		lifeTime,
		figureGroupId
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_SPLINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

int springLegacyAI::CAIAICallback::CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifeTime, int figureGroupId) {
	SCreateLineFigureDrawerCommand cmd = {
		&pos1[0],
		&pos2[0],
		width,
		static_cast<bool>(arrow),
		lifeTime,
		figureGroupId
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_CREATE_LINE, &cmd);
	return cmd.ret_newFigureGroupId;
}

void springLegacyAI::CAIAICallback::SetFigureColor(int figureGroupId, float red, float green, float blue, float alpha) {
	short cpyColor[] = {
		(short) (  red * 255),
		(short) (green * 255),
		(short) ( blue * 255),
		(short) (alpha * 255),
	};

	SSetColorFigureDrawerCommand cmd = {figureGroupId, &cpyColor[0], cpyColor[3]};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_SET_COLOR, &cmd);
}

void springLegacyAI::CAIAICallback::DeleteFigureGroup(int figureGroupId) {
	SDeleteFigureDrawerCommand cmd = {figureGroupId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_FIGURE_DELETE, &cmd);
}

void springLegacyAI::CAIAICallback::DrawUnit(const char* name, float3 pos, float rotation, int lifeTime, int unitTeamId, bool transparent, bool drawBorder, int facing) {
	SDrawUnitDrawerCommand cmd = {
		sAICallback->getUnitDefByName(skirmishAIId, name),
		&pos[0],
		rotation,
		lifeTime,
		unitTeamId,
		transparent,
		drawBorder,
		facing
	};

	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_DRAW_UNIT, &cmd);
}



bool springLegacyAI::CAIAICallback::IsDebugDrawerEnabled() const {
	return sAICallback->Debug_GraphDrawer_isEnabled(skirmishAIId);
}

void springLegacyAI::CAIAICallback::DebugDrawerAddGraphPoint(int lineId, float x, float y) {
	SAddPointLineGraphDrawerDebugCommand cmd = {lineId, x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerDelGraphPoints(int lineId, int numPoints) {
	SDeletePointsLineGraphDrawerDebugCommand cmd = {lineId, numPoints};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS, &cmd);
}

void springLegacyAI::CAIAICallback::DebugDrawerSetGraphPos(float x, float y) {
	SSetPositionGraphDrawerDebugCommand cmd = {x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_POS, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetGraphSize(float w, float h) {
	SSetSizeGraphDrawerDebugCommand cmd = {w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetGraphLineColor(int lineId, const float3& color) {

	short color_s3[3];
	color_s3[0] = (short) color[0] * 256;
	color_s3[1] = (short) color[1] * 256;
	color_s3[2] = (short) color[2] * 256;

	SSetColorLineGraphDrawerDebugCommand cmd = {lineId, color_s3};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetGraphLineLabel(int lineId, const char* label) {
	SSetLabelLineGraphDrawerDebugCommand cmd = {lineId, label};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL, &cmd);
}

int springLegacyAI::CAIAICallback::DebugDrawerAddOverlayTexture(const float* texData, int w, int h) {
	SAddOverlayTextureDrawerDebugCommand cmd = {0, texData, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD, &cmd);
	return cmd.ret_overlayTextureId;
}
void springLegacyAI::CAIAICallback::DebugDrawerUpdateOverlayTexture(int overlayTextureId, const float* texData, int x, int y, int w, int h) {
	SUpdateOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, texData, x, y, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerDelOverlayTexture(int overlayTextureId) {
	SDeleteOverlayTextureDrawerDebugCommand cmd = {overlayTextureId};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetOverlayTexturePos(int overlayTextureId, float x, float y) {
	SSetPositionOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, x, y};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetOverlayTextureSize(int overlayTextureId, float w, float h) {
	SSetSizeOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, w, h};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE, &cmd);
}
void springLegacyAI::CAIAICallback::DebugDrawerSetOverlayTextureLabel(int overlayTextureId, const char* texLabel) {
	SSetLabelOverlayTextureDrawerDebugCommand cmd = {overlayTextureId, texLabel};
	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL, &cmd);
}



int springLegacyAI::CAIAICallback::HandleCommand(int commandId, void* data) {
	int ret = -99;

	switch (commandId) {
		case AIHCQuerySubVersionId: {
//			SQuerySubVersionCommand cmd;
//			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, cmdTopicId, &cmd);
			ret = sAICallback->Game_getAiInterfaceVersion(skirmishAIId);
			break;
		}
		case AIHCAddMapPointId: {
			const AIHCAddMapPoint* myData = static_cast<AIHCAddMapPoint*>(data);

			float3 cpyPos = myData->pos;
			SAddPointDrawCommand cmd = {&cpyPos[0], myData->label};

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_ADD, &cmd);
			break;
		}
		case AIHCAddMapLineId: {
			const AIHCAddMapLine* myData = static_cast<AIHCAddMapLine*>(data);

			float3 cpyPosFrom = myData->posfrom;
			float3 cpyPosTo = myData->posto;
			SAddLineDrawCommand cmd = {&cpyPosFrom[0], &cpyPosTo[0]};

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_LINE_ADD, &cmd);
			break;
		}
		case AIHCRemoveMapPointId: {
			const AIHCRemoveMapPoint* myData = static_cast<AIHCRemoveMapPoint*>(data);

			float3 cpyPos = myData->pos;
			SRemovePointDrawCommand cmd = {&cpyPos[0]};

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_DRAWER_POINT_REMOVE, &cmd);
			break;
		}
		case AIHCSendStartPosId: {
			const AIHCSendStartPos* myData = static_cast<AIHCSendStartPos*>(data);

			float3 cpyPos = myData->pos;
			SSendStartPosCommand cmd = {myData->ready, &cpyPos[0]};

			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_SEND_START_POS, &cmd);
			break;
		}

		case AIHCTraceRayId: {
			AIHCTraceRay* myData = static_cast<AIHCTraceRay*>(data);
			STraceRayCommand cCmdData = {
				&myData->rayPos[0],
				&myData->rayDir[0],
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
			AIHCFeatureTraceRay* myData = static_cast<AIHCFeatureTraceRay*>(data);
			SFeatureTraceRayCommand cCmdData = {
				&myData->rayPos[0],
				&myData->rayDir[0],
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
			const AIHCPause* cppCmdData = static_cast<AIHCPause*>(data);
			SPauseCommand cCmdData = {
				cppCmdData->enable,
				cppCmdData->reason
			};
			ret = sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_PAUSE, &cCmdData);
			break;
		}

		case AIHCGetDataDirId: {
			AIHCGetDataDir* cppCmdData = static_cast<AIHCGetDataDir*>(data);
			cppCmdData->ret_path = sAICallback->DataDirs_allocatePath(
					skirmishAIId,
					cppCmdData->relPath,
					cppCmdData->writeable,
					cppCmdData->create,
					cppCmdData->dir,
					cppCmdData->common);
			ret = (cppCmdData->ret_path[0] != 0);
			break;
		}
	}

	return ret;
}

bool springLegacyAI::CAIAICallback::ReadFile(const char* filename, void* buffer, int bufferLen) {
//	SReadFileCommand cmd = {name, buffer, bufferLen};
//	sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_READ_FILE, &cmd); return cmd.ret_isExecuted;
	return sAICallback->File_getContent(skirmishAIId, filename, buffer, bufferLen);
}


#define AIAICALLBACK_CALL_LUA(HandleName, HANDLENAME)  \
	std::string springLegacyAI::CAIAICallback::CallLua ## HandleName(const char* inData, int inSize) {  \
		char outData[MAX_RESPONSE_SIZE];  \
		SCallLua ## HandleName ## Command cmd = {inData, inSize, outData};  \
		sAICallback->Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, COMMAND_CALL_LUA_ ## HANDLENAME, &cmd);  \
		return std::string(outData);  \
	}

AIAICALLBACK_CALL_LUA(Rules, RULES)
AIAICALLBACK_CALL_LUA(UI, UI)

#undef AIAICALLBACK_CALL_LUA



std::map<std::string, std::string> springLegacyAI::CAIAICallback::GetMyInfo()
{
	std::map<std::string, std::string> info;

	const int info_size = sAICallback->SkirmishAI_Info_getSize(skirmishAIId);
	for (int ii = 0; ii < info_size; ++ii) {
		const char* key   = sAICallback->SkirmishAI_Info_getKey(skirmishAIId, ii);
		const char* value = sAICallback->SkirmishAI_Info_getValue(skirmishAIId, ii);
		if ((key != nullptr) && (value != nullptr)) {
			info[key] = value;
		}
	}

	return info;
}

std::map<std::string, std::string> springLegacyAI::CAIAICallback::GetMyOptionValues()
{
	std::map<std::string, std::string> optionVals;

	const int optionVals_size = sAICallback->SkirmishAI_OptionValues_getSize(skirmishAIId);
	for (int ovi = 0; ovi < optionVals_size; ++ovi) {
		const char* key   = sAICallback->SkirmishAI_OptionValues_getKey(skirmishAIId, ovi);
		const char* value = sAICallback->SkirmishAI_OptionValues_getValue(skirmishAIId, ovi);
		if ((key != nullptr) && (value != nullptr)) {
			optionVals[key] = value;
		}
	}

	return optionVals;
}
