/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AICheats.h"

#include "StdAfx.h"
#include "ExternalAI/SkirmishAIWrapper.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "mmgr.h"

#include <vector>
#include <list>

#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)uh->MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())

CUnit* CAICheats::GetUnit(int unitId) const {

	CUnit* unit = NULL;

	if (CHECK_UNITID(unitId)) {
		unit = uh->units[unitId];
	}

	return unit;
}


CAICheats::CAICheats(CSkirmishAIWrapper* ai): ai(ai)
{
}

CAICheats::~CAICheats()
{}



bool CAICheats::OnlyPassiveCheats()
{
	// returns whether cheats will desync (this is
	// always the case unless we are both the host
	// and the only client) if used by an AI
	if (!gameServer) {
		return true;
	} else if (gameSetup && (gameSetup->playerStartingData.size() == 1)) {
		// assumes AI's dont count toward numPlayers
		return false;
	} else {
		// disable it in case we are not sure
		return true;
	}
}

void CAICheats::EnableCheatEvents(bool enable)
{
	// enable sending of EnemyCreated, etc. events
	ai->SetCheatEventsEnabled(enable);
}



void CAICheats::SetMyIncomeMultiplier(float incomeMultiplier)
{
	if (!OnlyPassiveCheats()) {
		teamHandler->Team(ai->GetTeamId())->SetIncomeMultiplier(incomeMultiplier);
	}
}

void CAICheats::GiveMeMetal(float amount)
{
	if (!OnlyPassiveCheats())
		teamHandler->Team(ai->GetTeamId())->metal += amount;
}

void CAICheats::GiveMeEnergy(float amount)
{
	if (!OnlyPassiveCheats())
		teamHandler->Team(ai->GetTeamId())->energy += amount;
}

int CAICheats::CreateUnit(const char* name, const float3& pos)
{
	int unitId = 0;

	if (!OnlyPassiveCheats()) {
		CUnit* unit = unitLoader->LoadUnit(name, pos, ai->GetTeamId(), false, 0, NULL);
		if (unit) {
			unitId = unit->id;
		}
	}

	return unitId;
}

const UnitDef* CAICheats::GetUnitDef(int unitId)
{
	const UnitDef* unitDef = NULL;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitDef = unit->unitDef;
	}

	return unitDef;
}



float3 CAICheats::GetUnitPos(int unitId)
{
	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		return unit->pos;
	}

	return ZeroVector;
}

float3 CAICheats::GetUnitVelocity(int unitId)
{
	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		return unit->speed;
	}

	return ZeroVector;
}


static int FilterUnitsVector(const std::vector<CUnit*>& units, int* unitIds, int unitIds_max, bool (*includeUnit)(CUnit*) = NULL)
{
	int a = 0;

	if (unitIds_max < 0) {
		unitIds = NULL;
		unitIds_max = MAX_UNITS;
	}

	std::vector<CUnit*>::const_iterator ui;
	for (ui = units.begin(); (ui != units.end()) && (a < unitIds_max); ++ui) {
		CUnit* u = *ui;

		if ((includeUnit == NULL) || (*includeUnit)(u)) {
			if (unitIds != NULL) {
				unitIds[a] = u->id;
			}
			a++;
		}
	}

	return a;
}
static int FilterUnitsList(const std::list<CUnit*>& units, int* unitIds, int unitIds_max, bool (*includeUnit)(CUnit*) = NULL)
{
	int a = 0;

	if (unitIds_max < 0) {
		unitIds = NULL;
		unitIds_max = MAX_UNITS;
	}

	std::list<CUnit*>::const_iterator ui;
	for (ui = units.begin(); (ui != units.end()) && (a < unitIds_max); ++ui) {
		CUnit* u = *ui;

		if ((includeUnit == NULL) || (*includeUnit)(u)) {
			if (unitIds != NULL) {
				unitIds[a] = u->id;
			}
			a++;
		}
	}

	return a;
}

static inline bool unit_IsNeutral(CUnit* unit) {
	return unit->IsNeutral();
}

static int myAllyTeamId = -1;

/// You have to set myAllyTeamId before callign this function. NOT thread safe!
static inline bool unit_IsEnemy(CUnit* unit) {
	return (!teamHandler->Ally(unit->allyteam, myAllyTeamId)
			&& !unit_IsNeutral(unit));
}


int CAICheats::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	myAllyTeamId = teamHandler->AllyTeam(ai->GetTeamId());
	return FilterUnitsList(uh->activeUnits, unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	const std::vector<CUnit*>& units = qf->GetUnitsExact(pos, radius);
	myAllyTeamId = teamHandler->AllyTeam(ai->GetTeamId());
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	return FilterUnitsList(uh->activeUnits, unitIds, unitIds_max, &unit_IsNeutral);
}

int CAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	const std::vector<CUnit*>& units = qf->GetUnitsExact(pos, radius);
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsNeutral);
}

int CAICheats::GetFeatures(int* features, int max) const {
	// this method is never called anyway, see SSkirmishAICallbackImpl.cpp
	return 0;
}
int CAICheats::GetFeatures(int* features, int max, const float3& pos,
			float radius) const {
	// this method is never called anyway, see SSkirmishAICallbackImpl.cpp
	return 0;
}



int CAICheats::GetUnitTeam(int unitId)
{
	int unitTeamId = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitTeamId = unit->team;
	}

	return unitTeamId;
}

int CAICheats::GetUnitAllyTeam(int unitId)
{
	int unitAllyTeamId = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitAllyTeamId = unit->allyteam;
	}

	return unitAllyTeamId;
}

float CAICheats::GetUnitHealth(int unitId)
{
	float health = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		health = unit->health;
	}

	return health;
}

float CAICheats::GetUnitMaxHealth(int unitId)
{
	float maxHealth = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		maxHealth = unit->maxHealth;
	}

	return maxHealth;
}

float CAICheats::GetUnitPower(int unitId)
{
	float power = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		power = unit->power;
	}

	return power;
}

float CAICheats::GetUnitExperience(int unitId)
{
	float experience = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		experience = unit->experience;
	}

	return experience;
}

bool CAICheats::IsUnitActivated(int unitId)
{
	bool activated = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		activated = unit->activated;
	}

	return activated;
}

bool CAICheats::UnitBeingBuilt(int unitId)
{
	bool beingBuilt = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		beingBuilt = unit->beingBuilt;
	}

	return beingBuilt;
}

bool CAICheats::GetUnitResourceInfo(int unitId, UnitResourceInfo* unitResInf)
{
	bool fetchOk = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitResInf->energyMake = unit->energyMake;
		unitResInf->energyUse  = unit->energyUse;
		unitResInf->metalMake  = unit->metalMake;
		unitResInf->metalUse   = unit->metalUse;
		fetchOk = true;
	}

	return fetchOk;
}

const CCommandQueue* CAICheats::GetCurrentUnitCommands(int unitId)
{
	const CCommandQueue* currentUnitCommands = NULL;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		currentUnitCommands = &unit->commandAI->commandQue;
	}

	return currentUnitCommands;
}

int CAICheats::GetBuildingFacing(int unitId)
{
	int buildFacing = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		buildFacing = unit->buildFacing;
	}

	return buildFacing;
}

bool CAICheats::IsUnitCloaked(int unitId)
{
	bool isCloaked = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		isCloaked = unit->isCloaked;
	}

	return isCloaked;
}

bool CAICheats::IsUnitParalyzed(int unitId)
{
	bool stunned = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		stunned = unit->stunned;
	}

	return stunned;
}


bool CAICheats::IsUnitNeutral(int unitId)
{
	bool isNeutral = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		isNeutral = unit->IsNeutral();
	}

	return isNeutral;
}


bool CAICheats::GetProperty(int id, int property, void *data)
{
	bool fetchOk = false;

	switch (property) {
		case AIVAL_UNITDEF: {
			const CUnit* unit = GetUnit(id);
			if (unit) {
				(*(const UnitDef**) data) = unit->unitDef;
				fetchOk = true;
			}
			break;
		}
		default: {
			fetchOk = false;
		}
	}

	return fetchOk;
}

bool CAICheats::GetValue(int id, void* data) const
{
	return false;
}

int CAICheats::HandleCommand(int commandId, void* data)
{
	int ret = 0; // handling failed

	switch (commandId) {
		case AIHCQuerySubVersionId: {
			ret = 1; // current version of Handle Command interface
		} break;

		case AIHCTraceRayId: {
			AIHCTraceRay* cmdData = (AIHCTraceRay*) data;

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = uh->units[cmdData->srcUID];
				const CUnit* hitUnit = NULL;

				if (srcUnit != NULL) {
					cmdData->rayLen = helper->TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, 0.0f, srcUnit, hitUnit, cmdData->flags);
					cmdData->hitUID = (hitUnit != NULL)? hitUnit->id: -1;
				}
			}

			ret = 1;
		} break;

		case AIHCFeatureTraceRayId: {
			AIHCFeatureTraceRay* cmdData = (AIHCFeatureTraceRay*) data;

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = uh->units[cmdData->srcUID];
				const CUnit* hitUnit = NULL;
				const CFeature* hitFeature = NULL;

				if (srcUnit != NULL) {
					cmdData->rayLen = helper->TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, 0.0f, srcUnit, hitUnit, cmdData->flags, &hitFeature);
					cmdData->hitFID = (hitFeature != NULL)? hitFeature->id: -1;
				}
			}

			ret = 1;
		} break;

		default: {
			ret = 0; // handling failed
		}
	}

	return ret;
}
