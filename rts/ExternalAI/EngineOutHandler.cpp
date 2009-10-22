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

#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "NetProtocol.h"
#include "GlobalUnsynced.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "Util.h"
#include "TimeProfiler.h"

#include "creg/STL_Map.h"


CR_BIND_DERIVED(CEngineOutHandler, CObject, )

CR_REG_METADATA(CEngineOutHandler, (
				CR_MEMBER(id_skirmishAI),
				CR_MEMBER(team_skirmishAIs),
				CR_RESERVED(128)
				));

/////////////////////////////
// BEGIN: Exception Handling

bool CEngineOutHandler::IsCatchExceptions() {

	static bool init = false;
	static bool isCatchExceptions;

	if (!init) {
		isCatchExceptions = (configHandler->Get("CatchAIExceptions", 1) != 0);
		init = true;
	}

	return isCatchExceptions;
}

void CEngineOutHandler::HandleAIException(const char* description) {

	if (CEngineOutHandler::IsCatchExceptions()) {
		if (description) {
			logOutput.Print("AI Exception: \'%s\'", description);
		} else {
			logOutput.Print("AI Exception");
		}
//		exit(-1);
	}
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
{
}

CEngineOutHandler::~CEngineOutHandler() {

	// id_skirmishAI should be empty already, but this can not hurt
	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		delete ai->second;
	}
}


// This macro should be insterted at the start of each method sending AI events
#define AI_EVT_MTH()                           \
		if (id_skirmishAI.size() == 0) return; \
		SCOPED_TIMER("AI Total");


#define DO_FOR_SKIRMISH_AIS(FUNC)                          \
		for (id_ai_t::iterator ai = id_skirmishAI.begin(); \
				ai != id_skirmishAI.end(); ++ai) {         \
			try {                                          \
				ai->second->FUNC;                          \
			} CATCH_AI_EXCEPTION;                          \
		}


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(PreDestroy())
}

void CEngineOutHandler::Load(std::istream* s) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(Load(s))
}

void CEngineOutHandler::Save(std::ostream* s) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(Save(s))
}


void CEngineOutHandler::Update() {
	AI_EVT_MTH();

	const int frame = gs->frameNum;

	DO_FOR_SKIRMISH_AIS(Update(frame))
}



// Do only if the unit is not allied, in which case we know
// everything about it anyway, and do not need to be informed
#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)				\
		if (!teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {						\
			for (id_ai_t::iterator ai = id_skirmishAI.begin();							\
					ai != id_skirmishAI.end(); ++ai) {									\
				const int aiAllyTeam = teamHandler->AllyTeam(ai->second->GetTeamId());	\
				if (teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID)) {						\
					try {																\
						ai->second->FUNC;												\
					} CATCH_AI_EXCEPTION;												\
				}																		\
			}																			\
		}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)								\
		if (team_skirmishAIs.find(TEAM_ID) != team_skirmishAIs.end()) {		\
			for (ids_t::iterator ai = team_skirmishAIs[TEAM_ID].begin();	\
					ai != team_skirmishAIs[TEAM_ID].end(); ++ai) {			\
				try {														\
					id_skirmishAI[*ai]->FUNC;								\
				} CATCH_AI_EXCEPTION;										\
			}																\
		}


void CEngineOutHandler::UnitIdle(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {
	AI_EVT_MTH();

	const int teamId    = unit.team;
	const int unitId    = unit.id;
	const int builderId = builder? builder->id: -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam) {
	AI_EVT_MTH();

	const int newTeam = unit.team;
	const int unitId  = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitGiven(unitId, oldTeam, newTeam), newTeam);
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int newTeam) {
	AI_EVT_MTH();

	const int oldTeam = unit.team;
	const int unitId  = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCaptured(unitId, oldTeam, newTeam), oldTeam);
}

static inline bool isUnitInLosOrRadarOfAllyTeam(const CUnit& unit, const int allyTeamId) {
	return unit.losStatus[allyTeamId] & (LOS_INLOS | LOS_INRADAR);
}

void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed,
		const CUnit* attacker) {
	AI_EVT_MTH();

	const int destroyedId = destroyed.id;
	const int attackerId  = attacker ? attacker->id : -1;
	const int dt          = destroyed.team;

	// inform destroyed units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		const bool attackerInLosOrRadar = attacker && isUnitInLosOrRadarOfAllyTeam(*attacker, destroyed.allyteam);
		for (ids_t::iterator ai = team_skirmishAIs[dt].begin(); ai != team_skirmishAIs[dt].end(); ++ai) {
			CSkirmishAIWrapper* saw = id_skirmishAI[*ai];
			int visibleAttackerId = -1;
			if (attackerInLosOrRadar || saw->IsCheatEventsEnabled()) {
				visibleAttackerId = attackerId;
			}
			try {
				saw->UnitDestroyed(destroyedId, visibleAttackerId);
			} CATCH_AI_EXCEPTION;
		}
	}

	// inform attacker units team (including allies)
	if (attacker != NULL && !teamHandler->Ally(attacker->allyteam, destroyed.allyteam)) {
		for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
			const int t     = ai->second->GetTeamId();
			const int allyT = teamHandler->AllyTeam(t);
			if (teamHandler->Ally(allyT, attacker->allyteam)
				&& (ai->second->IsCheatEventsEnabled() || isUnitInLosOrRadarOfAllyTeam(destroyed, allyT)))
			{
				try {
					ai->second->EnemyDestroyed(destroyedId, attackerId);
				} CATCH_AI_EXCEPTION;
			}
		}
	}
}


