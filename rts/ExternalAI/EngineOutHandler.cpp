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
//				CR_MEMBER(localSkirmishAIs_size),
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
//		: localSkirmishAIs_size(0)
{
	/*for (size_t t=0; t < teams_size; ++t) {
		team_skirmishAIs[t] = NULL;
		team_skirmishAIs[t] = ais_t;
	}*/

	/*for (size_t t=0; t < teamHandler->ActiveTeams(); ++t) {
		// TODO: FIXME: uncomment last part of following line
		const SkirmishAIData* data = NULL;//gameSetup->GetSkirmishAIDataForTeam(t);
		if (data != NULL) {
			team_skirmishAI[t] = *data;
		}
	}*/
}

CEngineOutHandler::~CEngineOutHandler() {

	/*for (size_t t=0; t < skirmishAIs_size; ++t) {
		delete skirmishAIs[t];
		skirmishAIs[t] = NULL;
	}*/
}



#define DO_FOR_SKIRMISH_AIS(FUNC)	\
		for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {	\
			try {					\
				ai->second->FUNC;			\
			} HANDLE_EXCEPTION;		\
		}


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {

	//if (id_skirmishAI.size() == 0) return;

	DO_FOR_SKIRMISH_AIS(PreDestroy())
}

void CEngineOutHandler::Load(std::istream* s) {

	//if (id_skirmishAI.size() == 0) return;

	DO_FOR_SKIRMISH_AIS(Load(s))
}

void CEngineOutHandler::Save(std::ostream* s) {

	//if (id_skirmishAI.size() == 0) return;

	DO_FOR_SKIRMISH_AIS(Save(s))
}


void CEngineOutHandler::Update() {

	if (id_skirmishAI.size() == 0) return;

	SCOPED_TIMER("AI")
	int frame = gs->frameNum;
	DO_FOR_SKIRMISH_AIS(Update(frame))
}



