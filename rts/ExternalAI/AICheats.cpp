#include <vector>
#include <list>

#include "StdAfx.h"
#include "AICheats.h"
#include "SkirmishAIWrapper.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/GameServer.h"
#include "Game/GameSetup.h"
#include "mmgr.h"

#define CHECK_UNITID(id) ((unsigned)(id) < (unsigned)uh->MaxUnits())
#define CHECK_GROUPID(id) ((unsigned)(id) < (unsigned)gh->groups.size())


CAICheats::CAICheats(CSkirmishAIWrapper* ai): ai(ai)
{
}

CAICheats::~CAICheats(void)
{}

bool CAICheats::OnlyPassiveCheats()
{
	return IsPassive();
}
bool CAICheats::IsPassive()
{
	if (!gameServer) {
		// if we are NOT server, cheats will cause desync
		return true;
	}
	else if (gameSetup && (gameSetup->playerStartingData.size() == 1)) {
		// assuming AI's dont count on numPlayers
		return false;
	}
	else {
		// disable it in case we are not sure
		return true;
	}
}

void CAICheats::EnableCheatEvents(bool enable)
{
	ai->SetCheatEventsEnabled(enable);
}

void CAICheats::SetMyHandicap(float handicap)
{
	if (!OnlyPassiveCheats()) {
		teamHandler->Team(ai->GetTeamId())->handicap = 1 + handicap / 100;
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
		CUnit* u = unitLoader.LoadUnit(name, pos, ai->GetTeamId(), false, 0, NULL);
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
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->pos;
	}
	return ZeroVector;

}

int CAICheats::GetEnemyUnits(int* unitIds, int unitIds_max)
{
	std::list<CUnit*>::iterator ui;
	int a = 0;

	for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		CUnit* u = *ui;

		if (!teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(ai->GetTeamId()))) {
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}

int CAICheats::GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	std::vector<CUnit*> unit = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::iterator ui;
	int a = 0;

	for (ui = unit.begin(); ui != unit.end(); ++ui) {
		CUnit* u = *ui;

		if (!teamHandler->Ally(u->allyteam, teamHandler->AllyTeam(ai->GetTeamId()))) {
			if (!IsUnitNeutral(u->id)) {
				unitIds[a++] = u->id;
				if (a >= unitIds_max) {
					break;
				}
			}
		}
	}

	return a;
}



int CAICheats::GetNeutralUnits(int* unitIds, int unitIds_max)
{
	int a = 0;

	for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
		CUnit* u = *ui;

		if (IsUnitNeutral(u->id)) {
			unitIds[a++] = u->id;
			if (a >= unitIds_max) {
				break;
			}
		}
	}

	return a;
}

int CAICheats::GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max)
{
	std::vector<CUnit*> unit = qf->GetUnitsExact(pos, radius);
	std::vector<CUnit*>::iterator ui;
	int a = 0;

	for (ui = unit.begin(); ui != unit.end(); ++ui) {
		CUnit* u = *ui;

		if (IsUnitNeutral(u->id)) {
			unitIds[a++] = u->id;
			if (a >= unitIds_max) {
				break;
			}
		}
	}

	return a;
}

int CAICheats::GetFeatures(int *features, int max) {
	// this method is never called anyway, see SSkirmishAICallbackImpl.cpp
	return 0;
}
int CAICheats::GetFeatures(int *features, int max, const float3& pos,
			float radius) {
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

float CAICheats::GetUnitMaxHealth(int unitid)		//the units max health
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->maxHealth;
	}
	return 0;
}

float CAICheats::GetUnitPower(int unitid)				//sort of the measure of the units overall power
{
	if (!CHECK_UNITID(unitid)) return 0;
	CUnit* unit = uh->units[unitid];
	if (unit) {
		return unit->power;
	}
	return 0;
}

float CAICheats::GetUnitExperience(int unitid)	//how experienced the unit is (0.0-1.0)
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

bool CAICheats::GetValue(int id, void* data)
{
	return false;
}

int CAICheats::HandleCommand(int commandId, void *data)
{
	switch (commandId) {
		case AIHCQuerySubVersionId:
			return 1; // current version of Handle Command interface

		case AIHCTraceRayId: {
			AIHCTraceRay* cmdData = (AIHCTraceRay*) data;

			if (CHECK_UNITID(cmdData->srcUID)) {
				CUnit* srcUnit = uh->units[cmdData->srcUID];
				CUnit* hitUnit = NULL;

				if (srcUnit != NULL) {
					cmdData->rayLen = helper->TraceRay(cmdData->rayPos, cmdData->rayDir, cmdData->rayLen, 0.0f, srcUnit, hitUnit, cmdData->flags);
					cmdData->hitUID = (hitUnit != NULL)? hitUnit->id: -1;
				}
			}

			return 1;
		}

		default:
			return 0;
	}
}
