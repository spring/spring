/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EngineOutHandler.h"

#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SkirmishAIData.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/Team.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"

#include "System/creg/STL_Map.h"

CR_BIND(CEngineOutHandler, )
CR_REG_METADATA(CEngineOutHandler, (
	// FIXME:
	//   "constexpr std::pair<_T1, _T2>::pair(const std::pair<_T1, _T2>&)
	//   [with _T1 = const unsigned char; _T2 = std::unique_ptr<CSkirmishAIWrapper>]’
	//   is implicitly deleted because the default definition would be ill-formed"
	CR_IGNORED(hostSkirmishAIs),
	CR_MEMBER(teamSkirmishAIs)
))


static inline bool IsUnitInLosOrRadarOfAllyTeam(const CUnit& unit, const int allyTeamId) {
	// NOTE:
	//   we check for globalLOS because the LOS-state of a
	//   new unit has not yet been set when it is created
	//   (thus UnitCreated will not produce EnemyCreated,
	//   etc. without this, even when globalLOS is enabled
	//   (for non-cheating AI's))
	return (losHandler->globalLOS[allyTeamId] || (unit.losStatus[allyTeamId] & (LOS_INLOS | LOS_INRADAR)));
}

static CEngineOutHandler* singleton = nullptr;
static unsigned int numInstances = 0;

CEngineOutHandler* CEngineOutHandler::GetInstance() {
	// if more than one instance, some code called eoh->func()
	// and created another after Destroy was already executed
	// from ~CGame --> usually dtors
	assert(numInstances == 1);
	return singleton;
}

void CEngineOutHandler::Create() {
	if (singleton == nullptr) {
		singleton = new CEngineOutHandler();
		numInstances += 1;
	}
}

void CEngineOutHandler::Destroy() {
	if (singleton != nullptr) {
		singleton->PreDestroy();

		spring::SafeDelete(singleton);
		IAILibraryManager::Destroy();

		numInstances -= 1;
	}
}


CEngineOutHandler::~CEngineOutHandler() {
	// hostSkirmishAIs should be empty already, but this can not hurt
	// (it releases only those AI's that were not already released)
	while (!hostSkirmishAIs.empty()) {
		DestroySkirmishAI((hostSkirmishAIs.begin())->first);
	}
}


// This macro should be insterted at the start of each method sending AI events
#define AI_EVT_MTH()              \
	if (hostSkirmishAIs.empty())  \
		return;                   \
	SCOPED_TIMER("AI");


#define DO_FOR_SKIRMISH_AIS(FUNC)       \
	for (auto& ai: hostSkirmishAIs) {   \
		(ai.second)->FUNC;              \
	}


void CEngineOutHandler::PostLoad() {}

void CEngineOutHandler::PreDestroy() {
	AI_EVT_MTH();

	DO_FOR_SKIRMISH_AIS(PreDestroy())
}

void CEngineOutHandler::Load(std::istream* s, const uint8_t skirmishAIId) {
	AI_EVT_MTH();

	const auto it = hostSkirmishAIs.find(skirmishAIId);

	if (it == hostSkirmishAIs.end())
		return;

	(it->second)->Load(s);
}

void CEngineOutHandler::Save(std::ostream* s, const uint8_t skirmishAIId) {
	AI_EVT_MTH();

	const auto it = hostSkirmishAIs.find(skirmishAIId);

	if (it == hostSkirmishAIs.end())
		return;

	(it->second)->Save(s);
}


void CEngineOutHandler::Update() {
	AI_EVT_MTH();

	const int frame = gs->frameNum;

	DO_FOR_SKIRMISH_AIS(Update(frame))
}



// Do only if the unit is not allied, in which case we know
// everything about it anyway, and do not need to be informed
#define DO_FOR_ALLIED_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)            \
	if (!teamHandler->Ally(ALLY_TEAM_ID, UNIT_ALLY_TEAM_ID)) {                       \
		for (auto& p: hostSkirmishAIs) {                                             \
			const int aiAllyTeam = teamHandler->AllyTeam((p.second)->GetTeamId());   \
			if (teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID)) {                       \
				(p.second)->FUNC;                                                    \
			}                                                                        \
		}                                                                            \
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



