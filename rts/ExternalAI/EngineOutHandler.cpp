/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software {} you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation {} either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY {} without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "EngineOutHandler.h"
#include "SkirmishAIWrapper.h"
#include "Interface/AISCommands.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "Util.h"
#include "TimeProfiler.h"


CR_BIND_DERIVED(CEngineOutHandler,CObject, )

CR_REG_METADATA(CEngineOutHandler, (
				CR_MEMBER(skirmishAIs),
				CR_MEMBER(hasSkirmishAIs)
				));

/////////////////////////////
// BEGIN: Exception Handling

bool CEngineOutHandler::IsCatchExceptions() {

	static bool init = false;
	static bool isCatchExceptions;

	if (!init) {
		isCatchExceptions = configHandler->Get("CatchAIExceptions", 1) != 0;
		init = true;
	}

	return isCatchExceptions;
}

// to switch off the exception handling and have it catched by the debugger.
#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {		\
		if (IsCatchExceptions()) {			\
			handleAIException(e.what());	\
			throw;							\
		} else throw;						\
	}										\
	catch (const char *s) {					\
		if (IsCatchExceptions()) {			\
			handleAIException(s);			\
			throw;							\
		} else throw;						\
	}										\
	catch (...) {							\
		if (IsCatchExceptions()) {			\
			handleAIException(0);			\
			throw;							\
		} else throw;						\
	}

void handleAIException(const char* description) {

	if (description) {
		logOutput.Print("AI Exception: \'%s\'", description);
	} else {
		logOutput.Print("AI Exception");
	}
//	exit(-1);
}

// END: Exception Handling
/////////////////////////////


CEngineOutHandler* CEngineOutHandler::singleton = NULL;

void CEngineOutHandler::Initialize() {

	if (singleton == NULL) {
		singleton = new CEngineOutHandler();
	}
}
CEngineOutHandler* CEngineOutHandler::GetInstance() {

	if (singleton == NULL) {
		Initialize();
	}

	return singleton;
}
void CEngineOutHandler::Destroy() {

	if (singleton != NULL) {
		CEngineOutHandler* tmp = singleton;
		singleton = NULL;
		delete tmp;
		tmp = NULL;
	}
}

CEngineOutHandler::CEngineOutHandler()
		: activeTeams(teamHandler->ActiveTeams()) {

	for (int t=0; t < skirmishAIs_size; ++t) {
		skirmishAIs[t] = NULL;
	}
	hasSkirmishAIs = false;
}

CEngineOutHandler::~CEngineOutHandler() {

	for (int t=0; t < skirmishAIs_size; ++t) {
		delete skirmishAIs[t];
		skirmishAIs[t] = NULL;
	}
}



#define DO_FOR_SKIRMISH_AIS(FUNC)						\
		if (hasSkirmishAIs) {								\
			for (unsigned int t=0; t < activeTeams; ++t) {	\
				if (skirmishAIs[t]) {						\
					skirmishAIs[t]->FUNC;					\
				}											\
			}												\
		}


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {

	try {
		DO_FOR_SKIRMISH_AIS(PreDestroy())
	} HANDLE_EXCEPTION;
}

void CEngineOutHandler::Load(std::istream* s) {

	try {
		DO_FOR_SKIRMISH_AIS(Load(s))
	} HANDLE_EXCEPTION;
}

void CEngineOutHandler::Save(std::ostream* s) {

	try {
		DO_FOR_SKIRMISH_AIS(Save(s))
	} HANDLE_EXCEPTION;
}


void CEngineOutHandler::Update() {

	SCOPED_TIMER("AI")
	try {
		int frame = gs->frameNum;
		DO_FOR_SKIRMISH_AIS(Update(frame))
	} HANDLE_EXCEPTION;
}



#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)			\
	if (hasSkirmishAIs && !teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {	\
		for (unsigned int t=0; t < activeTeams; ++t) {								\
			if (skirmishAIs[t] && teamHandler->AllyTeam(t) == ALLY_TEAM_ID) {		\
				try {																\
					skirmishAIs[t]->FUNC;											\
				} HANDLE_EXCEPTION;													\
			}																		\
		}																			\
	}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)		\
	if (skirmishAIs[TEAM_ID]) {						\
		try {										\
			skirmishAIs[TEAM_ID]->FUNC;				\
		} HANDLE_EXCEPTION;							\
	}


void CEngineOutHandler::UnitIdle(const CUnit& unit) {

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {

	int teamId    = unit.team;
	int unitId    = unit.id;
	int builderId = builder? builder->id: -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam) {

	int newTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitGiven(unitId, oldTeam, newTeam), newTeam);
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int newTeam) {

	int oldTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCaptured(unitId, oldTeam, newTeam), oldTeam);
}


