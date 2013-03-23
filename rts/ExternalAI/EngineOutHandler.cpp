/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineOutHandler.h"

#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/SSkirmishAICallbackImpl.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/NetProtocol.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/TimeProfiler.h"

#include "System/creg/STL_Map.h"

CONFIG(int, CatchAIExceptions).defaultValue(1);
CONFIG(bool, AI_UnpauseAfterInit).defaultValue(true);

CR_BIND_DERIVED(CEngineOutHandler, CObject, )
CR_REG_METADATA(CEngineOutHandler, (
	CR_MEMBER(id_skirmishAI),
	CR_MEMBER(team_skirmishAIs),
	CR_RESERVED(128)
));


static inline bool IsUnitInLosOrRadarOfAllyTeam(const CUnit& unit, const int allyTeamId) {
	// NOTE:
	//     we check for globalLOS because the LOS-state of a
	//     new unit has not yet been set when it is created
	//     (thus UnitCreated will not produce EnemyCreated,
	//     etc. without this, even when globalLOS is enabled
	//     (for non-cheating AI's))
	return (gs->globalLOS[allyTeamId] || (unit.losStatus[allyTeamId] & (LOS_INLOS | LOS_INRADAR)));
}

/////////////////////////////
// BEGIN: Exception Handling

bool CEngineOutHandler::CatchExceptions() {
	static bool initialized = false;
	static bool catchExceptions = false;

	if (!initialized) {
		catchExceptions = (configHandler->GetInt("CatchAIExceptions") != 0);
		initialized = true;
	}

	return catchExceptions;
}

void CEngineOutHandler::HandleAIException(const char* description) {
	if (CEngineOutHandler::CatchExceptions()) {
		if (description) {
			LOG_L(L_ERROR, "AI Exception: \'%s\'", description);
		} else {
			LOG_L(L_ERROR, "AI Exception");
		}
//		exit(-1);
	}
}

// END: Exception Handling
/////////////////////////////


CEngineOutHandler* CEngineOutHandler::singleton = NULL;

CEngineOutHandler* CEngineOutHandler::GetInstance() {
	static unsigned int numInstances = 0;

	if (singleton == NULL) {
		numInstances += 1;
		singleton = new CEngineOutHandler();
	}

	// if more than one instance, some code called eoh->func()
	// and created another after Destroy was already executed
	// from ~CGame --> usually dtors
	assert(numInstances == 1);
	return singleton;
}
void CEngineOutHandler::Destroy() {
	if (singleton != NULL) {
		singleton->PreDestroy();

		CEngineOutHandler* tmp = singleton;
		singleton = NULL;
		delete tmp;

		IAILibraryManager::Destroy();
	}
}

CEngineOutHandler::~CEngineOutHandler() {
	// id_skirmishAI should be empty already, but this can not hurt
	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		delete ai->second;
	}
}


// This macro should be insterted at the start of each method sending AI events
#define AI_EVT_MTH()                               \
		if (id_skirmishAI.empty()) { return; } \
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



// Send to all teams which the unit is not allied to,
// and which have cheat-events enabled, or the unit in sensor range.
#define DO_FOR_ENEMY_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT)							\
		for (id_ai_t::iterator ai = id_skirmishAI.begin();							\
				ai != id_skirmishAI.end(); ++ai) {									\
			const CSkirmishAIWrapper* saw = ai->second;								\
			const int aiAllyTeam = teamHandler->AllyTeam(saw->GetTeamId());	\
			if (!teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID) &&						\
					(saw->IsCheatEventsEnabled() ||							\
					IsUnitInLosOrRadarOfAllyTeam(UNIT, aiAllyTeam))) {				\
				try {																\
					ai->second->FUNC;												\
				} CATCH_AI_EXCEPTION;												\
			}																		\
		}



void CEngineOutHandler::UnitIdle(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitIdle(unitId), teamId);
}

void CEngineOutHandler::UnitCreated(const CUnit& unit, const CUnit* builder) {
	AI_EVT_MTH();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;
	const int builderId  = builder? builder->id: -1;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyCreated(unitId), allyTeamId, unit);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyFinished(unitId), allyTeamId, unit);
}


void CEngineOutHandler::UnitMoveFailed(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId = unit.team;
	const int unitId = unit.id;

	DO_FOR_TEAM_SKIRMISH_AIS(UnitMoveFailed(unitId), teamId);
}