// Do only if the unit is not allied, in which case we know
// everything about it anyway, and do not need to be informed
#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)		\
		if (!teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {				\
			for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {	\
				if (teamHandler->AllyTeam(ai->second->GetTeamId()) == ALLY_TEAM_ID) {	\
					try {														\
						ai->second->FUNC;												\
					} HANDLE_EXCEPTION;											\
				}																\
			}																	\
		}


void CEngineOutHandler::UnitEnteredLos(const CUnit& unit, int allyTeamId) {

	if (id_skirmishAI.size() == 0) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftLos(const CUnit& unit, int allyTeamId) {

	if (id_skirmishAI.size() == 0) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveLOS(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitEnteredRadar(const CUnit& unit, int allyTeamId) {

	if (id_skirmishAI.size() == 0) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyEnterRadar(unitId), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::UnitLeftRadar(const CUnit& unit, int allyTeamId) {

	if (id_skirmishAI.size() == 0) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(EnemyLeaveRadar(unitId), allyTeamId, unitAllyTeamId)
}



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)							\
		if (team_skirmishAIs.find(TEAM_ID) != team_skirmishAIs.end()) {	\
			for (ais_t::iterator ai = team_skirmishAIs[TEAM_ID].begin(); ai != team_skirmishAIs[TEAM_ID].end(); ++ai) {	\
				try {													\
					(*ai)->FUNC;										\
				} HANDLE_EXCEPTION;										\
			}															\
		}


void CEngineOutHandler::UnitIdle(const CUnit& unit) {

	if (id_skirmishAI.size() == 0) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {

	if (id_skirmishAI.size() == 0) return;

	int teamId    = unit.team;
	int unitId    = unit.id;
	int builderId = builder? builder->id: -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {

	if (id_skirmishAI.size() == 0) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {

	if (id_skirmishAI.size() == 0) return;

	int teamId = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam) {

	if (id_skirmishAI.size() == 0) return;

	int newTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitGiven(unitId, oldTeam, newTeam), newTeam);
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int newTeam) {

	if (id_skirmishAI.size() == 0) return;

	int oldTeam = unit.team;
	int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCaptured(unitId, oldTeam, newTeam), oldTeam);
}


void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed,
		const CUnit* attacker) {

	if (id_skirmishAI.size() == 0) return;

	int destroyedId = destroyed.id;
	int attackerId  = attacker ? attacker->id : -1;

	// EnemyDestroyed is sent to all allies of the attacking/destroying team
	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		const int t = ai->second->GetTeamId();
		if (!teamHandler->Ally(teamHandler->AllyTeam(t), destroyed.allyteam)
			&& (ai->second->IsCheatEventsEnabled()
				|| (destroyed.losStatus[teamHandler->AllyTeam(t)] & (LOS_INLOS | LOS_INRADAR))))
		{
			try {
				ai->second->EnemyDestroyed(destroyedId, attackerId);
			} HANDLE_EXCEPTION;
		}
	}

	DO_FOR_TEAM_SKIRMISH_AIS(UnitDestroyed(destroyedId, attackerId), destroyed.team);
}


void CEngineOutHandler::UnitDamaged(const CUnit& damaged, const CUnit* attacker,
		float damage, int weaponDefId, bool paralyzer) {

	if (id_skirmishAI.size() == 0) return;

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
	const int dt = damaged.team;
	const int at = attacker ? attacker->team : -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitDamaged(damagedUnitId, attackerUnitId, damage, attackDir_damagedsView, weaponDefId, paralyzer), dt);

	if (attacker) {
		// EnemyDamaged is sent to the attacking team only,
		// not to its allies
		if (!teamHandler->Ally(teamHandler->AllyTeam(at), damaged.allyteam)
				&& (team_skirmishAIs.find(at) != team_skirmishAIs.end()))
		{
			const bool inLosOrRadar = (damaged.losStatus[teamHandler->AllyTeam(at)] & (LOS_INLOS | LOS_INRADAR));
			for (ais_t::iterator ai = team_skirmishAIs[at].begin(); ai != team_skirmishAIs[at].end(); ++ai)
			{
				if (inLosOrRadar || (*ai)->IsCheatEventsEnabled())
				{
					try {
						(*ai)->EnemyDamaged(damagedUnitId, attackerUnitId,
								damage, attackDir_attackersView, weaponDefId, paralyzer);
					} HANDLE_EXCEPTION;
				}
			}
		}
	}
}


void CEngineOutHandler::SeismicPing(int allyTeamId, const CUnit& unit,
		const float3& pos, float strength) {

	if (id_skirmishAI.size() == 0) return;

	int unitId         = unit.id;
	int unitAllyTeamId = unit.allyteam;

	DO_FOR_ALLIED_SKIRMISH_AIS(SeismicPing(allyTeamId, unitId, pos, strength), allyTeamId, unitAllyTeamId)
}

void CEngineOutHandler::WeaponFired(const CUnit& unit, const WeaponDef& def) {

	if (id_skirmishAI.size() == 0) return;

	int teamId = unit.team;
	int unitId = unit.id;
	int defId  = def.id;

	DO_FOR_TEAM_SKIRMISH_AIS(WeaponFired(unitId, defId), teamId);
}

void CEngineOutHandler::PlayerCommandGiven(
		const std::vector<int>& selectedUnitIds, const Command& c, int playerId)
{
	if (id_skirmishAI.size() == 0) return;

	int teamId = playerHandler->Player(playerId)->team;

	DO_FOR_TEAM_SKIRMISH_AIS(PlayerCommandGiven(selectedUnitIds, c, playerId), teamId);
}

void CEngineOutHandler::CommandFinished(const CUnit& unit, const Command& command) {

	if (id_skirmishAI.size() == 0) return;

	int teamId           = unit.team;
	int unitId           = unit.id;
	int aiCommandTopicId = extractAICommandTopic(&command);

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::GotChatMsg(const char* msg, int fromPlayerId) {

	if (id_skirmishAI.size() == 0) return;

	DO_FOR_SKIRMISH_AIS(GotChatMsg(msg, fromPlayerId))
}



void CEngineOutHandler::CreateSkirmishAI(const size_t skirmishAIId) {

	//const bool unpauseAfterAIInit = configHandler->Get("AI_UnpauseAfterInit", true);

	/*if (!teamHandler->IsValidTeam(teamId)) {
		return false;
	}*/

	try {
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

		// TODO: remove this if we finnally support multiple AIs per team
		/*if (team_skirmishAIs[teamId] != NULL) {
			DestroySkirmishAI(teamId);
		}*/

		net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_INITIALIZING));

		CSkirmishAIWrapper* aiWrapper = new CSkirmishAIWrapper(skirmishAIId);

		//skirmishAIs.push_back(aiWrapper);
		id_skirmishAI[skirmishAIId] = aiWrapper;
		team_skirmishAIs[aiWrapper->GetTeamId()].push_back(aiWrapper);

		aiWrapper->Init();

		const bool isDieing = skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId);
		if (!isDieing) {
			// Send a UnitCreated event for each unit of the team.
			// This will only do something if the AI is created mid-game.
			CTeam* team = teamHandler->Team(aiWrapper->GetTeamId());
			CUnitSet::iterator u, uNext;
			for (u = team->units.begin(); u != team->units.end(); ) {
				uNext = u; ++uNext;
				aiWrapper->UnitCreated((*u)->id, -1);
				aiWrapper->UnitFinished((*u)->id);
				u = uNext;
			}

			//localSkirmishAIs_size++;

			net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
		}

		// Unpause the game again, if we paused it and it is still paused.
		/*if (unpauseAfterAIInit && weDoPause) {
			net->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, false));
		}*/

		//return !isDieing;
	} HANDLE_EXCEPTION;

	//return false;
}