void CEngineOutHandler::UnitDamaged(const CUnit& damaged, const CUnit* attacker,
		float damage, int weaponDefId, bool paralyzer) {
	AI_EVT_MTH();

	const int damagedUnitId  = damaged.id;
	const int dt             = damaged.team;
	const int attackerUnitId = attacker ? attacker->id : -1;

	// inform damaged units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		float3 attackDir_damagedsView = ZeroVector;
		if (attacker) {
			attackDir_damagedsView =
					helper->GetUnitErrorPos(attacker, damaged.allyteam)
					- damaged.pos;
			attackDir_damagedsView.ANormalize();
		}
		const bool attackerInLosOrRadar = attacker && isUnitInLosOrRadarOfAllyTeam(*attacker, damaged.allyteam);
		for (ids_t::iterator ai = team_skirmishAIs[dt].begin(); ai != team_skirmishAIs[dt].end(); ++ai) {
			CSkirmishAIWrapper* saw = id_skirmishAI[*ai];
			int visibleAttackerUnitId = -1;
			if (attackerInLosOrRadar || saw->IsCheatEventsEnabled()) {
				visibleAttackerUnitId = attackerUnitId;
			}
			try {
				id_skirmishAI[*ai]->UnitDamaged(damagedUnitId, visibleAttackerUnitId, damage, attackDir_damagedsView, weaponDefId, paralyzer);
			} CATCH_AI_EXCEPTION;
		}
	}

	// inform attacker units team (not allies)
	if (attacker) {
		const int at = attacker ? attacker->team : -1;
		if (!teamHandler->Ally(attacker->allyteam, damaged.allyteam)
				&& (team_skirmishAIs.find(at) != team_skirmishAIs.end())) {
			float3 attackDir_attackersView = attacker->pos
						- helper->GetUnitErrorPos(&damaged, attacker->allyteam);
			attackDir_attackersView.ANormalize();
			const bool damagedInLosOrRadar = isUnitInLosOrRadarOfAllyTeam(damaged, attacker->allyteam);
			for (ids_t::iterator ai = team_skirmishAIs[at].begin(); ai != team_skirmishAIs[at].end(); ++ai)
			{
				CSkirmishAIWrapper* saw = id_skirmishAI[*ai];
				if (damagedInLosOrRadar || saw->IsCheatEventsEnabled())
				{
					try {
						saw->EnemyDamaged(damagedUnitId, attackerUnitId, damage,
								attackDir_attackersView, weaponDefId, paralyzer);
					} CATCH_AI_EXCEPTION;
				}
			}
		}
	}
}


void CEngineOutHandler::SeismicPing(int allyTeamId, const CUnit& unit,
		const float3& pos, float strength) {
	AI_EVT_MTH();

	const int unitId         = unit.id;
	const int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;
	const int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	AI_EVT_MTH();

	const int teamId = playerHandler->Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {
	AI_EVT_MTH();

	const int teamId           = unit.team;
	const int unitId           = unit.id;
	const int aiCommandTopicId = extractAICommandTopic(&command);

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::GotChatMsg(const char* msg, int fromPlayerId) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(GotChatMsg(msg, fromPlayerId))
}



void CEngineOutHandler::CreateSkirmishAI(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	//const bool unpauseAfterAIInit = configHandler->Get("AI_UnpauseAfterInit", true);

	// Pause the game for letting the AI initialize,
	// as this can take quite some time.
	/*bool weDoPause = !gs->paused;
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
	}*/

	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	if (aiData->status != SKIRMAISTATE_RELOADING) {
		net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_INITIALIZING));
	}

	if (aiData->isLuaAI) {
		// currently, we need doing nothing for Lua AIs
		net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	} else {
		CSkirmishAIWrapper* aiWrapper = NULL;
		try {
			CSkirmishAIWrapper* aiWrapper_tmp = new CSkirmishAIWrapper(skirmishAIId);

			id_skirmishAI[skirmishAIId] = aiWrapper_tmp;
			team_skirmishAIs[aiWrapper_tmp->GetTeamId()].push_back(skirmishAIId);

			aiWrapper_tmp->Init();

			aiWrapper = aiWrapper_tmp;
		} CATCH_AI_EXCEPTION;

		const bool isDieing = skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId);
		if (!isDieing) {
			// Send a UnitCreated event for each unit of the team.
			// This will only do something if the AI is created mid-game.
			CTeam* team = teamHandler->Team(aiWrapper->GetTeamId());
			CUnitSet::iterator u, uNext;
			for (u = team->units.begin(); u != team->units.end(); ) {
				uNext = u; ++uNext;
				try {
					aiWrapper->UnitCreated((*u)->id, -1);
					aiWrapper->UnitFinished((*u)->id);
				} CATCH_AI_EXCEPTION;
				u = uNext;
			}

			net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
		}
	}
}

void CEngineOutHandler::SetSkirmishAIDieing(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	try {
		id_skirmishAI[skirmishAIId]->Dieing();
	} CATCH_AI_EXCEPTION;
}

static void internal_aiErase(std::vector<size_t>& ais, const size_t skirmishAIId) {

	for (std::vector<size_t>::iterator ai = ais.begin(); ai != ais.end(); ++ai) {
		if (*ai == skirmishAIId) {
			ais.erase(ai);
			return;
		}
	}
	// failed to remove Skirmish AI ID
	assert(false);
}

void CEngineOutHandler::DestroySkirmishAI(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	try {
		CSkirmishAIWrapper* aiWrapper = id_skirmishAI[skirmishAIId];
		const int reason = skirmishAIHandler.GetLocalSkirmishAIDieReason(skirmishAIId);

		aiWrapper->Release(reason);

		id_skirmishAI.erase(skirmishAIId);
		internal_aiErase(team_skirmishAIs[aiWrapper->GetTeamId()], skirmishAIId);

		delete aiWrapper;
		aiWrapper = NULL;

		net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DEAD));
	} CATCH_AI_EXCEPTION;
}
