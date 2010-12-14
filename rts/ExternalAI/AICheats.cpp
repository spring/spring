/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AICheats.h"

#include "StdAfx.h"
#include "ExternalAI/SkirmishAIWrapper.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
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

int CAICheats::CreateUnit(const char* name, float3 pos)
{
	if (!OnlyPassiveCheats()) {
		CUnit* u = unitLoader->LoadUnit(name, pos, ai->GetTeamId(), false, 0, NULL);
		if (u)
			return u->id;
	}
	return 0;
}

const UnitDef* CAICheats::GetUnitDef(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->unitDef;
	}
	return 0;

}



float3 CAICheats::GetUnitPos(int unitid)
{
	if (!CHECK_UNITID(unitid)) return ZeroVector;
	const CUnit* unit = uh->units[unitid];
	if (unit != NULL) {
		return unit->pos;
	}
	return ZeroVector;
}

float3 CAICheats::GetUnitVelocity(int unitid)
{
	if (!CHECK_UNITID(unitid)) return ZeroVector;
	const CUnit* unit = uh->units[unitid];
	if (unit != NULL) {
		return unit->speed;
	}
	return ZeroVector;
}


static int FilterUnitsVector(const std::vector<CUnit*>& units, int* unitIds, int unitIds_max, bool (*includeUnit)(CUnit*) = NULL)
{
	int a = 0;

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



int CAICheats::GetUnitTeam(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->team;
	}
	return 0;
}

int CAICheats::GetUnitAllyTeam(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->allyteam;
	}
	return 0;
}

float CAICheats::GetUnitHealth(int unitid)			//the units current health
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->health;
	}
	return 0;
}

float CAICheats::GetUnitMaxHealth(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->maxHealth;
	}
	return 0;
}

float CAICheats::GetUnitPower(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->power;
	}
	return 0;
}

float CAICheats::GetUnitExperience(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->experience;
	}
	return 0;
}

bool CAICheats::IsUnitActivated(int unitid)
{
	if (!CHECK_UNITID(unitid)) return false;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->activated;
	}
	return false;
}

bool CAICheats::UnitBeingBuilt(int unitid)
{
	if (!CHECK_UNITID(unitid)) return false;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->beingBuilt;
	}
	return false;
}

bool CAICheats::GetUnitResourceInfo(int unitid, UnitResourceInfo *i)
{
	if (!CHECK_UNITID(unitid)) return false;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		i->energyMake = unit->energyMake;
		i->energyUse = unit->energyUse;
		i->metalMake = unit->metalMake;
		i->metalUse = unit->metalUse;
		return true;
	}
	return false;
}

const CCommandQueue* CAICheats::GetCurrentUnitCommands(int unitid)
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return &unit->commandAI->commandQue;
	}
	return 0;
}

int CAICheats::GetBuildingFacing(int unitid) {
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->buildFacing;
	}
	return 0;
}

bool CAICheats::IsUnitCloaked(int unitid) {
	if (!CHECK_UNITID(unitid)) return false;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->isCloaked;
	}
	return false;
}

bool CAICheats::IsUnitParalyzed(int unitid) {
	if (!CHECK_UNITID(unitid))
		return false;

	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->stunned;
	}

	return false;
}


bool CAICheats::IsUnitNeutral(int unitid) {
	if (!CHECK_UNITID(unitid))
		return false;

	CUnit* unit = uh->units[unitid];
	if (unit) {
		return (unit->IsNeutral());
	}

	return false;
}


bool CAICheats::GetProperty(int id, int property, void *data)
{
	switch (property) {
		case AIVAL_UNITDEF: {
			if (!CHECK_UNITID(id)) return false;
			CUnit* unit = uh->units[id];
			if (unit) {
				(*(const UnitDef**) data) = unit->unitDef;
				return true;
			}
			break;
		}
		default:
			return false;
	}
	return false; // never reached
}

bool CAICheats::GetValue(int id, void* data) const
{
	return false;
}

int CAICheats::HandleCommand(int commandId, void* data)
{
	switch (commandId) {
		case AIHCQuerySubVersionId:
			return 1; // current version of Handle Command interface

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

			return 1;
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

			return 1;
		} break;

		default: {
			return 0;
		}
	}
}