#define DO_FOR_TEAM_SKIRMISH_AIS(FUNC, TEAM_ID)                    \
	if (teamSkirmishAIs.find(TEAM_ID) == teamSkirmishAIs.end())    \
		return;                                                    \
                                                                   \
	for (uint8_t aiID: teamSkirmishAIs[TEAM_ID]) {                 \
		hostSkirmishAIs[aiID]->FUNC;                               \
	}                                                              \



// Send to all teams which the unit is not allied to,
// and which have cheat-events enabled, or the unit in sensor range.
#define DO_FOR_ENEMY_SKIRMISH_AIS(FUNC, ALLY_TEAM_ID, UNIT)                     \
	for (auto& p: hostSkirmishAIs) {                                            \
		CSkirmishAIWrapper* aiWrapper = (p.second).get();                       \
                                                                                \
		const int aiAllyTeam = teamHandler->AllyTeam(aiWrapper->GetTeamId());   \
                                                                                \
		const bool alliedAI = teamHandler->Ally(aiAllyTeam, ALLY_TEAM_ID);      \
		const bool cheatingAI = aiWrapper->IsCheatEventsEnabled();              \
                                                                                \
		if (alliedAI)                                                           \
			continue;                                                           \
		if (cheatingAI || IsUnitInLosOrRadarOfAllyTeam(UNIT, aiAllyTeam)) {     \
			aiWrapper->FUNC;                                                    \
		}                                                                       \
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

	// enemies first, do_for_team can return early
	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyCreated(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitCreated(unitId, builderId), teamId);
}

