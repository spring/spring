/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AICheats.h"

#include "ExternalAI/SkirmishAIWrapper.h"
#include "Game/TraceRay.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/GlobalConstants.h" // needed for MAX_UNITS
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Net/GameServer.h"
#include "Game/GameSetup.h"
#include "System/myMath.h"

#include <vector>
#include <list>

#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)unitHandler->MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())

CUnit* CAICheats::GetUnit(int unitId) const {

	CUnit* unit = NULL;

	if (CHECK_UNITID(unitId)) {
		unit = unitHandler->units[unitId];
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
	} else if ((CGameSetup::GetPlayerStartingData()).size() == 1) {
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
		teamHandler->Team(ai->GetTeamId())->res.metal += amount;
}

void CAICheats::GiveMeEnergy(float amount)
{
	if (!OnlyPassiveCheats())
		teamHandler->Team(ai->GetTeamId())->res.energy += amount;
}

int CAICheats::CreateUnit(const char* name, const float3& pos)
{
	int unitId = 0;

	if (!OnlyPassiveCheats()) {
		const UnitLoadParams unitParams = {NULL, NULL, pos, ZeroVector, -1, ai->GetTeamId(), FACING_SOUTH, false, false};
		const CUnit* unit = unitLoader->LoadUnit(name, unitParams);

		if (unit) {
			unitId = unit->id;
		}
	}

	return unitId;
}

const UnitDef* CAICheats::GetUnitDef(int unitId) const
{
	const UnitDef* unitDef = NULL;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitDef = unit->unitDef;
	}

	return unitDef;
}



float3 CAICheats::GetUnitPos(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		return unit->midPos;
	}

	return ZeroVector;
}

float3 CAICheats::GetUnitVelocity(int unitId) const
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
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
	myAllyTeamId = teamHandler->AllyTeam(ai->GetTeamId());
	return FilterUnitsVector(units, unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	return FilterUnitsVector(unitHandler->activeUnits, unitIds, unitIds_max, &unit_IsNeutral);
}

int CAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, radius);
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



int CAICheats::GetUnitTeam(int unitId) const
{
	int unitTeamId = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitTeamId = unit->team;
	}

	return unitTeamId;
}

int CAICheats::GetUnitAllyTeam(int unitId) const
{
	int unitAllyTeamId = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitAllyTeamId = unit->allyteam;
	}

	return unitAllyTeamId;
}

float CAICheats::GetUnitHealth(int unitId) const
{
	float health = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		health = unit->health;
	}

	return health;
}

float CAICheats::GetUnitMaxHealth(int unitId) const
{
	float maxHealth = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		maxHealth = unit->maxHealth;
	}

	return maxHealth;
}

float CAICheats::GetUnitPower(int unitId) const
{
	float power = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		power = unit->power;
	}

	return power;
}

float CAICheats::GetUnitExperience(int unitId) const
{
	float experience = 0.0f;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		experience = unit->experience;
	}

	return experience;
}

bool CAICheats::IsUnitActivated(int unitId) const
{
	bool activated = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		activated = unit->activated;
	}

	return activated;
}

bool CAICheats::UnitBeingBuilt(int unitId) const
{
	bool beingBuilt = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		beingBuilt = unit->beingBuilt;
	}

	return beingBuilt;
}

bool CAICheats::GetUnitResourceInfo(int unitId, UnitResourceInfo* unitResInf) const
{
	bool fetchOk = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		unitResInf->energyMake = unit->resourcesMake.energy;
		unitResInf->energyUse  = unit->resourcesUse.energy;
		unitResInf->metalMake  = unit->resourcesMake.metal;
		unitResInf->metalUse   = unit->resourcesUse.metal;
		fetchOk = true;
	}

	return fetchOk;
}

const CCommandQueue* CAICheats::GetCurrentUnitCommands(int unitId) const
{
	const CCommandQueue* currentUnitCommands = NULL;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		currentUnitCommands = &unit->commandAI->commandQue;
	}

	return currentUnitCommands;
}

int CAICheats::GetBuildingFacing(int unitId) const
{
	int buildFacing = 0;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		buildFacing = unit->buildFacing;
	}

	return buildFacing;
}

bool CAICheats::IsUnitCloaked(int unitId) const
{
	bool isCloaked = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		isCloaked = unit->isCloaked;
	}

	return isCloaked;
}

bool CAICheats::IsUnitParalyzed(int unitId) const
{
	bool stunned = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		stunned = unit->IsStunned();
	}

	return stunned;
}


bool CAICheats::IsUnitNeutral(int unitId) const
{
	bool isNeutral = false;

	const CUnit* unit = GetUnit(unitId);
	if (unit) {
		isNeutral = unit->IsNeutral();
	}

	return isNeutral;
}


bool CAICheats::GetProperty(int id, int property, void *data) const
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
			AIHCTraceRay* cmdData = static_cast<AIHCTraceRay*>(data);

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = unitHandler->units[cmdData->srcUID];
				CUnit* hitUnit = NULL;
				CFeature* hitFeature = NULL;

				if (srcUnit != NULL) {
					//FIXME ignore features?
					cmdData->rayLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);
					cmdData->hitUID = (hitUnit != NULL)? hitUnit->id: -1;
				}
			}

			ret = 1;
		} break;

		case AIHCFeatureTraceRayId: {
			AIHCFeatureTraceRay* cmdData = static_cast<AIHCFeatureTraceRay*>(data);

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = unitHandler->units[cmdData->srcUID];
				CUnit* hitUnit = NULL;
				CFeature* hitFeature = NULL;

				if (srcUnit != NULL) {
					//FIXME ignore units?
					cmdData->rayLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);
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