void CEngineOutHandler::UnitGiven(const CUnit& unit, int oldTeam, int newTeam) {
	AI_EVT_MTH();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler->AllyTeam(oldTeam);

	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		const int t          = ai->second->GetTeamId();
		const int allyT      = teamHandler->AllyTeam(t);
		const bool alliedOld = teamHandler->Ally(allyT, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(allyT, newAllyTeamId);
		// inform the new team and its allies
		// (except those also allied with the old team)
		// the old team and its allies were already informed by UnitCaptured
		bool inform = ((t == newTeam) || (alliedNew && !alliedOld));
		// exclude enemies that know from nothing
		if (inform && !alliedOld && !alliedNew &&
				!ai->second->IsCheatEventsEnabled() &&
				!IsUnitInLosOrRadarOfAllyTeam(unit, allyT)) {
			inform = false;
		}
		if (inform) {
			try {
				ai->second->UnitGiven(unitId, oldTeam, newTeam);
			} CATCH_AI_EXCEPTION;
		}
	}
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int oldTeam, int newTeam) {
	AI_EVT_MTH();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler->AllyTeam(oldTeam);

	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		const int t          = ai->second->GetTeamId();
		const int allyT      = teamHandler->AllyTeam(t);
		const bool alliedOld = teamHandler->Ally(allyT, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(allyT, newAllyTeamId);
		// inform the old team and its allies
		// (except those also allied with the new team)
		// the new team and its allies will be informed by UnitGiven
		bool inform = ((t == oldTeam) || (alliedOld && !alliedNew));
		// exclude enemies that know from nothing
		if (inform && !alliedOld && !alliedNew &&
				!ai->second->IsCheatEventsEnabled() &&
				!IsUnitInLosOrRadarOfAllyTeam(unit, allyT)) {
			inform = false;
		}
		if (inform) {
			try {
				ai->second->UnitCaptured(unitId, oldTeam, newTeam);
			} CATCH_AI_EXCEPTION;
		}
	}
}

void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed, const CUnit* attacker) {
	AI_EVT_MTH();

	const int destroyedId = destroyed.id;
	const int attackerId  = attacker ? attacker->id : -1;
	const int dt          = destroyed.team;

	// inform destroyed units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, destroyed.allyteam);

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

	// inform all enemy teams
	for (id_ai_t::iterator ai = id_skirmishAI.begin(); ai != id_skirmishAI.end(); ++ai) {
		const int t      = ai->second->GetTeamId();
		const int allyT  = teamHandler->AllyTeam(t);

		if (teamHandler->Ally(allyT, destroyed.allyteam))
			continue;

		if (!ai->second->IsCheatEventsEnabled() && !IsUnitInLosOrRadarOfAllyTeam(destroyed, allyT))
			continue;

		int myAttackerId = -1;

		if ((attacker != NULL) && teamHandler->Ally(allyT, attacker->allyteam)) {
			myAttackerId = attackerId;
		}
		try {
			ai->second->EnemyDestroyed(destroyedId, myAttackerId);
		} CATCH_AI_EXCEPTION;
	}
}