void CEngineOutHandler::UnitFinished(const CUnit& unit) {
	AI_EVT_MTH();

	const int teamId     = unit.team;
	const int allyTeamId = unit.allyteam;
	const int unitId     = unit.id;

	DO_FOR_ENEMY_SKIRMISH_AIS(EnemyFinished(unitId), allyTeamId, unit);
	DO_FOR_TEAM_SKIRMISH_AIS(UnitFinished(unitId), teamId);
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

	for (auto& p: hostSkirmishAIs) {
		CSkirmishAIWrapper* aiWrapper = (p.second).get();

		const int aiTeam     = aiWrapper->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);
		const bool alliedOld = teamHandler->Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(aiAllyTeam, newAllyTeamId);

		// inform the new team and its allies
		// (except those also allied with the old team,
		// which were already informed by UnitCaptured)
		bool informAI = ((aiTeam == newTeam) || (alliedNew && !alliedOld));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= aiWrapper->IsCheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		aiWrapper->UnitGiven(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitCaptured(const CUnit& unit, int oldTeam, int newTeam) {
	AI_EVT_MTH();

	const int unitId        = unit.id;
	const int newAllyTeamId = unit.allyteam;
	const int oldAllyTeamId = teamHandler->AllyTeam(oldTeam);

	for (auto& p: hostSkirmishAIs) {
		CSkirmishAIWrapper* aiWrapper = (p.second).get();

		const int aiTeam     = aiWrapper->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);

		const bool alliedOld = teamHandler->Ally(aiAllyTeam, oldAllyTeamId);
		const bool alliedNew = teamHandler->Ally(aiAllyTeam, newAllyTeamId);

		// inform the old team and its allies
		// (except those also allied with the new team,
		// which will be informed by UnitGiven instead)
		bool informAI = ((aiTeam == oldTeam) || (alliedOld && !alliedNew));

		if (informAI) {
			informAI  = false;
			informAI |= alliedOld;
			informAI |= alliedNew;
			informAI |= aiWrapper->IsCheatEventsEnabled();
			informAI |= IsUnitInLosOrRadarOfAllyTeam(unit, aiAllyTeam);
		}

		if (!informAI)
			continue;

		aiWrapper->UnitCaptured(unitId, oldTeam, newTeam);
	}
}

void CEngineOutHandler::UnitDestroyed(const CUnit& destroyed, const CUnit* attacker) {
	AI_EVT_MTH();

	const int destroyedId = destroyed.id;
	const int attackerId  = attacker ? attacker->id : -1;
	const int dt          = destroyed.team;

	// inform destroyed units team (not allies)
	if (teamSkirmishAIs.find(dt) != teamSkirmishAIs.end()) {
		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, destroyed.allyteam);

		for (uint8_t aiID: teamSkirmishAIs[dt]) {
			CSkirmishAIWrapper* aiWrapper = hostSkirmishAIs[aiID].get();
			int visibleAttackerId = -1;

			if (attackerInLosOrRadar || aiWrapper->IsCheatEventsEnabled())
				visibleAttackerId = attackerId;

			aiWrapper->UnitDestroyed(destroyedId, visibleAttackerId);
		}
	}

	// inform all enemy teams
	for (auto& p: hostSkirmishAIs) {
		CSkirmishAIWrapper* aiWrapper = (p.second).get();

		const int aiTeam = aiWrapper->GetTeamId();
		const int aiAllyTeam = teamHandler->AllyTeam(aiTeam);

		if (teamHandler->Ally(aiAllyTeam, destroyed.allyteam))
			continue;

		if (!aiWrapper->IsCheatEventsEnabled() && !IsUnitInLosOrRadarOfAllyTeam(destroyed, aiAllyTeam))
			continue;

		int myAttackerId = -1;

		if ((attacker != nullptr) && teamHandler->Ally(aiAllyTeam, attacker->allyteam))
			myAttackerId = attackerId;

		aiWrapper->EnemyDestroyed(destroyedId, myAttackerId);
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
	if (teamSkirmishAIs.find(dt) != teamSkirmishAIs.end()) {
		float3 attackeeDir;

		if (attacker != nullptr)
			attackeeDir = (attacker->GetErrorPos(damaged.allyteam) - damaged.pos).ANormalize();

		const bool attackerInLosOrRadar = attacker && IsUnitInLosOrRadarOfAllyTeam(*attacker, damaged.allyteam);

		for (uint8_t aiID: teamSkirmishAIs[dt]) {
			CSkirmishAIWrapper* aiWrapper = hostSkirmishAIs[aiID].get();

			int visibleAttackerUnitId = -1;

			if (attackerInLosOrRadar || aiWrapper->IsCheatEventsEnabled())
				visibleAttackerUnitId = attackerUnitId;

			aiWrapper->UnitDamaged(damagedUnitId, visibleAttackerUnitId, damage, attackeeDir, weaponDefID, paralyzer);
		}
	}

	// inform attacker units team (not allies)
	if (attacker != nullptr) {
		const int at = attacker ? attacker->team : -1;

		if (teamHandler->Ally(attacker->allyteam, damaged.allyteam))
			return;
		if (teamSkirmishAIs.find(at) == teamSkirmishAIs.end())
			return;

		// direction from the attacker's view
		const float3 attackerDir = (attacker->pos - damaged.GetErrorPos(attacker->allyteam)).ANormalize();
		const bool damagedInLosOrRadar = IsUnitInLosOrRadarOfAllyTeam(damaged, attacker->allyteam);

		for (uint8_t aiID: teamSkirmishAIs[at]) {
			CSkirmishAIWrapper* aiWrapper = hostSkirmishAIs[aiID].get();

			if (!damagedInLosOrRadar && !aiWrapper->IsCheatEventsEnabled())
				continue;

			aiWrapper->EnemyDamaged(damagedUnitId, attackerUnitId, damage, attackerDir, weaponDefID, paralyzer);
		}
	}
}


void CEngineOutHandler::SeismicPing(
	int allyTeamId,
	const CUnit& unit,
	const float3& pos,
	float strength
) {
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
	SCOPED_TIMER("AI");

	if (hostSkirmishAIs.empty())
		return false;

	unsigned int n = 0;

	if (aiTeam != -1) {
		// get the AI's for team <aiTeam>
		const auto aiTeamIt = teamSkirmishAIs.find(aiTeam);

		if (aiTeamIt == teamSkirmishAIs.end())
			return false;

		// get the vector of ID's for the AI's
		const ids_t& aiIDs = aiTeamIt->second;

		outData.resize(aiIDs.size(), "");

		// send only to AI's in team <aiTeam>
		for (uint8_t aiID: aiIDs) {
			CSkirmishAIWrapper* wrapperAI = hostSkirmishAIs[aiID].get();
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	} else {
		outData.resize(hostSkirmishAIs.size(), "");

		// broadcast to all AI's across all teams
		//
		// since neither AI ID's nor AI teams are
		// necessarily consecutive, store responses
		// in calling order
		for (auto& p: hostSkirmishAIs) {
			CSkirmishAIWrapper* wrapperAI = (p.second).get();
			wrapperAI->SendLuaMessage(inData, &outData[n++]);
		}
	}

	return true;
}



void CEngineOutHandler::CreateSkirmishAI(const uint8_t skirmishAIId) {
	SCOPED_TIMER("AI");

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
		clientNet->Send(CBaseNetProtocol::Get().SendPause(gu->myPlayerNum, true));
	}*/

	const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	if (aiData->status != SKIRMAISTATE_RELOADING) {
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_INITIALIZING));
	}

	if (aiData->isLuaAI) {
		// currently, we need to do nothing for Lua AIs
		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	} else {
		CSkirmishAIWrapper* aiWrapper = nullptr;

		hostSkirmishAIs[skirmishAIId].reset(aiWrapper = new CSkirmishAIWrapper(skirmishAIId));
		teamSkirmishAIs[aiWrapper->GetTeamId()].push_back(skirmishAIId);

		aiWrapper->Init();

		if (aiWrapper == nullptr)
			return;
		if (skirmishAIHandler.IsLocalSkirmishAIDieing(skirmishAIId))
			return;

		// We will only get here if the AI is created mid-game.
		if (!gs->PreSimFrame())
			aiWrapper->Update(gs->frameNum);

		// Send a UnitCreated event for each unit of the team.
		// This will only do something if the AI is created mid-game.
		const CTeam* team = teamHandler->Team(aiWrapper->GetTeamId());

		for (CUnit* u: team->units) {
			aiWrapper->UnitCreated(u->id, -1);
			aiWrapper->UnitFinished(u->id);
		}

		clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_ALIVE));
	}
}