/*bool CEngineOutHandler::HasSkirmishAI(const int teamId) const {
	return bla * (skirmishAIs[teamId] != NULL);
}*/

/*const SSkirmishAICallback* CEngineOutHandler::GetSkirmishAICallback(const size_t skirmishAIId) const {

	if (skirmishAIs.find(skirmishAIId) != skirmishAIs.end()) {
		return skirmishAIs[skirmishAIId]->GetCallback();
	} else {
		return NULL;
	}
}*/

static void internal_aiErase(std::vector<CSkirmishAIWrapper*> ais, const CSkirmishAIWrapper* ai) {

	for (size_t a = 0; a < ais.size(); ++a) {
		if (ais[a] == ai) {
			ais.erase(ais.begin() + a);
			break;
		}
	}
}

//TODO: move this whole function to SkirmishAIHandler
/*void CEngineOutHandler::DestroySkirmishAI(const size_t skirmishAIId, const int reason) {

	id_dieReason[skirmishAIId] = reason;

	skirmishAIHandler.SetDieing(skirmishAIId, reason);
	net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DIEING));
}*/

void CEngineOutHandler::DestroySkirmishAI(const size_t skirmishAIId) {

	try {
		//net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DIEING));

		CSkirmishAIWrapper* aiWrapper = id_skirmishAI[skirmishAIId];
		const int reason = skirmishAIHandler.GetLocalSkirmishAIDieReason(skirmishAIId);

		aiWrapper->Release(reason);

		//internal_aiErase(skirmishAIs, aiWrapper);
		id_skirmishAI.erase(skirmishAIId);
		internal_aiErase(team_skirmishAIs[id_skirmishAI[skirmishAIId]->GetTeamId()], aiWrapper);

		delete aiWrapper;
		aiWrapper = NULL;
		//localSkirmishAIs_size--;

		/*if (team_isSkirmishAIInitialized[teamId]) {
			localSkirmishAIs_size--;
			team_isSkirmishAIInitialized[teamId] = false;

			net->Send(CBaseNetProtocol::Get().SendAIDestroyed(gu->myPlayerNum, teamId));
		}*/

		net->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DEAD));
	} HANDLE_EXCEPTION;
}
