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
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "NetProtocol.h"
#include "GlobalUnsynced.h"
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

	for (size_t t=0; t < skirmishAIs_size; ++t) {
		skirmishAIs[t] = NULL;
	}
	hasSkirmishAIs = false;

	for (size_t t=0; t < activeTeams; ++t) {
		const SkirmishAIData* data = gameSetup->GetSkirmishAIDataForTeam(t);
		if (data != NULL) {
			team_skirmishAI[t] = *data;
		}
	}
}

CEngineOutHandler::~CEngineOutHandler() {

	for (size_t t=0; t < skirmishAIs_size; ++t) {
		delete skirmishAIs[t];
		skirmishAIs[t] = NULL;
	}
}



#define DO_FOR_SKIRMISH_AIS(FUNC)						\
		for (unsigned int t=0; t < activeTeams; ++t) {	\
			if (skirmishAIs[t]) {						\
				try {									\
					skirmishAIs[t]->FUNC;				\
				} HANDLE_EXCEPTION;						\
			}											\
		}												\


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {

	if (!hasSkirmishAIs) return;

	DO_FOR_SKIRMISH_AIS(PreDestroy())
}

void CEngineOutHandler::Load(std::istream* s) {

	if (!hasSkirmishAIs) return;

	DO_FOR_SKIRMISH_AIS(Load(s))
}

void CEngineOutHandler::Save(std::ostream* s) {

	if (!hasSkirmishAIs) return;

	DO_FOR_SKIRMISH_AIS(Save(s))
}


void CEngineOutHandler::Update() {

	if (!hasSkirmishAIs) return;

	SCOPED_TIMER("AI")
	int frame = gs->frameNum;
	DO_FOR_SKIRMISH_AIS(Update(frame))
}



