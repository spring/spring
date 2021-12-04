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
#include "System/SpringMath.h"

#include <vector>

#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)unitHandler.MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())

CUnit* CAICheats::GetUnit(int unitId) const
{
	if (CHECK_UNITID(unitId))
		return unitHandler.GetUnit(unitId);

	return nullptr;
}


bool CAICheats::OnlyPassiveCheats()
{
	// returns whether cheats will desync (this is
	// always the case unless we are both the host
	// and the only client) if used by an AI
	if (gameServer == nullptr)
		return true;
	// assumes AI's dont count toward numPlayers and disable it in case we are not sure
	return ((CGameSetup::GetPlayerStartingData()).size() != 1);
}

void CAICheats::EnableCheatEvents(bool enable)
{
	// enable sending of EnemyCreated, etc. events
	ai->SetCheatEvents(enable);
}



void CAICheats::SetMyIncomeMultiplier(float incomeMultiplier)
{
	if (OnlyPassiveCheats())
		return;

	teamHandler.Team(ai->GetTeamId())->SetIncomeMultiplier(incomeMultiplier);
}

void CAICheats::GiveMeMetal(float amount)
{
	if (OnlyPassiveCheats())
		return;

	teamHandler.Team(ai->GetTeamId())->res.metal += amount;
}

void CAICheats::GiveMeEnergy(float amount)
{
	if (OnlyPassiveCheats())
		return;

	teamHandler.Team(ai->GetTeamId())->res.energy += amount;
}

int CAICheats::CreateUnit(const char* name, const float3& pos)
{
	if (OnlyPassiveCheats())
		return 0;

	const UnitLoadParams unitParams = {nullptr, nullptr, pos, ZeroVector, -1, ai->GetTeamId(), FACING_SOUTH, false, false};
	const CUnit* unit = unitLoader->LoadUnit(name, unitParams);

	if (unit != nullptr)
		return unit->id;

	return 0;
}

const UnitDef* CAICheats::GetUnitDef(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->unitDef;

	return nullptr;
}



float3 CAICheats::GetUnitPos(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->midPos;

	return ZeroVector;
}

float3 CAICheats::GetUnitVelocity(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->speed;

	return ZeroVector;
}


static int FilterUnitsVector(const std::vector<CUnit*>& units, int* unitIds, int unitIds_max, bool (*includeUnit)(CUnit*) = nullptr)
{
	int a = 0;

	if (unitIds_max < 0) {
		unitIds = nullptr;
		unitIds_max = MAX_UNITS;
	}

	for (auto ui = units.begin(); (ui != units.end()) && (a < unitIds_max); ++ui) {
		CUnit* u = *ui;

		if ((includeUnit == nullptr) || (*includeUnit)(u)) {
			if (unitIds != nullptr)
				unitIds[a] = u->id;

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
	return (!teamHandler.Ally(unit->allyteam, myAllyTeamId) && !unit_IsNeutral(unit));
}


int CAICheats::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	myAllyTeamId = teamHandler.AllyTeam(ai->GetTeamId());
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);
	myAllyTeamId = teamHandler.AllyTeam(ai->GetTeamId());
	return FilterUnitsVector(*qfQuery.units, unitIds, unitIds_max, &unit_IsEnemy);
}

int CAICheats::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	return FilterUnitsVector(unitHandler.GetActiveUnits(), unitIds, unitIds_max, &unit_IsNeutral);
}

int CAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos, radius);
	return FilterUnitsVector(*qfQuery.units, unitIds, unitIds_max, &unit_IsNeutral);
}

int CAICheats::GetFeatures(int* features, int max) const {
	// this method is never called anyway, see SSkirmishAICallbackImpl.cpp
	return 0;
}
int CAICheats::GetFeatures(int* features, int max, const float3& pos, float radius) const {
	// this method is never called anyway, see SSkirmishAICallbackImpl.cpp
	return 0;
}



int CAICheats::GetUnitTeam(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->team;

	return 0;
}

int CAICheats::GetUnitAllyTeam(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->allyteam;

	return 0;
}

float CAICheats::GetUnitHealth(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->health;

	return 0.0f;
}

float CAICheats::GetUnitMaxHealth(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->maxHealth;

	return 0.0f;
}

float CAICheats::GetUnitPower(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->power;

	return 0.0f;
}

float CAICheats::GetUnitExperience(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->experience;

	return 0.0f;
}

bool CAICheats::IsUnitActivated(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->activated;

	return false;
}

bool CAICheats::UnitBeingBuilt(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->beingBuilt;

	return false;
}

bool CAICheats::GetUnitResourceInfo(int unitId, UnitResourceInfo* unitResInf) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit == nullptr)
		return false;

	unitResInf->energyMake = unit->resourcesMake.energy;
	unitResInf->energyUse  = unit->resourcesUse.energy;
	unitResInf->metalMake  = unit->resourcesMake.metal;
	unitResInf->metalUse   = unit->resourcesUse.metal;
	return true;
}

const CCommandQueue* CAICheats::GetCurrentUnitCommands(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return &unit->commandAI->commandQue;

	return nullptr;
}

int CAICheats::GetBuildingFacing(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->buildFacing;

	return 0;
}

bool CAICheats::IsUnitCloaked(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->isCloaked;

	return false;
}

bool CAICheats::IsUnitParalyzed(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->IsStunned();

	return false;
}


bool CAICheats::IsUnitNeutral(int unitId) const
{
	const CUnit* unit = GetUnit(unitId);

	if (unit != nullptr)
		return unit->IsNeutral();

	return false;
}


bool CAICheats::GetProperty(int id, int property, void* data) const
{
	switch (property) {
		case AIVAL_UNITDEF: {
			const CUnit* unit = GetUnit(id);

			if (unit != nullptr) {
				(*(const UnitDef**) data) = unit->unitDef;
				return true;
			}
		} break;

		default: {
		} break;
	}

	return false;
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
				const CUnit* srcUnit = unitHandler.GetUnit(cmdData->srcUID);

				CUnit* hitUnit = nullptr;
				CFeature* hitFeature = nullptr;

				if (srcUnit != nullptr) {
					//FIXME ignore features?
					cmdData->rayLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);
					cmdData->hitUID = (hitUnit != nullptr)? hitUnit->id: -1;
				}
			}

			ret = 1;
		} break;

		case AIHCFeatureTraceRayId: {
			AIHCFeatureTraceRay* cmdData = static_cast<AIHCFeatureTraceRay*>(data);

			if (CHECK_UNITID(cmdData->srcUID)) {
				const CUnit* srcUnit = unitHandler.GetUnit(cmdData->srcUID);

				CUnit* hitUnit = nullptr;
				CFeature* hitFeature = nullptr;

				if (srcUnit != nullptr) {
					//FIXME ignore units?
					cmdData->rayLen = TraceRay::TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, cmdData->flags, srcUnit, hitUnit, hitFeature);
					cmdData->hitFID = (hitFeature != nullptr)? hitFeature->id: -1;
				}
			}

			ret = 1;
		} break;

		default: {
			ret = 0; // handling failed
		} break;
	}

	return ret;
}