void CEngineOutHandler::UnitDamaged(
	const CUnit& damaged,
	const CUnit* attacker,
	float damage,
	int weaponDefID,
	int projectileID,
	bool paralyzer
) {
	AI_EVT_MTH();

	const int damagedUnitId  = damaged.id;
	const int dt             = damaged.team;
	const int attackerUnitId = attacker ? attacker->id : -1;

	// inform damaged units team (not allies)
	if (team_skirmishAIs.find(dt) != team_skirmishAIs.end()) {
		float3 attackDir_damagedsView = ZeroVector;
		if (attacker) {
			attackDir_damagedsView =
					CGameHelper::GetUnitErrorPos(attacker, damaged.allyteam)
					- damaged.pos;
			attackDir_damagedsView.ANormalize();
		}
		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, damaged.allyteam);
		for (ids_t::iterator ai = team_skirmishAIs[dt].begin(); ai != team_skirmishAIs[dt].end(); ++ai) {
			CSkirmishAIWrapper* saw = id_skirmishAI[*ai];
			int visibleAttackerUnitId = -1;
			if (attackerInLosOrRadar || saw->IsCheatEventsEnabled()) {
				visibleAttackerUnitId = attackerUnitId;
			}
			try {
				id_skirmishAI[*ai]->UnitDamaged(damagedUnitId, visibleAttackerUnitId, damage, attackDir_damagedsView, weaponDefID, paralyzer);
			} CATCH_AI_EXCEPTION;
		}
	}

	// inform attacker units team (not allies)
	if (attacker) {
		const int at = attacker ? attacker->team : -1;
		if (!teamHandler->Ally(attacker->allyteam, damaged.allyteam)
				&& (team_skirmishAIs.find(at) != team_skirmishAIs.end())) {
			// direction from the attacker's view
			const float3 attackDir = (attacker->pos
						- CGameHelper::GetUnitErrorPos(&damaged, attacker->allyteam))
						.ANormalize();
			const bool damagedInLosOrRadar = IsUnitInLosOrRadarOfAllyTeam(damaged, attacker->allyteam);
			for (ids_t::iterator ai = team_skirmishAIs[at].begin(); ai != team_skirmishAIs[at].end(); ++ai)
			{
				CSkirmishAIWrapper* saw = id_skirmishAI[*ai];
				if (damagedInLosOrRadar || saw->IsCheatEventsEnabled())
				{
					try {
						saw->EnemyDamaged(damagedUnitId, attackerUnitId, damage,
								attackDir, weaponDefID, paralyzer);
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
	const int aiCommandTopicId = extractAICommandTopic(&command, unitHandler->MaxUnits());

	DO_FOR_TEAM_SKIRMISH_AIS(CommandFinished(unitId, command.aiCommandId, aiCommandTopicId), teamId);
}

void CEngineOutHandler::SendChatMessage(const char* msg, int fromPlayerId) {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(SendChatMessage(msg, fromPlayerId))
}

bool CEngineOutHandler::SendLuaMessages(int aiTeam, const char* inData, std::vector<const char*>& outData) {
	SCOPED_TIMER("AI Total");

	if (id_skirmishAI.empty()) {
		return false;
	}

	id_ai_t::iterator it;
	unsigned int n = 0;

	if (aiTeam != -1) {
		// get the AI's for team <aiTeam>
		const team_ais_t::iterator aiTeamIter = team_skirmishAIs.find(aiTeam);

		if (aiTeamIter == team_skirmishAIs.end()) {
			return false;
		}

		// get the vector of ID's for the AI's
		ids_t& aiIDs = aiTeamIter->second;
		ids_t::iterator aiIDsIter;

		outData.resize(aiIDs.size(), "");

		// send only to AI's in team <aiTeam>
		for (aiIDsIter = aiIDs.begin(); aiIDsIter != aiIDs.end(); ++aiIDsIter) {
			const size_t aiID = aiIDs[*aiIDsIter];

			CSkirmishAIWrapper* wrapperAI = id_skirmishAI[aiID];
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	} else {
		outData.resize(id_skirmishAI.size(), "");

		// broadcast to all AI's across all teams
		//
		// since neither AI ID's nor AI teams are
		// necessarily consecutive, store responses
		// in calling order
		for (it = id_skirmishAI.begin(); it != id_skirmishAI.end(); ++it) {
			CSkirmishAIWrapper* wrapperAI = it->second;
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	}

	return true;
}



void CEngineOutHandler::CreateSkirmishAI(const size_t skirmishAIId) {
	SCOPED_TIMER("AI Total");

	//const bool unpauseAfterAIInit = configHandler->GetBool("AI_UnpauseAfterInit");

	// Pause the game for letting the AI initialize,
	// as this can take quite some time.
	/*bool weDoPause = !gs->paused;
	if (weDoPause) {
		const std::string& myPlayerName = playerHandler->Player(gu->myPlayerNum)->name;
		LOG(
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
			if (gs->frameNum > 0) {
				// We will only get here if the AI is created mid-game.
				aiWrapper->Update(gs->frameNum);
			}
			// Send a UnitCreated event for each unit of the team.
			// This will only do something if the AI is created mid-game.
			const CTeam* team = teamHandler->Team(aiWrapper->GetTeamId());
			CUnitSet::const_iterator u, uNext;
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
		assert(id_skirmishAI[skirmishAIId] != NULL);
		id_skirmishAI[skirmishAIId]->Dieing();
	} CATCH_AI_EXCEPTION;
}

static void internal_aiErase(std::vector<unsigned char>& ais, const unsigned char skirmishAIId) {
	for (std::vector<unsigned char>::iterator ai = ais.begin(); ai != ais.end(); ++ai) {
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