#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)			\
		if (!teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {					\
			for (unsigned int t=0; t < activeTeams; ++t) {							\
				if (skirmishAIs[t] && teamHandler->AllyTeam(t) == ALLY_TEAM_ID) {	\
					try {															\
						skirmishAIs[t]->FUNC;										\
					} HANDLE_EXCEPTION;												\
				}																	\
			}																		\
		}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {

	if (!hasSkirmishAIs) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {

	if (!hasSkirmishAIs) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {

	if (!hasSkirmishAIs) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {

	if (!hasSkirmishAIs) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)			\
		if (skirmishAIs[TEAM_ID]) {						\
			try {										\
				skirmishAIs[TEAM_ID]->FUNC;				\
			} HANDLE_EXCEPTION;							\
		}


void CEngineOutHandler::UnitIdle(const CUnit& unit) {

	if (!hasSkirmishAIs) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {

	if (!hasSkirmishAIs) return;

	int teamId    = unit.team;
	int unitId    = unit.id;
	int builderId = builder? builder->id: -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {

	if (!hasSkirmishAIs) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {

	if (!hasSkirmishAIs) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam) {

	if (!hasSkirmishAIs) return;

	int newTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitGiven(unitId, oldTeam, newTeam), newTeam);
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int newTeam) {

	if (!hasSkirmishAIs) return;

	int oldTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCaptured(unitId, oldTeam, newTeam), oldTeam);
}


void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed,
		const CUnit* attacker) {

	if (!hasSkirmishAIs) return;

	int destroyedId = destroyed.id;
	int attackerId  = attacker ? attacker->id : 0;

	for (unsigned int t=0; t < activeTeams; ++t) {
		if (skirmishAIs[t]
				&& !teamHandler->Ally(teamHandler->AllyTeam(t), destroyed.allyteam)
				&& (skirmishAIs[t]->IsCheatEventsEnabled()
					|| (destroyed.losStatus[teamHandler->AllyTeam(t)] & (LOS_INLOS | LOS_INRADAR)))) {
			try {
				skirmishAIs[t]->EnemyDestroyed(destroyedId, attackerId);
			} HANDLE_EXCEPTION;
		}
	}
	if (skirmishAIs[destroyed.team]) {
		try {
			skirmishAIs[destroyed.team]->UnitDestroyed(destroyedId, attackerId);
		} HANDLE_EXCEPTION;
	}
}


void CEngineOutHandler::UnitDamaged(const CUnit& damaged, const CUnit* attacker,
		float damage, int weaponDefId, bool paralyzer) {

	if (!hasSkirmishAIs) return;

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

	if (skirmishAIs[dt]) {
		try {
			skirmishAIs[dt]->UnitDamaged(damagedUnitId,
					attackerUnitId, damage, attackDir_damagedsView, weaponDefId, paralyzer);
		} HANDLE_EXCEPTION;
	}

	if (attacker) {
		if (skirmishAIs[at]
				&& !teamHandler->Ally(teamHandler->AllyTeam(at), damaged.allyteam)
				&& (skirmishAIs[at]->IsCheatEventsEnabled()
					|| (damaged.losStatus[teamHandler->AllyTeam(at)] & (LOS_INLOS | LOS_INRADAR)))) {
			try {
				skirmishAIs[at]->EnemyDamaged(damagedUnitId, attackerUnitId,
						damage, attackDir_attackersView, weaponDefId, paralyzer);
			} HANDLE_EXCEPTION;
		}
	}
}


void CEngineOutHandler::SeismicPing(int allyTeamId, const CUnit& unit,
		const float3& pos, float strength) {

	if (!hasSkirmishAIs) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {

	if (!hasSkirmishAIs) return;

	int teamId = unit.team;
	int unitId = unit.id;
	int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	if (!hasSkirmishAIs) return;

	int teamId = playerHandler->Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {

	if (!hasSkirmishAIs) return;

	int teamId           = unit.team;
	int unitId           = unit.id;
	int aiCommandTopicId = extractAICommandTopic(&command);

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::GotChatMsg(const char* msg, int fromPlayerId) {

	if (!hasSkirmishAIs) return;

	DO_FOR_SKIRMISH_AIS(GotChatMsg(msg, fromPlayerId))
}



bool CEngineOutHandler::CreateSkirmishAI(int teamId, const SkirmishAIKey& key, const SkirmishAIData& data) {

	const bool unpauseAfterAIInit = configHandler->Get("AI_UnpauseAfterInit", true);

	if (!teamHandler->IsValidTeam(teamId)) {
		return false;
	}

	try {
		// Pause the game for letting the AI initialize,
		// as this can take quite some time.
		bool weDoPause = !gs->paused;
		if (weDoPause) {
			const std::string& myPlayerName = playerHandler->Player(gu->myPlayerNum)->name;
			logOutput.Print(
					"Player %s (auto)-paused the game for letting Skirmish AI"
					" %s initialize for controlling team %i.%s",
					myPlayerName.c_str(), key.GetShortName().c_str(), teamId,
					(unpauseAfterAIInit ?
					 " The game is auto-unpaused as soon as the AI is ready." :
					 ""));
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
		}

		if (skirmishAIs[teamId]) {
			DestroySkirmishAI(teamId);
		}

		team_skirmishAI[teamId] = data;
		skirmishAIs[teamId] = new CSkirmishAIWrapper(teamId, key);
		skirmishAIs[teamId]->Init();
		hasSkirmishAIs = true;

		// Send a UnitCreated event for each unit of the team.
		// This will only do something if the AI is created mid-game.
		CTeam* team = teamHandler->Team(teamId);
		CUnitSet::iterator u, uNext;
		for (u = team->units.begin(); u != team->units.end(); ) {
			uNext = u; ++uNext;
			skirmishAIs[teamId]->UnitFinished((*u)->id);
			u = uNext;
		}

		// Unpause the game again, if we paused it and it is still paused.
		if (unpauseAfterAIInit && gs->paused && weDoPause) {
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, false));
		}

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

void CEngineOutHandler::DestroySkirmishAI(int teamId, int reason) {

	try {
		if (IsSkirmishAI(teamId)) {
			skirmishAIs[teamId]->Release(reason);
			delete skirmishAIs[teamId];
			skirmishAIs[teamId] = NULL;
			team_skirmishAI.erase(teamId);
		}
	} HANDLE_EXCEPTION;
}

const SkirmishAIData* CEngineOutHandler::GetSkirmishAIData(int teamId) const {

	std::map<int, SkirmishAIData>::const_iterator sad;
	sad = team_skirmishAI.find(teamId);
	if (sad == team_skirmishAI.end()) {
		return NULL;
	} else {
		return &(sad->second);
	}
}