void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed,
		const CUnit* attacker) {

	int destroyedId = destroyed.id;
	int attackerId  = attacker ? attacker->id : 0;

	if (hasSkirmishAIs) {
		try {
			for (unsigned int t=0; t < activeTeams; ++t) {
				if (skirmishAIs[t]
						&& !teamHandler->Ally(teamHandler->AllyTeam(t), destroyed.allyteam)
						&& (skirmishAIs[t]->IsCheatEventsEnabled()
							|| (destroyed.losStatus[t] & (LOS_INLOS | LOS_INRADAR)))) {
					skirmishAIs[t]->EnemyDestroyed(destroyedId, attackerId);
				}
			}
			if (skirmishAIs[destroyed.team]) {
				skirmishAIs[destroyed.team]->UnitDestroyed(destroyedId, attackerId);
			}
		} HANDLE_EXCEPTION;
	}
}


void CEngineOutHandler::UnitDamaged(const CUnit& damaged, const CUnit* attacker,
		float damage) {

	int damagedUnitId  = damaged.id;
	int attackerUnitId = attacker ? attacker->id : -1;

	float3 attackDir_damagedsView;
	float3 attackDir_attackersView;
	if (attacker) {
		attackDir_damagedsView =
				helper->GetUnitErrorPos(attacker, damaged.allyteam)
				- damaged.pos;
		attackDir_damagedsView.ANormalize();

		attackDir_attackersView =
				attacker->pos
				- helper->GetUnitErrorPos(&damaged, attacker->allyteam);
		attackDir_attackersView.ANormalize();
	} else {
		attackDir_damagedsView = ZeroVector;
	}
	int dt = damaged.team;
	int at = attacker ? attacker->team : -1;

	if (hasSkirmishAIs) {
		try {
			if (skirmishAIs[dt]) {
				skirmishAIs[dt]->UnitDamaged(damagedUnitId,
						attackerUnitId, damage, attackDir_damagedsView);
			}

			if (attacker) {
				if (skirmishAIs[at]
						&& !teamHandler->Ally(teamHandler->AllyTeam(at), damaged.allyteam)
						&& (skirmishAIs[at]->IsCheatEventsEnabled()
							|| (damaged.losStatus[at] & (LOS_INLOS | LOS_INRADAR)))) {
					skirmishAIs[at]->EnemyDamaged(damagedUnitId, attackerUnitId,
							damage, attackDir_attackersView);
				}
			}
		} HANDLE_EXCEPTION;
	}
}


void CEngineOutHandler::SeismicPing(int allyTeamId, const CUnit& unit,
		const float3& pos, float strength) {

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {

	int teamId = unit.team;
	int unitId = unit.id;
	int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	int teamId = playerHandler->Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {

	int teamId           = unit.team;
	int unitId           = unit.id;
	// save some CPU time if there are no Skirmish AIs in use
	int aiCommandTopicId = hasSkirmishAIs ? extractAICommandTopic(&command) : -1;

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::GotChatMsg(const char* msg, int fromPlayerId) {

	DO_FOR_SKIRMISH_AIS(GotChatMsg(msg, fromPlayerId))
}



bool CEngineOutHandler::CreateSkirmishAI(int teamId, const SkirmishAIKey& key) {

	if ((teamId < 0) || (teamId >= (int)activeTeams)) {
		return false;
	}

	try {
		if (skirmishAIs[teamId]) {
			delete skirmishAIs[teamId];
			skirmishAIs[teamId] = NULL;
		}

		skirmishAIs[teamId] = new CSkirmishAIWrapper(teamId, key);
		skirmishAIs[teamId]->Init();
		hasSkirmishAIs = true;

		return true;
	} HANDLE_EXCEPTION;

	return false;
}

const SkirmishAIKey* CEngineOutHandler::GetSkirmishAIKey(int teamId) const {

	if (IsSkirmishAI(teamId)) {
		return &(skirmishAIs[teamId]->GetKey());
	} else {
		return NULL;
	}
}

bool CEngineOutHandler::IsSkirmishAI(int teamId) const {
	return skirmishAIs[teamId] != NULL;
}

const SSkirmishAICallback* CEngineOutHandler::GetSkirmishAICallback(int teamId) const {

	if (IsSkirmishAI(teamId)) {
		return skirmishAIs[teamId]->GetCallback();
	} else {
		return NULL;
	}
}

void CEngineOutHandler::DestroySkirmishAI(int teamId) {

	try {
		if (IsSkirmishAI(teamId)) {
			delete skirmishAIs[teamId];
			skirmishAIs[teamId] = NULL;
		}
	} HANDLE_EXCEPTION;
}