void CEngineOutHandler::SetSkirmishAIDieing(const uint8_t skirmishAIId) {
	SCOPED_TIMER("AI");

	// if exiting before start, AI's have not been loaded yet
	// and std::map<>::operator[] would insert a NULL instance
	if (hostSkirmishAIs.find(skirmishAIId) == hostSkirmishAIs.end())
		return;

	assert(hostSkirmishAIs[skirmishAIId].get() != nullptr);
	hostSkirmishAIs[skirmishAIId]->Dieing();
}

void CEngineOutHandler::DestroySkirmishAI(const uint8_t skirmishAIId) {
	SCOPED_TIMER("AI");

	CSkirmishAIWrapper* aiWrapper = hostSkirmishAIs[skirmishAIId].get();

	const int teamID = aiWrapper->GetTeamId();
	const auto aiIter = std::find(teamSkirmishAIs[teamID].begin(), teamSkirmishAIs[teamID].end(), skirmishAIId);

	if (aiIter != teamSkirmishAIs[teamID].end()) {
		teamSkirmishAIs[teamID].erase(aiIter);
	} else {
		assert(false);
	}

	hostSkirmishAIs.erase(skirmishAIId);

	clientNet->Send(CBaseNetProtocol::Get().SendAIStateChanged(gu->myPlayerNum, skirmishAIId, SKIRMAISTATE_DEAD));
}

